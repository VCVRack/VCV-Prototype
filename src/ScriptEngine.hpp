#pragma once
#include <rack.hpp>


static const int NUM_ROWS = 6;


struct Prototype;


struct ScriptEngine {
	// Virtual methods for subclasses
	virtual ~ScriptEngine() {}
	virtual std::string getEngineName() {return "";}
	/** Executes the script.
	Return nonzero if failure, and set error message with setMessage().
	Called only once per instance.
	*/
	virtual int run(const std::string& path, const std::string& script) {return 0;}

	struct ProcessBlock {
		float sampleRate = 0.f;
		float sampleTime = 0.f;
		float inputs[NUM_ROWS] = {};
		float outputs[NUM_ROWS] = {};
		float knobs[NUM_ROWS] = {};
		bool switches[NUM_ROWS] = {};
		float lights[NUM_ROWS][3] = {};
		float switchLights[NUM_ROWS][3] = {};
	};
	/** Calls the script's process() method.
	Return nonzero if failure, and set error message with setMessage().
	*/
	virtual int process(ProcessBlock& block) {return 0;}

	// Communication with Prototype module.
	// These cannot be called from the constructor, so initialize in the run() method.
	void setMessage(const std::string& message);
	int getFrameDivider();
	void setFrameDivider(int frameDivider);
	// private
	Prototype* module;
};


// List of ScriptEngines

// Add your createMyEngine() function here.
ScriptEngine* createDuktapeEngine();

inline ScriptEngine* createScriptEngine(std::string ext) {
	ext = rack::string::lowercase(ext);
	if (ext == "js")
		return createDuktapeEngine();
	// Add your file extension check here.
	return NULL;
}
