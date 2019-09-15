#include <rack.hpp>
#include <osdialog.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include "ScriptEngine.hpp"


using namespace rack;
Plugin* pluginInstance;


struct Prototype : Module {
	enum ParamIds {
		ENUMS(KNOB_PARAMS, NUM_ROWS),
		ENUMS(SWITCH_PARAMS, NUM_ROWS),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IN_INPUTS, NUM_ROWS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUTS, NUM_ROWS),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LIGHT_LIGHTS, NUM_ROWS * 3),
		ENUMS(SWITCH_LIGHTS, NUM_ROWS * 3),
		NUM_LIGHTS
	};

	std::string path;
	std::string script;
	std::string engineName;
	ScriptEngine* scriptEngine = NULL;
	std::mutex scriptMutex;
	std::string message;
	int frame = 0;
	int frameDivider;
	ScriptEngine::ProcessBlock block;
	int blockIndex = 0;

	Prototype() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < NUM_ROWS; i++)
			configParam(KNOB_PARAMS + i, 0.f, 1.f, 0.f, string::f("Knob %d", i + 1));
		for (int i = 0; i < NUM_ROWS; i++)
			configParam(SWITCH_PARAMS + i, 0.f, 1.f, 0.f, string::f("Switch %d", i + 1));

		clearScriptEngine();
	}

	~Prototype() {
		std::lock_guard<std::mutex> lock(scriptMutex);
		clearScriptEngine();
	}

	void onReset() override {
		setScriptString(path, script);
	}

	void process(const ProcessArgs& args) override {
		if (!scriptEngine)
			return;

		// Frame divider for reducing sample rate
		if (++frame < frameDivider)
			return;
		frame = 0;

		// Inputs
		for (int i = 0; i < NUM_ROWS; i++)
			block.inputs[i][blockIndex] = inputs[IN_INPUTS + i].getVoltage();
		// Outputs
		for (int i = 0; i < NUM_ROWS; i++)
			outputs[OUT_OUTPUTS + i].setVoltage(block.outputs[i][blockIndex]);

		// Block divider
		if (++blockIndex < block.bufferSize)
			return;
		blockIndex = 0;

		// Params
		for (int i = 0; i < NUM_ROWS; i++)
			block.knobs[i] = params[KNOB_PARAMS + i].getValue();
		for (int i = 0; i < NUM_ROWS; i++)
			block.switches[i] = (params[SWITCH_PARAMS + i].getValue() > 0.f);

		// Set other block parameters
		block.sampleRate = args.sampleRate;
		block.sampleTime = args.sampleTime;

		{
			std::lock_guard<std::mutex> lock(scriptMutex);
			// Check for certain inside the mutex
			if (scriptEngine) {
				if (scriptEngine->process(block)) {
					clearScriptEngine();
					return;
				}
			}
		}

		// Lights
		for (int i = 0; i < NUM_ROWS; i++)
			for (int c = 0; c < 3; c++)
				lights[LIGHT_LIGHTS + i * 3 + c].setBrightness(block.lights[i][c]);
		for (int i = 0; i < NUM_ROWS; i++)
			for (int c = 0; c < 3; c++)
				lights[SWITCH_LIGHTS + i * 3 + c].setBrightness(block.switchLights[i][c]);
	}

	void clearScriptEngine() {
		if (scriptEngine) {
			delete scriptEngine;
			scriptEngine = NULL;
		}
		// Reset outputs because they might hold old values
		for (int i = 0; i < NUM_ROWS; i++)
			outputs[OUT_OUTPUTS + i].setVoltage(0.f);
		for (int i = 0; i < NUM_ROWS; i++)
			lights[LIGHT_LIGHTS + i].setBrightness(0.f);
		for (int i = 0; i < NUM_ROWS; i++)
			lights[SWITCH_LIGHTS + i].setBrightness(0.f);
		std::memset(block.inputs, 0, sizeof(block.inputs));
		// Reset settings
		frameDivider = 32;
		block.bufferSize = 1;
	}

	void setScriptString(std::string path, std::string script) {
		std::lock_guard<std::mutex> lock(scriptMutex);
		message = "";
		this->path = "";
		this->script = "";
		this->engineName = "";
		clearScriptEngine();
		// Get ScriptEngine from path extension
		if (path == "") {
			return;
		}
		std::string ext = string::filenameExtension(string::filename(path));
		scriptEngine = createScriptEngine(ext);
		if (!scriptEngine) {
			message = string::f("No engine for .%s extension", ext.c_str());
			return;
		}
		this->path = path;
		this->script = script;
		this->engineName = scriptEngine->getEngineName();
		// Initialize ScriptEngine
		scriptEngine->module = this;
		if (scriptEngine->initialize()) {
			// Error message should have been set by ScriptEngine
			clearScriptEngine();
			return;
		}
		// Read file
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			file.open(this->path);
			std::stringstream buffer;
			buffer << file.rdbuf();
			this->script = buffer.str();
		}
		catch (const std::runtime_error& err) {
		}
		// Run script
		if (this->script == "") {
			message = "Could not load script.";
			clearScriptEngine();
			return;
		}
		if (scriptEngine->run(this->path, this->script)) {
			// Error message should have been set by ScriptEngine
			clearScriptEngine();
			return;
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "path", json_string(path.c_str()));
		json_object_set_new(rootJ, "script", json_stringn(script.data(), script.size()));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* pathJ = json_object_get(rootJ, "path");
		json_t* scriptJ = json_object_get(rootJ, "script");
		if (pathJ && scriptJ) {
			std::string path = json_string_value(pathJ);
			std::string script = std::string(json_string_value(scriptJ), json_string_length(scriptJ));
			setScriptString(path, script);
		}
	}
};


void ScriptEngine::setMessage(const std::string& message) {
	module->message = message;
}
int ScriptEngine::getFrameDivider() {
	return module->frameDivider;
}
void ScriptEngine::setFrameDivider(int frameDivider) {
	module->frameDivider = frameDivider;
}
int ScriptEngine::getBufferSize() {
	return module->block.bufferSize;
}
void ScriptEngine::setBufferSize(int bufferSize) {
	module->block.bufferSize = bufferSize;
}


struct FileChoice : LedDisplayChoice {
	Prototype* module;

	void step() override {
		if (module && module->engineName != "")
			text = module->engineName;
		else
			text = "Script";
		text += ": ";
		if (module && module->path != "")
			text += string::filename(module->path);
		else
			text += "(click to load)";
	}

	void onAction(const event::Action& e) override {
		std::string dir = asset::user("");
		char* pathC = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);
		if (!pathC) {
			return;
		}
		std::string path = pathC;
		std::free(pathC);

		module->setScriptString(path, "");
	}
};


struct MessageChoice : LedDisplayChoice {
	Prototype* module;

	void step() override {
		text = module ? module->message : "";
	}
};


struct PrototypeDisplay : LedDisplay {
	PrototypeDisplay() {
		box.size = mm2px(Vec(69.879, 27.335));
	}

	void setModule(Prototype* module) {
		FileChoice* fileChoice = new FileChoice;
		fileChoice->box.size.x = box.size.x;
		fileChoice->module = module;
		addChild(fileChoice);

		LedDisplaySeparator* fileSeparator = new LedDisplaySeparator;
		fileSeparator->box.size.x = box.size.x;
		fileSeparator->box.pos = fileChoice->box.getBottomLeft();
		addChild(fileSeparator);

		MessageChoice* messageChoice = new MessageChoice;
		messageChoice->box.pos = fileChoice->box.getBottomLeft();
		messageChoice->box.size.x = box.size.x;
		messageChoice->module = module;
		addChild(messageChoice);
	}
};


struct PrototypeWidget : ModuleWidget {
	PrototypeWidget(Prototype* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Prototype.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(8.099, 64.401)), module, Prototype::KNOB_PARAMS + 0));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(20.099, 64.401)), module, Prototype::KNOB_PARAMS + 1));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(32.099, 64.401)), module, Prototype::KNOB_PARAMS + 2));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(44.099, 64.401)), module, Prototype::KNOB_PARAMS + 3));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(56.099, 64.401)), module, Prototype::KNOB_PARAMS + 4));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(68.099, 64.401)), module, Prototype::KNOB_PARAMS + 5));
		addParam(createParamCentered<PB61303>(mm2px(Vec(8.099, 80.151)), module, Prototype::SWITCH_PARAMS + 0));
		addParam(createParamCentered<PB61303>(mm2px(Vec(20.099, 80.151)), module, Prototype::SWITCH_PARAMS + 1));
		addParam(createParamCentered<PB61303>(mm2px(Vec(32.099, 80.151)), module, Prototype::SWITCH_PARAMS + 2));
		addParam(createParamCentered<PB61303>(mm2px(Vec(44.099, 80.151)), module, Prototype::SWITCH_PARAMS + 3));
		addParam(createParamCentered<PB61303>(mm2px(Vec(56.099, 80.151)), module, Prototype::SWITCH_PARAMS + 4));
		addParam(createParamCentered<PB61303>(mm2px(Vec(68.099, 80.151)), module, Prototype::SWITCH_PARAMS + 5));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.099, 96.025)), module, Prototype::IN_INPUTS + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.099, 96.025)), module, Prototype::IN_INPUTS + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.099, 96.025)), module, Prototype::IN_INPUTS + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(44.099, 96.025)), module, Prototype::IN_INPUTS + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(56.099, 96.025)), module, Prototype::IN_INPUTS + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(68.099, 96.025)), module, Prototype::IN_INPUTS + 5));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.099, 112.25)), module, Prototype::OUT_OUTPUTS + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.099, 112.25)), module, Prototype::OUT_OUTPUTS + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(32.099, 112.25)), module, Prototype::OUT_OUTPUTS + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(44.099, 112.25)), module, Prototype::OUT_OUTPUTS + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(56.099, 112.25)), module, Prototype::OUT_OUTPUTS + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(68.099, 112.25)), module, Prototype::OUT_OUTPUTS + 5));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(8.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 0));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(20.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 1));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(32.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 2));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(44.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(56.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 4));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(68.099, 51.4)), module, Prototype::LIGHT_LIGHTS + 3 * 5));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(8.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 0));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(20.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 3 * 1));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(32.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 3 * 2));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(44.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 3 * 3));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(56.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 3 * 4));
		addChild(createLightCentered<PB61303Light<RedGreenBlueLight>>(mm2px(Vec(68.099, 80.151)), module, Prototype::SWITCH_LIGHTS + 3 * 5));

		PrototypeDisplay* display = createWidget<PrototypeDisplay>(mm2px(Vec(3.16, 14.837)));
		display->setModule(module);
		addChild(display);
	}
};


void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(createModel<Prototype, PrototypeWidget>("Prototype"));
}
