#include "ScriptEngine.hpp"
#include <luajit-2.0/lua.hpp>

static const luaL_Reg lj_lib_load[] = {
    { "",              luaopen_base },
    { LUA_LOADLIBNAME, luaopen_package },
    { LUA_TABLIBNAME,  luaopen_table },
    { LUA_STRLIBNAME,  luaopen_string },
    { LUA_MATHLIBNAME, luaopen_math },
    { LUA_BITLIBNAME,  luaopen_bit },
    { LUA_JITLIBNAME,  luaopen_jit },
    { LUA_FFILIBNAME,  luaopen_ffi },
    { NULL,            NULL }
};

LUALIB_API void custom_openlibs(lua_State *L)
{
    const luaL_Reg *lib;
    for (lib = lj_lib_load; lib->func; lib++) {
    	lua_pushcfunction(L, lib->func);
    	lua_pushstring(L, lib->name);
    	lua_call(L, 1, 0);
  	}
  	lua_pop(L, 1);
}

struct LuaJITEngine : ScriptEngine {
	lua_State* L = NULL;

	// This is a mirror of ProcessBlock that we are going to use
	// to provide 1-based indices within the Lua VM
	struct LuaProcessBlock {
		float sampleRate;
		float sampleTime;
		int bufferSize;
		float* inputs[NUM_ROWS+1];
		float* outputs[NUM_ROWS+1];
		float* knobs;
		bool* switches;
		float* lights[NUM_ROWS+1];
		float* switchLights[NUM_ROWS+1];
	};

	LuaProcessBlock block;

	~LuaJITEngine() {
		if (L)
			lua_close(L);
	}

	std::string getEngineName() override {
		return "Lua";
	}

	int run(const std::string& path, const std::string& script) override {
		ProcessBlock* original_block = getProcessBlock();

		block.sampleRate = original_block->sampleRate;
		block.sampleTime = original_block->sampleTime;
		block.bufferSize = original_block->bufferSize;

		// Initialize all the pointers with an offset of -1
		block.knobs = (float*)&(original_block->knobs[-1]);
		block.switches = (bool*)&(original_block->switches[-1]);

		for(int i = 1; i < NUM_ROWS + 1; i++) {
			block.inputs[i] = (float*)&(original_block->inputs[i-1][-1]);
			block.outputs[i] = (float*)&(original_block->outputs[i-1][-1]);
			block.lights[i] = (float*)&(original_block->lights[i-1][-1]);
			block.switchLights[i] = (float*)&(original_block->switchLights[i-1][-1]);
		}

		L = luaL_newstate();
		if (!L) {
			display("Could not create LuaJIT context");
			return -1;
		}

		// Import a subset of the standard library
		custom_openlibs(L);

		// Set user pointer
		lua_pushlightuserdata(L, this);
		lua_setglobal(L, "_engine");

		// Set global functions
		// lua_pushcfunction(L, native_print);
		// lua_setglobal(L, "print");

		lua_pushcfunction(L, native_display);
		lua_setglobal(L, "display");

		// Set config
		lua_newtable(L);
		{
			// frameDivider
			lua_pushinteger(L, 32);
			lua_setfield(L, -2, "frameDivider");
			// bufferSize
			lua_pushinteger(L, 1);
			lua_setfield(L, -2, "bufferSize");
		}
		lua_setglobal(L, "config");

		// Loads the FFI auxiliary functions.
		std::stringstream ffi_stream;
		ffi_stream
		<< "local ffi = require('ffi')" << std::endl
		// Describes the struct 'LuaProcessBlock' that way LuaJIT knows how to access the data
		<< "ffi.cdef[[" << std::endl
		<< "struct LuaProcessBlock {" << std::endl
		<< "float sampleRate;" << std::endl
		<< "float sampleTime;" << std::endl
		<< "int bufferSize;" << std::endl
		<< "float *inputs[" << NUM_ROWS + 1 << "];" << std::endl
		<< "float *outputs[" << NUM_ROWS + 1 << "];" << std::endl
		<< "float *knobs;" << std::endl
		<< "bool *switches;" << std::endl
		<< "float *lights[" << NUM_ROWS + 1 << "];" << std::endl
		<< "float *switchLights[" << NUM_ROWS + 1 << "];" << std::endl
		<<"};]]" << std::endl
		// Declares the function 'castBlock' used to transform the 'block' pointer into a LuaJIT cdata
		<< "function castBlock(b) return ffi.cast('struct LuaProcessBlock*', b) end";
		std::string ffi_script = ffi_stream.str();

		// Compile the ffi script
		if (luaL_loadbuffer(L, ffi_script.c_str(), ffi_script.size(), "ffi_script.lua")) {
			const char* s = lua_tostring(L, -1);
			WARN("LuaJIT: %s", s);
			display(s);
			lua_pop(L, 1);
			return -1;
		}

		// Run the ffi script
		if (lua_pcall(L, 0, 0, 0)) {
			const char* s = lua_tostring(L, -1);
			WARN("LuaJIT: %s", s);
			display(s);
			lua_pop(L, 1);
			return -1;
		}

		// Compile user script
		if (luaL_loadbuffer(L, script.c_str(), script.size(), path.c_str())) {
			const char* s = lua_tostring(L, -1);
			WARN("LuaJIT: %s", s);
			display(s);
			lua_pop(L, 1);
			return -1;
		}

		// Run script
		if (lua_pcall(L, 0, 0, 0)) {
			const char* s = lua_tostring(L, -1);
			WARN("LuaJIT: %s", s);
			display(s);
			lua_pop(L, 1);
			return -1;
		}

		// Get config
		lua_getglobal(L, "config");
		{
			// frameDivider
			lua_getfield(L, -1, "frameDivider");
			int frameDivider = lua_tointeger(L, -1);
			setFrameDivider(frameDivider);
			lua_pop(L, 1);
			// bufferSize
			lua_getfield(L, -1, "bufferSize");
			int bufferSize = lua_tointeger(L, -1);
			setBufferSize(bufferSize);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		// Get process function
		lua_getglobal(L, "process");
		if (!lua_isfunction(L, -1)) {
			display("No process() function");
			return -1;
		}

		// Create block object
        lua_getglobal(L, "castBlock");
        lua_pushlightuserdata(L, (void *)&block);
        if (lua_pcall(L, 1, 1, 0) != 0)
            printf("error running function 'castBlock': %s", lua_tostring(L, -1));

		return 0;
	}

	int process() override {
		ProcessBlock* original_block = getProcessBlock();

		// Update the values of the block (the pointers should not change)
		block.sampleRate = original_block->sampleRate;
		block.sampleTime = original_block->sampleTime;
		block.bufferSize = original_block->bufferSize;

		// Duplicate process function
		lua_pushvalue(L, -2);
		// Duplicate block
		lua_pushvalue(L, -2);
		// Call process function
		if (lua_pcall(L, 1, 0, 0)) {
			const char* err = lua_tostring(L, -1);
			WARN("LuaJIT: %s", err);
			display(err);
			return -1;
		}

		return 0;
	}

	static LuaJITEngine* getEngine(lua_State* L) {
		lua_getglobal(L, "_engine");
		LuaJITEngine* engine = (LuaJITEngine*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return engine;
	}

	// static int native_print(lua_State* L) {
	// 	lua_getglobal(L, "tostring");
	// 	lua_pushvalue(L, 1);
	// 	lua_call(L, 1, 1);
	// 	const char* s = lua_tostring(L, 1);
	// 	INFO("LuaJIT: %s", s);
	// 	return 0;
	// }

	static int native_display(lua_State* L) {
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, 1);
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, -1);
		if (!s)
			s = "(null)";
		getEngine(L)->display(s);
		return 0;
	}
};


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<LuaJITEngine>("lua");
}
