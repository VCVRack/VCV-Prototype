/************************************************************************
 FAUST Architecture File
 Copyright (C) 2020 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 ************************************************************************/

#include "ScriptEngine.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>

#pragma GCC diagnostic push
#ifndef __clang__
	#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#include <faust/dsp/libfaust.h>
#include <faust/gui/DecoratorUI.h>
#include <faust/gui/ValueConverter.h>

#define kBufferSize 64

#ifdef INTERP
	#include <faust/dsp/interpreter-dsp.h>
#else
	#include <faust/dsp/llvm-dsp.h>
#endif
#pragma GCC diagnostic pop

extern rack::Plugin* pluginInstance;

// UI handler for switches, knobs and lights
struct RackUI : public GenericUI {
	typedef std::function<void(ProcessBlock* block)> updateFunction;

	std::vector<ConverterZoneControl*> fConverters;
	std::vector<updateFunction> fUpdateFunIn;
	std::vector<updateFunction> fUpdateFunOut;

	// For checkbox handling
	struct CheckBox {
		float fLastButton = 0.0f;
	};
	std::map <FAUSTFLOAT*, CheckBox> fCheckBoxes;

	std::string fKey, fValue, fScale;

	int getIndex(const std::string& value) {
		try {
			int index = stoi(value);
			if (index >= 0 && index <= NUM_ROWS) {
				return index;
			}
			else {
				WARN("ERROR : incorrect '%d' value", index);
				return -1;
			}
		}
		catch (std::invalid_argument& e) {
			return -1;
		}
	}

	RackUI(): fScale("lin")
	{}

	virtual ~RackUI() {
		for (auto& it : fConverters)
			delete it;
	}

	void addButton(const char* label, FAUSTFLOAT* zone) override {
		int index = getIndex(fValue);
		if (fKey == "switch" && (index != -1)) {
			fUpdateFunIn.push_back([ = ](ProcessBlock * block) {
				*zone = block->switches[index - 1];

				// And set the color to red when ON
				block->switchLights[index - 1][0] = *zone;
			});
		}
		fKey = fValue = "";
	}

	void addCheckButton(const char* label, FAUSTFLOAT* zone) override {
		int index = getIndex(fValue);
		if (fKey == "switch" && (index != -1)) {
			// Add a checkbox
			fCheckBoxes[zone] = CheckBox();
			// Update function
			fUpdateFunIn.push_back([ = ](ProcessBlock * block) {
				float button = block->switches[index - 1];
				// Detect upfront
				if (button == 1.0 && (button != fCheckBoxes[zone].fLastButton)) {
					// Switch button state
					*zone = !*zone;
					// And set the color to white when ON
					block->switchLights[index - 1][0] = *zone;
					block->switchLights[index - 1][1] = *zone;
					block->switchLights[index - 1][2] = *zone;
				}
				// Keep previous button state
				fCheckBoxes[zone].fLastButton = button;
			});
		}
		fKey = fValue = "";
	}

	void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override {
		addNumEntry(label, zone, init, min, max, step);
	}

	void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override {
		addNumEntry(label, zone, init, min, max, step);
	}

	void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) override {
		int index = getIndex(fValue);
		if (fKey == "knob" && (index != -1)) {
			ConverterZoneControl* converter;
			if (fScale == "log") {
				converter = new ConverterZoneControl(zone, new LogValueConverter(0., 1., min, max));
			}
			else if (fScale == "exp") {
				converter = new ConverterZoneControl(zone, new ExpValueConverter(0., 1., min, max));
			}
			else {
				converter = new ConverterZoneControl(zone, new LinearValueConverter(0., 1., min, max));
			}
			fUpdateFunIn.push_back([ = ](ProcessBlock * block) {
				converter->update(block->knobs[index - 1]);
			});
			fConverters.push_back(converter);
		}
		fScale = "lin";
		fKey = fValue = "";
	}

	void addBarGraph(FAUSTFLOAT* zone) {
		int index = getIndex(fValue);
		if ((fKey == "light_red") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->lights[index - 1][0] = *zone;
			});
		}
		else if ((fKey == "light_green") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->lights[index - 1][1] = *zone;
			});
		}
		else if ((fKey == "light_blue") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->lights[index - 1][2] = *zone;
			});
		}
		else if ((fKey == "switchlight_red") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->switchLights[index - 1][0] = *zone;
			});
		}
		else if ((fKey == "switchlight_green") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->switchLights[index - 1][1] = *zone;
			});
		}
		else if ((fKey == "switchlight_blue") && (index != -1)) {
			fUpdateFunOut.push_back([ = ](ProcessBlock * block) {
				block->switchLights[index - 1][2] = *zone;
			});
		}
		fKey = fValue = "";
	}

	void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override {
		addBarGraph(zone);
	}

	void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override {
		addBarGraph(zone);
	}

	void addSoundfile(const char* label, const char* soundpath, Soundfile** sf_zone) override {
		WARN("Faust Prototype : 'soundfile' primitive not yet supported");
	}

	void declare(FAUSTFLOAT* zone, const char* key, const char* val) override {
		static std::vector<std::string> keys = {"switch", "knob", "light_red", "light_green", "light_blue", "switchlight_red", "switchlight_green", "switchlight_blue"};
		if (find(keys.begin(), keys.end(), key) != keys.end()) {
			fKey = key;
			fValue = val;
		}
		else if (std::string(key) == "scale") {
			fScale = val;
		}
	}
};

// Faust engine using libfaust/LLVM
class FaustEngine : public ScriptEngine {

public:

	FaustEngine():
		fDSPFactory(nullptr),
		fDSP(nullptr),
		fInputs(nullptr),
		fOutputs(nullptr),
		fDSPLibraries(rack::asset::plugin(pluginInstance, "faust_libraries"))
	{}

	~FaustEngine() {
		delete [] fInputs;
		delete [] fOutputs;
		delete fDSP;
#ifdef INTERP
		deleteInterpreterDSPFactory(static_cast<interpreter_dsp_factory*>(fDSPFactory));
#else
		deleteDSPFactory(static_cast<llvm_dsp_factory*>(fDSPFactory));
#endif
	}

	std::string getEngineName() override {
		return "Faust";
	}

	int run(const std::string& path, const std::string& script) override {
#if defined ARCH_LIN
		std::string temp_cache = "/var/tmp/VCV_" + generateSHA1(script);
#elif defined ARCH_MAC
		std::string temp_cache = "/private/var/tmp/VCV_" + generateSHA1(script);
#elif defined ARCH_WIN
		char buf[MAX_PATH + 1] = {0};
		GetTempPath(sizeof(buf), buf);
		std::string temp_cache = std::string(buf) + "/VCV_" + generateSHA1(script);
#endif
		std::string error_msg;

		// Try to load the machine code cache
#ifdef INTERP
		fDSPFactory = readInterpreterDSPFactoryFromBitcodeFile(temp_cache, error_msg);
#else
		fDSPFactory = readDSPFactoryFromMachineFile(temp_cache, "", error_msg);
#endif

		if (!fDSPFactory) {
			// Otherwise recompile the DSP
			int argc = 0;
			const char* argv[8];
			argv[argc++] = "-I";
			argv[argc++] = fDSPLibraries.c_str();
			argv[argc] = nullptr;  // NULL terminated argv

#ifdef INTERP
			fDSPFactory = createInterpreterDSPFactoryFromString("FaustDSP", script, argc, argv, error_msg);
#else
			fDSPFactory = createDSPFactoryFromString("FaustDSP", script, argc, argv, "", error_msg, -1);
#endif
			if (!fDSPFactory) {
				display("ERROR : cannot create factory !");
				WARN("Faust Prototype : %s", error_msg.c_str());
				return -1;
			}
			else {
				// And save the cache
				display("Compiling factory finished");
#ifdef INTERP
				writeInterpreterDSPFactoryToBitcodeFile(static_cast<interpreter_dsp_factory*>(fDSPFactory), temp_cache);
#else
				writeDSPFactoryToMachineFile(static_cast<llvm_dsp_factory*>(fDSPFactory), temp_cache, "");
#endif
			}
		}

		// Create DSP
		fDSP = fDSPFactory->createDSPInstance();
		if (!fDSP) {
			display("ERROR: cannot create instance !");
			return -1;
		}
		else {
			display("Created DSP");
		}

		// Check inputs/outputs
		if (fDSP->getNumInputs() > NUM_ROWS) {
			display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " inputs !");
			return -1;
		}

		if (fDSP->getNumOutputs() > NUM_ROWS) {
			display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " outputs !");
			return -1;
		}

		// Prepare buffers for process
		ProcessBlock* block = getProcessBlock();

		fInputs = new FAUSTFLOAT*[fDSP->getNumInputs()];
		for (int chan = 0; chan < fDSP->getNumInputs(); chan++) {
			fInputs[chan] = block->inputs[chan];
		}

		fOutputs = new FAUSTFLOAT*[fDSP->getNumOutputs()];
		for (int chan = 0; chan < fDSP->getNumOutputs(); chan++) {
			fOutputs[chan] = block->outputs[chan];
		}

		// Setup UI
		fDSP->buildUserInterface(&fRackUI);

		setFrameDivider(1);
		setBufferSize(kBufferSize);

		// Init DSP with default SR
		fDSP->init(44100);
		return 0;
	}

	int process() override {
		ProcessBlock* block = getProcessBlock();

		// Possibly update SR
		if (block->sampleRate != fDSP->getSampleRate()) {
			fDSP->init(block->sampleRate);
		}

		// Update inputs controllers
		for (auto& it : fRackUI.fUpdateFunIn)
			it(block);

		// Compute samples
		fDSP->compute(block->bufferSize, fInputs, fOutputs);

		// Update output controllers
		for (auto& it : fRackUI.fUpdateFunOut)
			it(block);

		return 0;
	}

private:
	dsp_factory* fDSPFactory;
	dsp* fDSP;
	FAUSTFLOAT** fInputs;
	FAUSTFLOAT** fOutputs;
	RackUI fRackUI;
	std::string fDSPLibraries;
};

__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<FaustEngine>("dsp");
}
