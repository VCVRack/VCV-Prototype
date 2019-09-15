#include "ScriptEngine.hpp"
#include <duktape.h>


struct DuktapeEngine : ScriptEngine {
	duk_context* ctx = NULL;

	~DuktapeEngine() {
		if (ctx)
			duk_destroy_heap(ctx);
	}

	std::string getEngineName() override {
		return "JavaScript";
	}

	int run(const std::string& path, const std::string& script) override {
		assert(!ctx);
		// Create duktape context
		ctx = duk_create_heap_default();
		if (!ctx) {
			setMessage("Could not create duktape context");
			return -1;
		}

		// Initialize globals
		// user pointer
		duk_push_pointer(ctx, this);
		duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("p"));

		// console
		duk_idx_t consoleIdx = duk_push_object(ctx);
		{
			// log
			duk_push_c_function(ctx, native_console_log, 1);
			duk_put_prop_string(ctx, consoleIdx, "log");
			// info (alias for log)
			duk_push_c_function(ctx, native_console_log, 1);
			duk_put_prop_string(ctx, consoleIdx, "info");
			// debug
			duk_push_c_function(ctx, native_console_debug, 1);
			duk_put_prop_string(ctx, consoleIdx, "debug");
			// warn
			duk_push_c_function(ctx, native_console_warn, 1);
			duk_put_prop_string(ctx, consoleIdx, "warn");
		}
		duk_put_global_string(ctx, "console");

		// display
		duk_push_c_function(ctx, native_display, 1);
		duk_put_global_string(ctx, "display");

		// config
		duk_idx_t configIdx = duk_push_object(ctx);
		{
			// frameDivider
			duk_push_int(ctx, getFrameDivider());
			duk_put_prop_string(ctx, configIdx, "frameDivider");
			// bufferSize
			duk_push_int(ctx, getBufferSize());
			duk_put_prop_string(ctx, configIdx, "bufferSize");
		}
		duk_put_global_string(ctx, "config");

		// Compile string
		duk_push_string(ctx, path.c_str());
		if (duk_pcompile_lstring_filename(ctx, 0, script.c_str(), script.size()) != 0) {
			const char* s = duk_safe_to_string(ctx, -1);
			rack::WARN("duktape: %s", s);
			setMessage(s);
			duk_pop(ctx);
			return -1;
		}
		// Execute function
		if (duk_pcall(ctx, 0)) {
			const char* s = duk_safe_to_string(ctx, -1);
			rack::WARN("duktape: %s", s);
			setMessage(s);
			duk_pop(ctx);
			return -1;
		}
		// Ignore return value
		duk_pop(ctx);

		// Get config
		duk_get_global_string(ctx, "config");
		{
			// frameDivider
			duk_get_prop_string(ctx, -1, "frameDivider");
			setFrameDivider(duk_get_int(ctx, -1));
			duk_pop(ctx);
			// bufferSize
			duk_get_prop_string(ctx, -1, "bufferSize");
			setBufferSize(duk_get_int(ctx, -1));
			duk_pop(ctx);
		}
		duk_pop(ctx);

		// Keep process function on stack for faster calling
		duk_get_global_string(ctx, "process");
		if (!duk_is_function(ctx, -1)) {
			setMessage("No process() function");
			return -1;
		}

		// block (keep on stack)
		duk_idx_t blockIdx = duk_push_object(ctx);
		{
			int bufferSize = getBufferSize();

			// inputs
			duk_idx_t inputsIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_idx_t inputIdx = duk_push_array(ctx);
				for (int j = 0; j < bufferSize; j++) {
					duk_push_number(ctx, 0.0);
					duk_put_prop_index(ctx, inputIdx, j);
				}
				duk_put_prop_index(ctx, inputsIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "inputs");

			// outputs
			duk_idx_t outputsIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_idx_t outputIdx = duk_push_array(ctx);
				for (int j = 0; j < bufferSize; j++) {
					duk_push_number(ctx, 0.0);
					duk_put_prop_index(ctx, outputIdx, j);
				}
				duk_put_prop_index(ctx, outputsIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "outputs");

			// knobs
			duk_idx_t knobsIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_push_number(ctx, 0.0);
				duk_put_prop_index(ctx, knobsIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "knobs");

			// switches
			duk_idx_t switchesIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_push_number(ctx, 0.0);
				duk_put_prop_index(ctx, switchesIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "switches");

			// lights
			duk_idx_t lightsIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_idx_t lightIdx = duk_push_array(ctx);
				for (int c = 0; c < 3; c++) {
					duk_push_number(ctx, 0.0);
					duk_put_prop_index(ctx, lightIdx, c);
				}
				duk_put_prop_index(ctx, lightsIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "lights");

			// switchLights
			duk_idx_t switchLightsIdx = duk_push_array(ctx);
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_idx_t switchLightIdx = duk_push_array(ctx);
				for (int c = 0; c < 3; c++) {
					duk_push_number(ctx, 0.0);
					duk_put_prop_index(ctx, switchLightIdx, c);
				}
				duk_put_prop_index(ctx, switchLightsIdx, i);
			}
			duk_put_prop_string(ctx, blockIdx, "switchLights");
		}

		return 0;
	}

	int process(ProcessBlock& block) override {
		// block
		duk_idx_t blockIdx = duk_get_top(ctx) - 1;
		{
			// sampleRate
			duk_push_number(ctx, block.sampleRate);
			duk_put_prop_string(ctx, blockIdx, "sampleRate");

			// sampleTime
			duk_push_number(ctx, block.sampleTime);
			duk_put_prop_string(ctx, blockIdx, "sampleTime");

			// bufferSize
			duk_push_int(ctx, block.bufferSize);
			duk_put_prop_string(ctx, blockIdx, "bufferSize");

			// inputs
			duk_get_prop_string(ctx, blockIdx, "inputs");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_get_prop_index(ctx, -1, i);
				for (int j = 0; j < block.bufferSize; j++) {
					duk_push_number(ctx, block.inputs[i][j]);
					duk_put_prop_index(ctx, -2, j);
				}
				duk_pop(ctx);
			}
			duk_pop(ctx);

			// knobs
			duk_get_prop_string(ctx, blockIdx, "knobs");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_push_number(ctx, block.knobs[i]);
				duk_put_prop_index(ctx, -2, i);
			}
			duk_pop(ctx);

			// switches
			duk_get_prop_string(ctx, blockIdx, "switches");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_push_boolean(ctx, block.switches[i]);
				duk_put_prop_index(ctx, -2, i);
			}
			duk_pop(ctx);
		}

		// Duplicate process function
		duk_dup(ctx, -2);
		// Duplicate block object
		duk_dup(ctx, -2);
		// Call process function
		if (duk_pcall(ctx, 1)) {
			const char* s = duk_safe_to_string(ctx, -1);
			rack::WARN("duktape: %s", s);
			setMessage(s);
			duk_pop(ctx);
			return -1;
		}
		// return value
		duk_pop(ctx);

		// block
		{
			// outputs
			duk_get_prop_string(ctx, -1, "outputs");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_get_prop_index(ctx, -1, i);
				for (int j = 0; j < block.bufferSize; j++) {
					duk_get_prop_index(ctx, -1, j);
					block.outputs[i][j] = duk_get_number(ctx, -1);
					duk_pop(ctx);
				}
				duk_pop(ctx);
			}
			duk_pop(ctx);

			// lights
			duk_get_prop_string(ctx, -1, "lights");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_get_prop_index(ctx, -1, i);
				for (int c = 0; c < 3; c++) {
					duk_get_prop_index(ctx, -1, c);
					block.lights[i][c] = duk_get_number(ctx, -1);
					duk_pop(ctx);
				}
				duk_pop(ctx);
			}
			duk_pop(ctx);

			// switchLights
			duk_get_prop_string(ctx, -1, "switchLights");
			for (int i = 0; i < NUM_ROWS; i++) {
				duk_get_prop_index(ctx, -1, i);
				for (int c = 0; c < 3; c++) {
					duk_get_prop_index(ctx, -1, c);
					block.switchLights[i][c] = duk_get_number(ctx, -1);
					duk_pop(ctx);
				}
				duk_pop(ctx);
			}
			duk_pop(ctx);
		}

		return 0;
	}

	static DuktapeEngine* getDuktapeEngine(duk_context* ctx) {
		duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("p"));
		DuktapeEngine* engine = (DuktapeEngine*) duk_get_pointer(ctx, -1);
		duk_pop(ctx);
		return engine;
	}

	static duk_ret_t native_console_log(duk_context* ctx) {
		const char* s = duk_safe_to_string(ctx, -1);
		rack::INFO("VCV Prototype: %s", s);
		return 0;
	}
	static duk_ret_t native_console_debug(duk_context* ctx) {
		const char* s = duk_safe_to_string(ctx, -1);
		rack::DEBUG("VCV Prototype: %s", s);
		return 0;
	}
	static duk_ret_t native_console_warn(duk_context* ctx) {
		const char* s = duk_safe_to_string(ctx, -1);
		rack::WARN("VCV Prototype: %s", s);
		return 0;
	}
	static duk_ret_t native_display(duk_context* ctx) {
		const char* s = duk_safe_to_string(ctx, -1);
		getDuktapeEngine(ctx)->setMessage(s);
		return 0;
	}
};


ScriptEngine* createDuktapeEngine() {
	return new DuktapeEngine;
}