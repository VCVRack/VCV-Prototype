#include "ScriptEngine.hpp"

#include "lang/SC_LanguageClient.h"
#include "LangSource/SC_LanguageConfig.hpp"
#include "LangSource/SCBase.h"
#include "LangSource/VMGlobals.h"

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
	void interpret(const char *) noexcept;
	void evaluateProcessBlock(ProcessBlock* block) noexcept;
	int getFrameDivider() noexcept { return 1; } // getResultAsInt("^~vcv_frameDivider"); }
	int getBufferSize() noexcept { return 256; } // getResultAsInt("^~vcv_bufferSize"); }

	void postText(const char* str, size_t len) override;

	// No concept of flushing or stdout vs stderr
	void postFlush(const char* str, size_t len) override { postText(str, len); }
	void postError(const char* str, size_t len) override { postText(str, len); }
	void flush() override {}

private:
	// TODO
	int getResultAsInt(const char* text) noexcept;

	SuperColliderEngine* _engine;
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

		_client->evaluateProcessBlock(getProcessBlock());
		return 0;
	}

private:
	bool waitingOnClient() const noexcept { return !_clientRunning; }

	// TODO handle failure conditions
	void finishClientLoading() noexcept { _clientRunning = true; }

	std::unique_ptr<SC_VcvPrototypeClient> _client;
	std::thread _clientThread; // used only to start up client
	std::atomic_bool _clientRunning{false}; // set to true when client is ready to process data
};

// TODO
#define FAIL(_msg_) _engine->display(_msg_)

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
	_engine->display(std::string(str, len));
}

// TODO test code
static long long int gmax = 0;

void SC_VcvPrototypeClient::evaluateProcessBlock(ProcessBlock* block) noexcept {
	std::ostringstream builder;

	builder << "~vcv_process.value(" << block->knobs[0] << ");\n";

	// TIMING TODO test code
	auto start = std::chrono::high_resolution_clock::now();
	auto&& string = builder.str();
	interpret(string.c_str());
	auto end = std::chrono::high_resolution_clock::now();
	auto ticks = (end - start).count();
	if (gmax < ticks)
	{
		gmax = ticks;
		printf("MAX TIME %lld\n", ticks);
	}
	// END TIMING
}

int SC_VcvPrototypeClient::getResultAsInt(const char* text) noexcept {
	interpret(text);

	auto* resultSlot = &scGlobals()->result;
	if (IsInt(resultSlot)) {
		auto intResult = slotRawInt(resultSlot);
		printf("%s: %d\n", text, intResult);
		return intResult;
	}

	FAIL(std::string("Result of '") + text + "' was not int as expected");
	return -1;
}

__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<SuperColliderEngine>("sc");
	addScriptEngine<SuperColliderEngine>("scd");
}
