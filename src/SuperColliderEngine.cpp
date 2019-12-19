#include "ScriptEngine.hpp"

#include "lang/SC_LanguageClient.h"
#include "LangSource/SC_LanguageConfig.hpp"
#include "LangSource/SCBase.h"
#include "LangSource/VMGlobals.h"
#include "LangSource/PyrObject.h"
#include "LangSource/PyrKernel.h"

#include <thread>
#include <atomic>
#include <numeric>
#include <unistd.h> // getcwd

// SuperCollider script engine for VCV-Prototype
// Original author: Brian Heim <brianlheim@gmail.com>

/* DESIGN
 *
 * The idea is that the user writes a script that defines a couple environment variables:
 *
 * ~vcv_frameDivider: Integer
 * ~vcv_bufferSize: Integer
 * ~vcv_process: Function (VcvPrototypeProcessBlock -> VcvPrototypeProcessBlock)
 *
 * ~vcv_process is invoked once per process block. Users should not manipulate
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
	void setNumRows() noexcept {
		std::string&& command = "VcvPrototypeProcessBlock.numRows = " + std::to_string(NUM_ROWS);
		interpret(command.c_str());
	}
	int getFrameDivider() noexcept { return getResultAsInt("^~vcv_frameDivider"); }
	int getBufferSize() noexcept { return getResultAsInt("^~vcv_bufferSize"); }

	bool isOk() const noexcept { return _ok; }

	void postText(const char* str, size_t len) override;

	// No concept of flushing or stdout vs stderr
	void postFlush(const char* str, size_t len) override { postText(str, len); }
	void postError(const char* str, size_t len) override { postText(str, len); }
	void flush() override {}

private:
	const char* buildScProcessBlockString(const ProcessBlock* block) const noexcept;
	int getResultAsInt(const char* text) noexcept;
	bool isVcvPrototypeProcessBlock(const PyrSlot* slot) const noexcept;

	void fail(const std::string& msg) noexcept;

	// converts top of stack back to ProcessBlock data
	void readScProcessBlockResult(ProcessBlock* block) noexcept;

	// helpers for copying SC info back into process block's arrays
	template <typename Array>
	bool copyArrayOfFloatArrays(const PyrSlot& inSlot, const char* context, Array& array, int size) noexcept;
	bool copyFloatArray(const PyrSlot& inSlot, const char* context, float* outArray, int size) noexcept;

	SuperColliderEngine* _engine;
	PyrSymbol* _vcvPrototypeProcessBlockSym;
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
				_client->setNumRows();
				_client->interpret(script.c_str());
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

SC_VcvPrototypeClient::SC_VcvPrototypeClient(SuperColliderEngine* engine)
	: SC_LanguageClient("SC VCV-Prototype client")
	, _engine(engine)
{
	using Path = SC_LanguageConfig::Path;
	Path sc_lib_root = rack::asset::plugin(pluginInstance, "dep/supercollider/SCClassLibrary");
	Path sc_ext_root = rack::asset::plugin(pluginInstance, "dep/supercollider_extensions");
	Path sc_yaml_path = rack::asset::plugin(pluginInstance, "dep/supercollider/sclang_vcv_config.yml");

	if (!SC_LanguageConfig::defaultLibraryConfig(/* isStandalone */ true))
		fail("Failed setting default library config");
	if (!gLanguageConfig->addIncludedDirectory(sc_lib_root))
		fail("Failed to add main include directory");
	if (!gLanguageConfig->addIncludedDirectory(sc_ext_root))
		fail("Failed to add extensions include directory");
	if (!SC_LanguageConfig::writeLibraryConfigYAML(sc_yaml_path))
		fail("Failed to write library config YAML file");

	SC_LanguageConfig::setConfigPath(sc_yaml_path);

	// TODO allow users to add extensions somehow?

	initRuntime();
	compileLibrary(/* isStandalone */ true);
	if (!isLibraryCompiled())
		fail("Error while compiling class library");

	_vcvPrototypeProcessBlockSym = getsym("VcvPrototypeProcessBlock");
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

// This should be well above what we ever need to represent a process block.
constexpr unsigned overhead = 512;
constexpr unsigned floatSize = 10;
constexpr unsigned insOutsSize = MAX_BUFFER_SIZE * NUM_ROWS * 2 * floatSize;
constexpr unsigned otherArraysSize = floatSize * NUM_ROWS * 8;
constexpr unsigned bufferSize = overhead + insOutsSize + otherArraysSize;

// Don't write initial string every time
#define PROCESS_BEGIN_STRING "^~vcv_process.(VcvPrototypeProcessBlock.new("
static char processBlockStringScratchBuf[bufferSize] = PROCESS_BEGIN_STRING;
constexpr unsigned processBeginStringOffset = sizeof(PROCESS_BEGIN_STRING);
#undef PROCESS_BEGIN_STRING

const char* SC_VcvPrototypeClient::buildScProcessBlockString(const ProcessBlock* block) const noexcept {
	auto* buf = processBlockStringScratchBuf + processBeginStringOffset - 1;

	// Perhaps imprudently assuming snprintf never returns a negative code
	buf += std::sprintf(buf, "%.6f,%.6f,%d,", block->sampleRate, block->sampleTime, block->bufferSize);

	auto&& appendInOutArray = [&buf](const int bufferSize, const float (&data)[NUM_ROWS][MAX_BUFFER_SIZE]) {
		buf += std::sprintf(buf, "[");
		for (int i = 0; i < NUM_ROWS; ++i) {
			buf += std::sprintf(buf, "Signal[");
			for (int j = 0; j < bufferSize; ++j) {
				buf += std::sprintf(buf, "%g%c", data[i][j], j == bufferSize - 1 ? ' ' : ',');
			}
			buf += std::sprintf(buf, "]%c", i == NUM_ROWS - 1 ? ' ' : ',');
		}
		buf += std::sprintf(buf, "],");
	};

	appendInOutArray(block->bufferSize, block->inputs);
	appendInOutArray(block->bufferSize, block->outputs);

	// knobs
	buf += std::sprintf(buf, "FloatArray[");
	for (int i = 0; i < NUM_ROWS; ++i)
		buf += std::sprintf(buf, "%g%c", block->knobs[i], i == NUM_ROWS - 1 ? ' ' : ',');

	// switches
	buf += std::sprintf(buf, "],[");
	for (int i = 0; i < NUM_ROWS; ++i)
		buf += std::sprintf(buf, "%s%c", block->switches[i] ? "true" : "false", i == NUM_ROWS - 1 ? ' ' : ',');
	buf += std::sprintf(buf, "]");

	// lights, switchlights
	auto&& appendLightsArray = [&buf](const float (&array)[NUM_ROWS][3]) {
		buf += std::sprintf(buf, ",[");
		for (int i = 0; i < NUM_ROWS; ++i) {
			buf += std::sprintf(buf, "FloatArray[%g,%g,%g]%c", array[i][0], array[i][1], array[i][2],
				i == NUM_ROWS - 1 ? ' ' : ',');
		}
		buf += std::sprintf(buf, "]");
	};

	appendLightsArray(block->lights);
	appendLightsArray(block->switchLights);

	buf += std::sprintf(buf, "));");

	return processBlockStringScratchBuf;
}

bool SC_VcvPrototypeClient::isVcvPrototypeProcessBlock(const PyrSlot* slot) const noexcept {
	if (NotObj(slot))
		return false;

	auto* klass = slotRawObject(slot)->classptr;
	auto* klassNameSymbol = slotRawSymbol(&klass->name);
	return klassNameSymbol == _vcvPrototypeProcessBlockSym;
}

template <typename Array>
bool SC_VcvPrototypeClient::copyArrayOfFloatArrays(const PyrSlot& inSlot, const char* context, Array& outArray, int size) noexcept
{
	// OUTPUTS
	if (!isKindOfSlot(const_cast<PyrSlot*>(&inSlot), class_array)) {
		fail(std::string(context) + " must be a Array");
		return false;
	}
	auto* inObj = slotRawObject(&inSlot);
	if (inObj->size != NUM_ROWS) {
		fail(std::string(context) + " must be of size " + std::to_string(NUM_ROWS));
		return false;
	}

	for (int i = 0; i < NUM_ROWS; ++i) {
		if (!copyFloatArray(inObj->slots[i], "subarray", outArray[i], size)) {
			return false;
		}
	}

	return true;
}

bool SC_VcvPrototypeClient::copyFloatArray(const PyrSlot& inSlot, const char* context, float* outArray, int size) noexcept
{
	if (!isKindOfSlot(const_cast<PyrSlot*>(&inSlot), class_floatarray)) {
		fail(std::string(context) + " must be a FloatArray");
		return false;
	}
	auto* floatArrayObj = slotRawObject(&inSlot);
	if (floatArrayObj->size != size) {
		fail(std::string(context) + " must be of size " + std::to_string(size));
		return false;
	}

	auto* floatArray = reinterpret_cast<const PyrFloatArray*>(floatArrayObj);
	auto* rawArray = static_cast<const float*>(floatArray->f);
	std::memcpy(outArray, rawArray, size * sizeof(float));
	return true;
}

void SC_VcvPrototypeClient::readScProcessBlockResult(ProcessBlock* block) noexcept {
	auto* resultSlot = &scGlobals()->result;
	if (!isVcvPrototypeProcessBlock(resultSlot)) {
		fail("Result of ~vcv_process must be an instance of VcvPrototypeProcessBlock");
		return;
	}

	PyrObject* object = slotRawObject(resultSlot);
	auto* rawSlots = static_cast<PyrSlot*>(object->slots);

	// See .sc object definition
	constexpr unsigned outputsSlotIndex = 4;
	constexpr unsigned knobsSlotIndex = 5;
	constexpr unsigned lightsSlotIndex = 7;
	constexpr unsigned switchLightsSlotIndex = 8;

	if (!copyArrayOfFloatArrays(rawSlots[outputsSlotIndex], "outputs", block->outputs, block->bufferSize))
		return;
	if (!copyArrayOfFloatArrays(rawSlots[lightsSlotIndex], "lights", block->lights, 3))
		return;
	if (!copyArrayOfFloatArrays(rawSlots[switchLightsSlotIndex], "switchLights", block->switchLights, 3))
		return;
	if (!copyFloatArray(rawSlots[knobsSlotIndex], "knobs", block->knobs, NUM_ROWS))
		return;
}

void SC_VcvPrototypeClient::fail(const std::string& msg) noexcept {
	_engine->display(msg);
	_ok = false;
}

#ifdef SC_VCV_ENGINE_TIMING
static long long int gmax = 0;
static constexpr unsigned int nTimes = 1024;
static long long int times[nTimes] = {};
static unsigned int timesIndex = 0;
#endif

void SC_VcvPrototypeClient::evaluateProcessBlock(ProcessBlock* block) noexcept {
#ifdef SC_VCV_ENGINE_TIMING
	auto start = std::chrono::high_resolution_clock::now();
#endif
	auto* buf = buildScProcessBlockString(block);
	interpret(buf);
	readScProcessBlockResult(block);
#ifdef SC_VCV_ENGINE_TIMING
	auto end = std::chrono::high_resolution_clock::now();
	auto ticks = (end - start).count();

	times[timesIndex] = ticks;
	timesIndex++;
	timesIndex %= nTimes;
	if (gmax < ticks)
	{
		gmax = ticks;
		std::printf("MAX TIME %lld\n", ticks);
	}
	if (timesIndex == 0)
	{
		std::printf("AVG TIME %lld\n", std::accumulate(std::begin(times), std::end(times), 0ull) / nTimes);
	}
#endif
}

int SC_VcvPrototypeClient::getResultAsInt(const char* text) noexcept {
	interpret(text);

	auto* resultSlot = &scGlobals()->result;
	if (IsInt(resultSlot)) {
		auto intResult = slotRawInt(resultSlot);
		if (intResult > 0) {
			return intResult;
		} else {
			fail(std::string("Result of '") + text + "' should be > 0");
			return -1;
		}
	} else {
		fail(std::string("Result of '") + text + "' should be Integer");
		return -1;
	}
}

__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<SuperColliderEngine>("sc");
	addScriptEngine<SuperColliderEngine>("scd");
}
