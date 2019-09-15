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

	struct ProcessArgs {
		float sampleRate;
		float sampleTime;
	};
	/** Calls the script's process() method.
	Return nonzero if failure, and set error message with setMessage().
	*/
	virtual int process(ProcessArgs& block) {return 0;}

	// Communication with Prototype module.
	// These cannot be called from your constructor, so initialize your engine in the run() method.
	void setMessage(const std::string& message);
	int getFrameDivider();
	void setFrameDivider(int frameDivider);
	float getInput(int index);
	void setOutput(int index, float voltage);
	float getKnob(int index);
	bool getSwitch(int index);
	void setLight(int index, int color, float brightness);
	void setSwitchLight(int index, int color, float brightness);
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
