#include "ScriptEngine.hpp"
#include <luajit-2.0/lua.hpp>


struct LuaJITEngine : ScriptEngine {
	lua_State* L = NULL;

	~LuaJITEngine() {
		if (L)
			lua_close(L);
	}

	std::string getEngineName() override {
		return "Lua";
	}

	int run(const std::string& path, const std::string& script) override {
		ProcessBlock* block = getProcessBlock();

		L = luaL_newstate();
		if (!L) {
			display("Could not create LuaJIT context");
			return -1;
		}

		// Import a subset of the standard library
		luaopen_base(L);
		luaopen_string(L);
		luaopen_table(L);
		luaopen_math(L);

		// Set user pointer
		lua_pushlightuserdata(L, this);
		lua_setglobal(L, "_engine");

		// Set global functions
		lua_pushcfunction(L, native_print);
		lua_setglobal(L, "print");

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

		// Compile script
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
		lua_newtable(L);
		{
			// inputs
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_newtable(L);
				for (int j = 0; j < block->bufferSize; j++) {
					lua_pushnumber(L, 0.0);
					lua_rawseti(L, -2, j + 1);
				}
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "inputs");
			// outputs
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_newtable(L);
				for (int j = 0; j < block->bufferSize; j++) {
					lua_pushnumber(L, 0.0);
					lua_rawseti(L, -2, j + 1);
				}
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "outputs");
			// knobs
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_pushnumber(L, 0.0);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "knobs");
			// switches
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_pushboolean(L, false);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "switches");
			// lights
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_newtable(L);
				for (int c = 0; c < 3; c++) {
					lua_pushnumber(L, 0.0);
					lua_rawseti(L, -2, c + 1);
				}
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "lights");
			// switchLights
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_newtable(L);
				for (int c = 0; c < 3; c++) {
					lua_pushnumber(L, 0.0);
					lua_rawseti(L, -2, c + 1);
				}
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "switchLights");
		}

		return 0;
	}

	int process() override {
		ProcessBlock* block = getProcessBlock();

		// Set block
		{
			// sampleRate
			lua_pushnumber(L, block->sampleRate);
			lua_setfield(L, -2, "sampleRate");

			// sampleTime
			lua_pushnumber(L, block->sampleTime);
			lua_setfield(L, -2, "sampleTime");

			// bufferSize
			lua_pushinteger(L, block->bufferSize);
			lua_setfield(L, -2, "bufferSize");

			// inputs
			lua_getfield(L, -1, "inputs");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_rawgeti(L, -1, i + 1);
				for (int j = 0; j < block->bufferSize; j++) {
					lua_pushnumber(L, block->inputs[i][j]);
					lua_rawseti(L, -2, j + 1);
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			// knobs
			lua_getfield(L, -1, "knobs");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_pushnumber(L, block->knobs[i]);
				lua_rawseti(L, -2, i + 1);
			}
			lua_pop(L, 1);
			// switches
			lua_getfield(L, -1, "switches");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_pushboolean(L, block->switches[i]);
				lua_rawseti(L, -2, i + 1);
			}
			lua_pop(L, 1);
		}

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

		// Get block
		{
			// outputs
			lua_getfield(L, -1, "outputs");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_rawgeti(L, -1, i + 1);
				for (int j = 0; j < block->bufferSize; j++) {
					lua_rawgeti(L, -1, j + 1);
					block->outputs[i][j] = lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			// knobs
			lua_getfield(L, -1, "knobs");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_rawgeti(L, -1, i + 1);
				block->knobs[i] = lua_tonumber(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			// lights
			lua_getfield(L, -1, "lights");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_rawgeti(L, -1, i + 1);
				for (int c = 0; c < 3; c++) {
					lua_rawgeti(L, -1, c + 1);
					block->lights[i][c] = lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			// switchLights
			lua_getfield(L, -1, "switchLights");
			for (int i = 0; i < NUM_ROWS; i++) {
				lua_rawgeti(L, -1, i + 1);
				for (int c = 0; c < 3; c++) {
					lua_rawgeti(L, -1, c + 1);
					block->switchLights[i][c] = lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}

		return 0;
	}

	static LuaJITEngine* getEngine(lua_State* L) {
		lua_getglobal(L, "_engine");
		LuaJITEngine* engine = (LuaJITEngine*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return engine;
	}

	static int native_print(lua_State* L) {
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, 1);
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, 1);
		INFO("LuaJIT: %s", s);
		return 0;
	}

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


ScriptEngine* createLuaJITEngine() {
	return new LuaJITEngine;
}