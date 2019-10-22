#include "ScriptEngine.hpp"
#include <quickjs/quickjs.h>

static JSClassID QuickJSEngineClass;

static std::string ErrorToString(JSContext *ctx) {
  JSValue exception_val, val;
  const char *stack;
  const char *str;
  bool is_error;
  std::string ret;
  size_t s1, s2;

  exception_val = JS_GetException(ctx);
  is_error = JS_IsError(ctx, exception_val);
  str = JS_ToCStringLen(ctx, &s1, exception_val);

  if (!str) {
    return std::string("error thrown but no error message");
  }

  if (!is_error) {
    ret = std::string("Throw:\n") + std::string(str);
  } else {
    val = JS_GetPropertyStr(ctx, exception_val, "stack");

    if (!JS_IsUndefined(val)) {
        stack = JS_ToCStringLen(ctx, &s2, val);
        ret = std::string(str) + std::string("\n") + std::string(stack);
        JS_FreeCString(ctx, stack);
    }
    JS_FreeValue(ctx, val);
  }

  JS_FreeCString(ctx, str);
  JS_FreeValue(ctx, exception_val);

  return ret;
}


struct QuickJSEngine : ScriptEngine {
  JSRuntime *rt = NULL;
	JSContext *ctx = NULL;

  QuickJSEngine() {
    rt = JS_NewRuntime();
  }

	~QuickJSEngine() {
    if (ctx) {
      JS_FreeContext(ctx);
    }
    if (rt) {
      JS_FreeRuntime(rt);
    }
	}

	std::string getEngineName() override {
		return "JavaScript";
	}

	int run(const std::string& path, const std::string& script) override {
		assert(!ctx);
		// Create quickjs context
		ctx = JS_NewContext(rt);
		if (!ctx) {
			display("Could not create QuickJS context");
			return -1;
		}

		// Initialize globals
    // user pointer
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue handle = JS_NewObjectClass(ctx, QuickJSEngineClass);
    JS_SetOpaque(handle, this);
    JS_SetPropertyStr(ctx, global_obj, "p", handle);

    // console
    JSValue console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log",
                      JS_NewCFunction(ctx, native_console_log, "log", 1));
    JS_SetPropertyStr(ctx, console, "info",
                      JS_NewCFunction(ctx, native_console_info, "info", 1));
    JS_SetPropertyStr(ctx, console, "debug",
                      JS_NewCFunction(ctx, native_console_debug, "debug", 1));
    JS_SetPropertyStr(ctx, console, "warn",
                      JS_NewCFunction(ctx, native_console_warn, "warn", 1));
    JS_SetPropertyStr(ctx, global_obj, "console", console);

    // display
    JS_SetPropertyStr(ctx, global_obj, "display",
                      JS_NewCFunction(ctx, native_display, "display", 1));

		// config: Set defaults
    JSValue config = JS_NewObject(ctx);
    // frameDivider
    JSValue fd =JS_NewInt32(ctx, 32);
    JS_SetPropertyStr(ctx, config, "frameDivider", fd);
    JS_FreeValue(ctx, fd);
    // bufferSize
    JSValue bs = JS_NewInt32(ctx, 1);
    JS_SetPropertyStr(ctx, config, "bufferSize", bs);
    JS_SetPropertyStr(ctx, global_obj, "config", config);
    JS_FreeValue(ctx, bs);

		// Compile string
    JSValue val = JS_Eval(ctx, script.c_str(), script.size(), path.c_str(), 0);
    if (JS_IsException(val)) {
      display(ErrorToString(ctx));
      JS_FreeValue(ctx, val);
      JS_FreeValue(ctx, global_obj);

      return -1;
    }

    ProcessBlock* block = getProcessBlock();
    // config: Read values
    config = JS_GetPropertyStr(ctx, global_obj, "config");
    {
      // frameDivider
      JSValue divider = JS_GetPropertyStr(ctx, config, "frameDivider");
      int32_t dividerValue;
      if (JS_ToInt32(ctx, &dividerValue, divider) == 0) {
        setFrameDivider(dividerValue);
      }

      // bufferSize
      JSValue buffer = JS_GetPropertyStr(ctx, config, "bufferSize");
      int32_t bufferValue;
      if (JS_ToInt32(ctx, &bufferValue, buffer) == 0) {
        setBufferSize(bufferValue);
      }

      JS_FreeValue(ctx, config);
    }

    // block
    JSValue blockIdx = JS_NewObject(ctx);
    {
      // inputs
      JSValue arr = JS_NewArray(ctx);
      for (int i = 0; i < NUM_ROWS; i++) {
        JSValue buffer = JS_NewArrayBuffer(ctx, (uint8_t *) block->inputs[i], sizeof(float) * block->bufferSize, NULL, NULL, true);
        if (JS_SetPropertyUint32(ctx, arr, i, buffer) < 0) {
          WARN("Unable to set property %d of inputs array", i);
        }
      }
      JS_SetPropertyStr(ctx, blockIdx, "inputs", arr);

      // outputs
      arr = JS_NewArray(ctx);
      for (int i = 0; i < NUM_ROWS; i++) {
        JSValue buffer = JS_NewArrayBuffer(ctx, (uint8_t *) block->outputs[i], sizeof(float) * block->bufferSize, NULL, NULL, true);
        if (JS_SetPropertyUint32(ctx, arr, i, buffer) < 0) {
          WARN("Unable to set property %d of outputs array", i);
        }
      }
      JS_SetPropertyStr(ctx, blockIdx, "outputs", arr);

      // knobs
      JSValue knobsIdx = JS_NewArrayBuffer(ctx, (uint8_t *) &block->knobs, sizeof(float) * NUM_ROWS, NULL, NULL, true);
      JS_SetPropertyStr(ctx, blockIdx, "knobs", knobsIdx);

      // switches
      JSValue switchesIdx = JS_NewArrayBuffer(ctx, (uint8_t *) &block->switches, sizeof(bool) * NUM_ROWS, NULL, NULL, true);
      JS_SetPropertyStr(ctx, blockIdx, "switches", switchesIdx);

      // lights
      arr = JS_NewArray(ctx);
      for (int i = 0; i < NUM_ROWS; i++) {
        JSValue buffer = JS_NewArrayBuffer(ctx, (uint8_t *) &block->lights[i], sizeof(float) * 3, NULL, NULL, true);
        if (JS_SetPropertyUint32(ctx, arr, i, buffer) < 0) {
          WARN("Unable to set property %d of lights array", i);
        }
      }
      JS_SetPropertyStr(ctx, blockIdx, "lights", arr);

      // switchlights
      arr = JS_NewArray(ctx);
      for (int i = 0; i < NUM_ROWS; i++) {
        JSValue buffer = JS_NewArrayBuffer(ctx, (uint8_t *) &block->switchLights[i], sizeof(float) * 3, NULL, NULL, true);
        if (JS_SetPropertyUint32(ctx, arr, i, buffer) < 0) {
          WARN("Unable to set property %d of switchLights array", i);
        }
      }
      JS_SetPropertyStr(ctx, blockIdx, "switchLights", arr);
    }

    JS_SetPropertyStr(ctx, global_obj, "block", blockIdx);

    // this is a horrible hack to get QuickJS to allocate the correct types
    static const std::string updateTypes = R"(
    for (var i = 0; i < 6; i++) {
      block.inputs[i] = new Float32Array(block.inputs[i]);
      block.outputs[i] = new Float32Array(block.outputs[i]);
      block.lights[i] = new Float32Array(block.lights[i]);
      block.switchLights[i] = new Float32Array(block.switchLights[i]);
    }
    block.knobs = new Float32Array(block.knobs);
    block.switches = new Uint8Array(block.switches);
    )";

    JSValue hack = JS_Eval(ctx, updateTypes.c_str(), updateTypes.size(), "QuickJS Hack", 0);
    if (JS_IsException(hack)) {
      std::string errorString = ErrorToString(ctx);
      WARN("QuickJS: %s", errorString.c_str());
      display(errorString.c_str());
    }

    JS_FreeValue(ctx, hack);

    JS_FreeValue(ctx, val);
    JS_FreeValue(ctx, global_obj);

    if (JS_IsException(val)) {
      std::string errorString = ErrorToString(ctx);
      WARN("QuickJS: %s", errorString.c_str());
      display(errorString.c_str());

      JS_FreeValue(ctx, val);
      JS_FreeValue(ctx, blockIdx);
      JS_FreeValue(ctx, global_obj);

      return -1;
    }

		return 0;
	}

	int process() override {
    // global object
    JSValue global_obj = JS_GetGlobalObject(ctx);

    // block
    ProcessBlock* block = getProcessBlock();
    JSValue blockIdx = JS_GetPropertyStr(ctx, global_obj, "block");
    {
      // sampleRate
      JSValue sampleRate = JS_NewFloat64(ctx, (double) block->sampleRate);
      JS_SetPropertyStr(ctx, blockIdx, "sampleRate", sampleRate);

      // sampleTime
      JSValue sampleTime = JS_NewFloat64(ctx, (double) block->sampleTime);
      JS_SetPropertyStr(ctx, blockIdx, "sampleTime", sampleTime);

      // bufferSize
      JSValue bufferSize = JS_NewInt32(ctx, (double) block->bufferSize);
      JS_SetPropertyStr(ctx, blockIdx, "bufferSize", bufferSize);
    }

    JSValue process = JS_GetPropertyStr(ctx, global_obj, "process");

    JSValue val = JS_Call(ctx, process, JS_UNDEFINED, 1, &blockIdx);

    if (JS_IsException(val)) {
      std::string errorString = ErrorToString(ctx);
      WARN("QuickJS: %s", errorString.c_str());
      display(errorString.c_str());

      JS_FreeValue(ctx, val);
      JS_FreeValue(ctx, process);
      JS_FreeValue(ctx, blockIdx);
      JS_FreeValue(ctx, global_obj);

      return -1;
    }

    JS_FreeValue(ctx, val);
    JS_FreeValue(ctx, process);
    JS_FreeValue(ctx, global_obj);

		return 0;
	}

  static QuickJSEngine* getQuickJSEngine(JSContext* ctx) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue p = JS_GetPropertyStr(ctx, global_obj, "p");
		QuickJSEngine* engine = (QuickJSEngine*) JS_GetOpaque(p, QuickJSEngineClass);

    JS_FreeValue(ctx, p);
    JS_FreeValue(ctx, global_obj);

		return engine;
	}

	static JSValue native_console_log(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    if (argc) {
      const char *s = JS_ToCString(ctx, argv[0]);
      INFO("QuickJS: %s", s);
    }
		return JS_UNDEFINED;
	}
  static JSValue native_console_debug(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    if (argc) {
      const char *s = JS_ToCString(ctx, argv[0]);
      DEBUG("QuickJS: %s", s);
    }
		return JS_UNDEFINED;
	}
  static JSValue native_console_info(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    if (argc) {
      const char *s = JS_ToCString(ctx, argv[0]);
      INFO("QuickJS: %s", s);
    }
		return JS_UNDEFINED;
	}
  static JSValue native_console_warn(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    if (argc) {
      const char *s = JS_ToCString(ctx, argv[0]);
      WARN("QuickJS: %s", s);
    }
		return JS_UNDEFINED;
	}
	static JSValue native_display(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    if (argc) {
      const char *s = JS_ToCString(ctx, argv[0]);
  		getQuickJSEngine(ctx)->display(s);
    }
		return JS_UNDEFINED;
	}
};


__attribute__((constructor(1000)))
static void constructor() {
  addScriptEngine<QuickJSEngine>("js");
}
