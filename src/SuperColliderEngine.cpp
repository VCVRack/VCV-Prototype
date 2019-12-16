#include "ScriptEngine.hpp"

#include "lang/SC_LanguageClient.h"
#include "LangSource/SC_LanguageConfig.hpp"
#include "LangSource/SCBase.h"
#include "LangSource/VMGlobals.h"
#include "LangSource/PyrObject.h"

#include <thread>
#include <atomic>
#include <unistd.h> // getcwd

// SuperCollider script engine for VCV-Prototype
// Original author: Brian Heim <brianlheim@gmail.com>

/* DESIGN
 *
 * This is currently a work in progress. The idea is that the user writes a script
 * that defines a couple environment variables:
 *
 * ~vcv_frameDivider: Integer
 * ~vcv_bufferSize: Integer
 * ~vcv_process: Function (VcvPrototypeProcessBlock -> VcvPrototypeProcessBlock)
 *
 * ~vcv_process is invoked once per process block. Ideally, users should not manipulate
 * the block object in any way other than by writing directly to the arrays in `outputs`,
 * `knobs`, `lights`, and `switchLights`.
 */

extern rack::plugin::Plugin* pluginInstance; // plugin's version of 'this'
class SuperColliderEngine;

class SC_VcvPrototypeClient final : public SC_LanguageClient {
public:
	SC_VcvPrototypeClient(SuperColliderEngine* engine);
	~SC_VcvPrototypeClient();

	// These will invoke the interpreter
	void interpret(const char * text) noexcept;
	void evaluateProcessBlock(ProcessBlock* block) noexcept;
	int getFrameDivider() noexcept { return getResultAsInt("^~vcv_frameDivider"); }
	int getBufferSize() noexcept { return getResultAsInt("^~vcv_bufferSize"); }

	bool isOk() const noexcept { return _ok; }

	void postText(const char* str, size_t len) override;

	// No concept of flushing or stdout vs stderr
	void postFlush(const char* str, size_t len) override { postText(str, len); }
	void postError(const char* str, size_t len) override { postText(str, len); }
	void flush() override {}

private:
	std::string buildScProcessBlockString(const ProcessBlock* block) const noexcept;
	int getResultAsInt(const char* text) noexcept;
	bool isVcvPrototypeProcessBlock(const PyrSlot* slot) const noexcept;

	// converts top of stack back to ProcessBlock data
	void readScProcessBlockResult(ProcessBlock* block) noexcept;

	SuperColliderEngine* _engine;
	bool _ok = true;
};

class SuperColliderEngine final : public ScriptEngine {
public:
	~SuperColliderEngine() noexcept { _clientThread.join(); }

	std::string getEngineName() override { return "SuperCollider"; }

	int run(const std::string& path, const std::string& script) override {
		if (!_clientThread.joinable()) {
			_clientThread = std::thread([this, script]() {
				_client.reset(new SC_VcvPrototypeClient(this));
				_client->interpret(script.c_str());
				// _client->setNumRows(); TODO
				setFrameDivider(_client->getFrameDivider());
				setBufferSize(_client->getBufferSize());
				finishClientLoading();
			});
		}

		return 0;
	}

	int process() override {
		if (waitingOnClient())
			return 0;

		if (clientHasError())
			return 1;

		_client->evaluateProcessBlock(getProcessBlock());
		return clientHasError() ? 1 : 0;
	}

private:
	bool waitingOnClient() const noexcept { return !_clientRunning; }
	bool clientHasError() const noexcept { return !_client->isOk(); }
	void finishClientLoading() noexcept { _clientRunning = true; }

	std::unique_ptr<SC_VcvPrototypeClient> _client;
	std::thread _clientThread; // used only to start up client
	std::atomic_bool _clientRunning{false}; // set to true when client is ready to process data
};

// TODO
#define FAIL(_msg_) do { _engine->display(_msg_); _ok = false; } while (0)

SC_VcvPrototypeClient::SC_VcvPrototypeClient(SuperColliderEngine* engine)
	: SC_LanguageClient("SC VCV-Prototype client")
	, _engine(engine)
{
	using Path = SC_LanguageConfig::Path;
	Path sc_lib_root = rack::asset::plugin(pluginInstance, "dep/supercollider/SCClassLibrary");
	Path sc_ext_root = rack::asset::plugin(pluginInstance, "dep/supercollider_extensions");
	Path sc_yaml_path = rack::asset::plugin(pluginInstance, "dep/supercollider/sclang_vcv_config.yml");

	if (!SC_LanguageConfig::defaultLibraryConfig(/* isStandalone */ true))
		FAIL("Failed setting default library config");
	if (!gLanguageConfig->addIncludedDirectory(sc_lib_root))
		FAIL("Failed to add main include directory");
	if (!gLanguageConfig->addIncludedDirectory(sc_ext_root))
		FAIL("Failed to add extensions include directory");
	if (!SC_LanguageConfig::writeLibraryConfigYAML(sc_yaml_path))
		FAIL("Failed to write library config YAML file");

	SC_LanguageConfig::setConfigPath(sc_yaml_path);

	// TODO allow users to add extensions somehow?

	initRuntime();
	compileLibrary(/* isStandalone */ true);
	// TODO better logging here?
	if (!isLibraryCompiled())
		FAIL("Error while compiling class library");
}

SC_VcvPrototypeClient::~SC_VcvPrototypeClient() {
	shutdownLibrary();
	shutdownRuntime();
}

void SC_VcvPrototypeClient::interpret(const char* text) noexcept {
	setCmdLine(text);
	interpretCmdLine();
}

void SC_VcvPrototypeClient::postText(const char* str, size_t len) {
	// Ensure the last message logged (presumably an error) stays onscreen.
	if (_ok)
		_engine->display(std::string(str, len));
}

std::string SC_VcvPrototypeClient::buildScProcessBlockString(const ProcessBlock* block) const noexcept {
	std::ostringstream builder;

	// TODO so expensive
	builder << std::fixed; // to ensure floats aren't actually treated as Integers
	builder << "^~vcv_process.value(VcvPrototypeProcessBlock.new("
		<< block->sampleRate << ','
		<< block->sampleTime << ','
		<< block->bufferSize << ',';

	auto&& appendInOutArray = [&builder](const int bufferSize, const float (&data)[NUM_ROWS][MAX_BUFFER_SIZE]) {
		builder << '[';
		for (int i = 0; i < NUM_ROWS; ++i) {
			builder << "FloatArray[";
			for (int j = 0; j < bufferSize; ++j) {
				builder << data[i][j];
				if (j != bufferSize - 1)
					builder << ',';
			}
			builder << ']';
		if (i != NUM_ROWS - 1)
			builder << ',';
		}
		builder << ']';
	};

	appendInOutArray(block->bufferSize, block->inputs);
	builder << ',';
	appendInOutArray(block->bufferSize, block->outputs);
	builder << ',';

	// knobs
	builder << "FloatArray[";
	for (int i = 0; i < NUM_ROWS; ++i) {
		builder << block->knobs[i];
		if (i != NUM_ROWS - 1)
			builder << ',';
	}
	builder << "],";

	// switches
	builder << '[' << std::boolalpha;
	for (int i = 0; i < NUM_ROWS; ++i) {
		builder << block->switches[i];
		if (i != NUM_ROWS - 1)
			builder << ',';
	}
	builder << "],";

	// lights, switchlights
	auto&& appendLightsArray = [&builder](const float (&array)[NUM_ROWS][3]) {
		builder << '[';
		for (int i = 0; i < NUM_ROWS; ++i) {
			builder << "FloatArray[" << array[i][0] << ',' << array[i][1] << ',' << array[i][2] << ']';
			if (i != NUM_ROWS - 1)
				builder << ',';
		}
		builder << ']';
	};

	appendLightsArray(block->lights);
	builder << ',';
	appendLightsArray(block->switchLights);

	builder << "));\n";
	return builder.str();
}

bool SC_VcvPrototypeClient::isVcvPrototypeProcessBlock(const PyrSlot* slot) const noexcept {
	// TODO
	return true;
}

void SC_VcvPrototypeClient::readScProcessBlockResult(ProcessBlock* block) noexcept {
	auto* resultSlot = &scGlobals()->result;
	if (!isVcvPrototypeProcessBlock(resultSlot)) {
		FAIL("Result of ~vcv_process must be an instance of VcvPrototypeProcessBlock");
		return;
	}

	constexpr unsigned outputsSlotIndex = 4;
	constexpr unsigned knobsSlotIndex = 5;
	constexpr unsigned lightsSlotIndex = 7;
	constexpr unsigned switchLightsSlotIndex = 8;

	PyrObject* object = slotRawObject(resultSlot);

	{
		// OUTPUTS
		PyrSlot* outputsSlot = &object->slots[outputsSlotIndex];
		// TODO check is array
		// TODO check size
		PyrObject* outputsObj = slotRawObject(outputsSlot);
		for (int i = 0; i < NUM_ROWS; ++i) {
			PyrSlot* floatArraySlot = &outputsObj->slots[i];
			// TODO check is floatarray
			// TODO check size
			PyrObject* floatArrayObj = slotRawObject(floatArraySlot);
			PyrFloatArray* floatArray = reinterpret_cast<PyrFloatArray*>(floatArrayObj);
			for (int j = 0; j < block->bufferSize; ++j) {
				block->outputs[i][j] = floatArray->f[j];
			}
		}
	}

	{
		// KNOBS
		PyrSlot* knobsSlot = &object->slots[knobsSlotIndex];
		// TODO check is floatarray
		// TODO check size
		PyrObject* knobsObj = slotRawObject(knobsSlot);
		PyrFloatArray* floatArray = reinterpret_cast<PyrFloatArray*>(knobsObj);
		for (int i = 0; i < NUM_ROWS; ++i) {
			block->knobs[i] = floatArray->f[i];
		}
	}

	{
		// LIGHTS
		PyrSlot* lightsSlot = &object->slots[lightsSlotIndex];
		// TODO check is array
		// TODO check size
		PyrObject* lightsObj = slotRawObject(lightsSlot);
		for (int i = 0; i < NUM_ROWS; ++i) {
			PyrSlot* floatArraySlot = &lightsObj->slots[i];
			// TODO check is floatarray
			// TODO check size
			PyrObject* floatArrayObj = slotRawObject(floatArraySlot);
			PyrFloatArray* floatArray = reinterpret_cast<PyrFloatArray*>(floatArrayObj);
			block->lights[i][0] = floatArray->f[0];
			block->lights[i][1] = floatArray->f[1];
			block->lights[i][2] = floatArray->f[2];
		}
	}

	{
		// SWITCH LIGHTS
		PyrSlot* switchLightsSlot = &object->slots[switchLightsSlotIndex];
		// TODO check is array
		// TODO check size
		PyrObject* switchLightsObj = slotRawObject(switchLightsSlot);
		for (int i = 0; i < NUM_ROWS; ++i) {
			PyrSlot* floatArraySlot = &switchLightsObj->slots[i];
			// TODO check is floatarray
			// TODO check size
			PyrObject* floatArrayObj = slotRawObject(floatArraySlot);
			PyrFloatArray* floatArray = reinterpret_cast<PyrFloatArray*>(floatArrayObj);
			block->switchLights[i][0] = floatArray->f[0];
			block->switchLights[i][1] = floatArray->f[1];
			block->switchLights[i][2] = floatArray->f[2];
		}
	}
}

// TODO test code
static long long int gmax = 0;

void SC_VcvPrototypeClient::evaluateProcessBlock(ProcessBlock* block) noexcept {
	// TODO timing test code
	auto start = std::chrono::high_resolution_clock::now();
	auto&& string = buildScProcessBlockString(block);
	interpret(string.c_str());
	readScProcessBlockResult(block);
	auto end = std::chrono::high_resolution_clock::now();
	auto ticks = (end - start).count();
	if (gmax < ticks)
	{
		gmax = ticks;
		printf("MAX TIME %lld\n", ticks);
	}
}

int SC_VcvPrototypeClient::getResultAsInt(const char* text) noexcept {
	interpret(text);

	auto* resultSlot = &scGlobals()->result;
	if (IsInt(resultSlot)) {
		auto intResult = slotRawInt(resultSlot);
		if (intResult > 0) {
			return intResult;
		} else {
			// TODO better formatting
			FAIL(std::string("Result of '") + text + "' should be > 0");
			return -1;
		}
	} else {
		FAIL(std::string("Result of '") + text + "' should be Integer");
		return -1;
	}
}

__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<SuperColliderEngine>("sc");
	addScriptEngine<SuperColliderEngine>("scd");
}
