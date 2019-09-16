#include "ScriptEngine.hpp"
#include <lua.hpp>


struct LuaJitEngine : ScriptEngine {
	lua_State* L = NULL;

	~LuaJitEngine() {
		if (L)
			lua_close(L);
	}

	std::string getEngineName() override {
		return "Lua";
	}

	int run(const std::string& path, const std::string& script) override {
		L = luaL_newstate();
		if (!L) {
			setMessage("Could not create LuaJIT context");
			return -1;
		}

		// Use only "safe" libraries
		luaopen_math(L);
		luaopen_string(L);
		luaopen_table(L);

		if (luaL_loadbuffer(L, script.c_str(), script.size(), path.c_str())) {
			const char* s = lua_tostring(L, -1);
			rack::WARN("LuaJIT: %s", s);
			setMessage(s);
			lua_pop(L, 1);
			return -1;
		}

		if (lua_pcall(L, 0, 0, 0)) {
			const char* s = lua_tostring(L, -1);
			rack::WARN("LuaJIT: %s", s);
			setMessage(s);
			lua_pop(L, 1);
			return -1;
		}

		return 0;
	}

	int process() override {
		return 0;
	}

	static int native_print(lua_State* L) {
		const char* s = lua_tostring(L, 1);
		rack::INFO("LuaJIT: %s", s);
		return 1;
	}
};


ScriptEngine* createLuaJitEngine() {
	return new LuaJitEngine;
}