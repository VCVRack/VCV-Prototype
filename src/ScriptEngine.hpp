#pragma once
#include <rack.hpp>


static const int NUM_ROWS = 6;
static const int MAX_BUFFER_SIZE = 4096;


struct Prototype;


struct ProcessBlock {
	float sampleRate = 0.f;
	float sampleTime = 0.f;
	int bufferSize = 1;
	float inputs[NUM_ROWS][MAX_BUFFER_SIZE] = {};
	float outputs[NUM_ROWS][MAX_BUFFER_SIZE] = {};
	float knobs[NUM_ROWS] = {};
	bool switches[NUM_ROWS] = {};
	float lights[NUM_ROWS][3] = {};
	float switchLights[NUM_ROWS][3] = {};
};


struct ScriptEngine {
	// Virtual methods for subclasses
	virtual ~ScriptEngine() {}
	virtual std::string getEngineName() {return "";}
	/** Executes the script.
	Return nonzero if failure, and set error message with setMessage().
	Called only once per instance.
	*/
	virtual int run(const std::string& path, const std::string& script) {return 0;}

	/** Calls the script's process() method.
	Return nonzero if failure, and set error message with setMessage().
	*/
	virtual int process() {return 0;}

	// Communication with Prototype module.
	// These cannot be called from your constructor, so initialize your engine in the run() method.
	void display(const std::string& message);
	void setFrameDivider(int frameDivider);
	void setBufferSize(int bufferSize);
	ProcessBlock* getProcessBlock();
	// private
	Prototype* module = NULL;
};


struct ScriptEngineFactory {
	virtual ScriptEngine* createScriptEngine() = 0;
};
extern std::map<std::string, ScriptEngineFactory*> scriptEngineFactories;

/** Called from functions with
__attribute__((constructor(1000)))
*/
template<typename TScriptEngine>
void addScriptEngine(std::string extension) {
	struct TScriptEngineFactory : ScriptEngineFactory {
		ScriptEngine* createScriptEngine() {
			return new TScriptEngine;
		}
	};
	scriptEngineFactories[extension] = new TScriptEngineFactory;
}
