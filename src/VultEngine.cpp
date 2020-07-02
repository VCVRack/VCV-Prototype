#include "ScriptEngine.hpp"
#include "vultc.h"
#include <quickjs/quickjs.h>

/* The Vult engine relies on both QuickJS and LuaJIT.
 *
 * The compiler is written in OCaml but converted to JavaScript. The JavaScript
 * code is embedded as a string and executed by the QuickJs engine. The Vult
 * compiler generates Lua code that is executed by the LuaJIT engine.
 */

// Special version of createScriptEngine that only creates Lua engines
ScriptEngine* createLuaEngine() {
	auto it = scriptEngineFactories.find("lua");
	if (it == scriptEngineFactories.end())
		return NULL;
	return it->second->createScriptEngine();
}

struct VultEngine : ScriptEngine {

	// used to run the lua generated code
	ScriptEngine* luaEngine;

	// used to run the Vult compiler
	JSRuntime* rt = NULL;
	JSContext* ctx = NULL;

	VultEngine() {
		rt = JS_NewRuntime();
		// Create QuickJS context
		ctx = JS_NewContext(rt);
		if (!ctx) {
			display("Could not create QuickJS context");
			return;
		}

		JSValue global_obj = JS_GetGlobalObject(ctx);

		// Load the Vult compiler code
		JSValue val =
		  JS_Eval(ctx, (const char*)vultc_h, vultc_h_size, "vultc.js", 0);
		if (JS_IsException(val)) {
			display("Error loading the Vult compiler");
			JS_FreeValue(ctx, val);
			JS_FreeValue(ctx, global_obj);
			return;
		}
	}

	~VultEngine() {
		if (ctx) {
			JS_FreeContext(ctx);
		}
		if (rt) {
			JS_FreeRuntime(rt);
		}
	}

	std::string getEngineName() override {
		return "Vult";
	}

	int run(const std::string& path, const std::string& script) override {
		display("Loading...");

		JSValue global_obj = JS_GetGlobalObject(ctx);

		// Put the script text in the 'code' variable
		JSValue code = JS_NewString(ctx, script.c_str());
		JS_SetPropertyStr(ctx, global_obj, "code", code);

		// Put the script path in 'file' variable
		JSValue file = JS_NewString(ctx, path.c_str());
		JS_SetPropertyStr(ctx, global_obj, "file", file);

		display("Compiling...");
		// Call the Vult compiler to generate Lua code
		static const std::string testVult = R"(
    var result = vult.generateLua([{ file:file, code:code}],{ output:'Engine', template:'vcv-prototype'});)";

		JSValue compile =
		  JS_Eval(ctx, testVult.c_str(), testVult.size(), "Compile", 0);

		JS_FreeValue(ctx, code);
		JS_FreeValue(ctx, file);

		// If there are any internal errors, the execution could fail
		if (JS_IsException(compile)) {
			display("Fatal error in the Vult compiler");
			JS_FreeValue(ctx, global_obj);
			return -1;
		}

		// Retrive the variable 'result'
		JSValue result = JS_GetPropertyStr(ctx, global_obj, "result");
		// Get the first element of the 'result' array
		JSValue first = JS_GetPropertyUint32(ctx, result, 0);
		// Try to get the 'msg' field which is only present in error messages
		JSValue msg = JS_GetPropertyStr(ctx, first, "msg");
		// Display the error if any
		if (!JS_IsUndefined(msg)) {
			const char* text = JS_ToCString(ctx, msg);
			const char* row =
			  JS_ToCString(ctx, JS_GetPropertyStr(ctx, first, "line"));
			const char* col = JS_ToCString(ctx, JS_GetPropertyStr(ctx, first, "col"));
			// Compose the error message
			std::stringstream error;
			error << "line:" << row << ":" << col << ": " << text;
			WARN("Vult Error: %s", error.str().c_str());
			display(error.str().c_str());

			JS_FreeValue(ctx, result);
			JS_FreeValue(ctx, first);
			JS_FreeValue(ctx, msg);
			return -1;
		}
		// In case of no error, retrieve the generated code
		JSValue luacode = JS_GetPropertyStr(ctx, first, "code");
		std::string luacode_str(JS_ToCString(ctx, luacode));

		//WARN("Generated Code: %s", luacode_str.c_str());

		luaEngine = createLuaEngine();

		if (!luaEngine) {
			WARN("Could not create a Lua script engine");
			return -1;
		}

		luaEngine->module = this->module;

		display("Running...");

		JS_FreeValue(ctx, luacode);
		JS_FreeValue(ctx, first);
		JS_FreeValue(ctx, msg);
		JS_FreeValue(ctx, msg);
		JS_FreeValue(ctx, global_obj);

		return luaEngine->run(path, luacode_str);
	}

	int process() override {
		if (!luaEngine)
      return -1;
		return luaEngine->process();
	}
};

__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<VultEngine>("vult");
}
