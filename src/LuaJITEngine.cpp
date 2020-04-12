#include "ScriptEngine.hpp"
#include <luajit-2.0/lua.hpp>


struct LuaJITEngine : ScriptEngine {
	lua_State* L = NULL;

	struct SafeArray {
		void* p;
		size_t len;
	};

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
		luaopen_bit(L);

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

		// FloatArray metatable
		lua_newtable(L);
		{
			// __index
			lua_pushcfunction(L, native_FloatArray_index);
			lua_setfield(L, -2, "__index");
			// __newindex
			lua_pushcfunction(L, native_FloatArray_newindex);
			lua_setfield(L, -2, "__newindex");
			// __len
			lua_pushcfunction(L, native_FloatArray_len);
			lua_setfield(L, -2, "__len");
		}
		lua_setglobal(L, "FloatArray");

		// BoolArray metatable
		lua_newtable(L);
		{
			// __index
			lua_pushcfunction(L, native_BoolArray_index);
			lua_setfield(L, -2, "__index");
			// __newindex
			lua_pushcfunction(L, native_BoolArray_newindex);
			lua_setfield(L, -2, "__newindex");
			// __len
			lua_pushcfunction(L, native_BoolArray_len);
			lua_setfield(L, -2, "__len");
		}
		lua_setglobal(L, "BoolArray");

		// Create block object
		lua_newtable(L);
		{
			// inputs
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				SafeArray* input = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
				input->p = &block->inputs[i];
				input->len = block->bufferSize;
				lua_getglobal(L, "FloatArray");
				lua_setmetatable(L, -2);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "inputs");

			// outputs
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				SafeArray* output = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
				output->p = &block->outputs[i];
				output->len = block->bufferSize;
				lua_getglobal(L, "FloatArray");
				lua_setmetatable(L, -2);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "outputs");

			// knobs
			SafeArray* knobs = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
			knobs->p = &block->knobs;
			knobs->len = 6;
			lua_getglobal(L, "FloatArray");
			lua_setmetatable(L, -2);
			lua_setfield(L, -2, "knobs");

			// switches
			SafeArray* switches = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
			switches->p = &block->switches;
			switches->len = 6;
			lua_getglobal(L, "BoolArray");
			lua_setmetatable(L, -2);
			lua_setfield(L, -2, "switches");

			// lights
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				SafeArray* light = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
				light->p = &block->lights[i];
				light->len = 3;
				lua_getglobal(L, "FloatArray");
				lua_setmetatable(L, -2);
				lua_rawseti(L, -2, i + 1);
			}
			lua_setfield(L, -2, "lights");

			// switchLights
			lua_newtable(L);
			for (int i = 0; i < NUM_ROWS; i++) {
				SafeArray* switchLight = (SafeArray*) lua_newuserdata(L, sizeof(SafeArray));
				switchLight->p = &block->switchLights[i];
				switchLight->len = 3;
				lua_getglobal(L, "FloatArray");
				lua_setmetatable(L, -2);
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

	static int native_FloatArray_index(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		float* data = (float*) a->p;
		size_t index = lua_tointeger(L, 2) - 1;
		if (index >= a->len) {
			lua_pushstring(L, "Array out of bounds");
			lua_error(L);
		}
		lua_pushnumber(L, data[index]);
		return 1;
	}
	static int native_FloatArray_newindex(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		float* data = (float*) a->p;
		size_t index = lua_tointeger(L, 2) - 1;
		if (index >= a->len) {
			lua_pushstring(L, "Array out of bounds");
			lua_error(L);
		}
		data[index] = lua_tonumber(L, 3);
		return 0;
	}
	static int native_FloatArray_len(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		lua_pushinteger(L, a->len);
		return 1;
	}

	static int native_BoolArray_index(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		bool* data = (bool*) a->p;
		size_t index = lua_tointeger(L, 2) - 1;
		if (index >= a->len) {
			lua_pushstring(L, "Array out of bounds");
			lua_error(L);
		}
		lua_pushboolean(L, data[index]);
		return 1;
	}
	static int native_BoolArray_newindex(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		bool* data = (bool*) a->p;
		size_t index = lua_tointeger(L, 2) - 1;
		if (index >= a->len) {
			lua_pushstring(L, "Array out of bounds");
			lua_error(L);
		}
		data[index] = lua_toboolean(L, 3);
		return 0;
	}
	static int native_BoolArray_len(lua_State* L) {
		SafeArray* a = (SafeArray*) lua_touserdata(L, 1);
		lua_pushinteger(L, a->len);
		return 1;
	}
};


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<LuaJITEngine>("lua");
}
