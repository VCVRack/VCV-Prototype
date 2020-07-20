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

#include <faust/dsp/llvm-dsp.h>
#include <faust/dsp/libfaust.h>
#include <faust/gui/DecoratorUI.h>
#include <faust/gui/ValueConverter.h>

#include <iostream>
#include <memory>

using namespace std;

#define kBufferSize 64

extern rack::Plugin* pluginInstance;

// UI handler for switches, knobs and lights
struct RackUI : public GenericUI
{
    typedef function<void(ProcessBlock* block)> updateFunction;
    
    vector<ConverterZoneControl*> fConverters;
    vector<updateFunction> fUpdateFunIn;
    vector<updateFunction> fUpdateFunOut;
    
    // For checkbox handling
    struct CheckBox { float fLastButton = 0.0f; };
    map <FAUSTFLOAT*, CheckBox> fCheckBoxes;
    
    string fKey, fValue, fScale;
    
    int getIndex(const string& value)
    {
        try {
            int index = stoi(value);
            if (index >= 0 && index <= NUM_ROWS) {
                return index;
            } else {
                cerr << "ERROR : incorrect '" << index << "' value !\n";
                return -1;
            }
        } catch (invalid_argument& e) {
            return -1;
        }
    }
   
    RackUI():fScale("lin")
    {}
    
    virtual ~RackUI()
    {
        for (auto& it : fConverters) delete it;
    }
    
    void addButton(const char* label, FAUSTFLOAT* zone)
    {
        int index = getIndex(fValue);
        if (fKey == "switch" && (index != -1)) {
            fUpdateFunIn.push_back([=] (ProcessBlock* block) { *zone = block->switches[index-1]; });
        }
    }
    
    void addCheckButton(const char* label, FAUSTFLOAT* zone)
    {
        int index = getIndex(fValue);
        if (fKey == "switch" && (index != -1)) {
            // Add a checkbox
            fCheckBoxes[zone] = CheckBox();
            // Update function
            fUpdateFunIn.push_back([=] (ProcessBlock* block)
            {
                float button = block->switches[index-1];
                // Detect upfront
                if (button == 1.0 && (button != fCheckBoxes[zone].fLastButton)) {
                    // Switch button state
                    *zone = !*zone;
                    // And set the color
                    block->switchLights[index-1][0] = *zone;
                    block->switchLights[index-1][1] = *zone;
                    block->switchLights[index-1][2] = *zone;
                }
                // Keep previous button state
                fCheckBoxes[zone].fLastButton = button;
            });
        }
    }
  
    void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    {
        addNumEntry(label, zone, init, min, max, step);
    }
    
    void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    {
        addNumEntry(label, zone, init, min, max, step);
    }
    
    void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    {
        int index = getIndex(fValue);
        if (fKey == "knob" && (index != -1)) {
            ConverterZoneControl* converter;
            if (fScale == "log") {
                converter = new ConverterZoneControl(zone, new LogValueConverter(0., 1., min, max));
            } else if (fScale == "exp") {
                converter = new ConverterZoneControl(zone, new ExpValueConverter(0., 1., min, max));
            } else {
                converter = new ConverterZoneControl(zone, new LinearValueConverter(0., 1., min, max));
            }
            fUpdateFunIn.push_back([=] (ProcessBlock* block) { converter->update(block->knobs[index-1]); });
            fConverters.push_back(converter);
        }
    }
    
    void addBarGraph(FAUSTFLOAT* zone)
    {
        int index = getIndex(fValue);
        if ((fKey == "light_red") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->lights[index-1][0] = *zone; });
        } else if ((fKey == "light_green") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->lights[index-1][1] = *zone; });
        } else if ((fKey == "light_blue") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->lights[index-1][2] = *zone; });
        } else if ((fKey == "switchlight_red") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->switchLights[index-1][0] = *zone; });
        } else if ((fKey == "switchlight_green") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->switchLights[index-1][1] = *zone; });
        } else if ((fKey == "switchlight_blue") && (index != -1)) {
            fUpdateFunOut.push_back([=] (ProcessBlock* block) { block->switchLights[index-1][2] = *zone; });
        }
    }
    
    void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
    {
        addBarGraph(zone);
    }
    
    void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
    {
        addBarGraph(zone);
    }
        
    void declare(FAUSTFLOAT* zone, const char* key, const char* val)
    {
        static vector<string> keys = {"switch", "knob", "light_red", "light_green", "light_blue", "switchlight_red", "switchlight_green", "switchlight_blue"};
        if (find(keys.begin(), keys.end(), key) != keys.end()) {
            fKey = key;
            fValue = val;
        } else if (string(key) == "scale") {
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
    
        ~FaustEngine()
        {
            delete [] fInputs;
            delete [] fOutputs;
            delete fDSP;
            deleteDSPFactory(fDSPFactory);
        }
    
        string getEngineName() override
        {
            return "Faust";
        }
    
        int run(const string& path, const string& script) override
        {
        #if defined ARCH_LIN
            string temp_cache = "/var/tmp/VCV_" + generateSHA1(script);
        #elif defined ARCH_MAC
            string temp_cache = "/private/var/tmp/VCV_" + generateSHA1(script);
        #elif defined ARCH_WIN
            char buf[MAX_PATH+1] = {0};
            GetTempPath(sizeof(buf), buf);
            string temp_cache = string(buf) + "/VCV_" + generateSHA1(script);
        #endif
            string error_msg;
            
            // Try to load the machine code cache
            fDSPFactory = readDSPFactoryFromMachineFile(temp_cache, "", error_msg);
            
            if (!fDSPFactory) {
                // Otherwise recompile the DSP
                int argc = 0;
                const char* argv[8];
                argv[argc++] = "-I";
                argv[argc++] = fDSPLibraries.c_str();
                argv[argc] = nullptr;  // NULL terminated argv
                
                fDSPFactory = createDSPFactoryFromString("FaustDSP", script, argc, argv, "", error_msg, -1);
                if (!fDSPFactory) {
                    display("ERROR : cannot create factory !");
                    WARN("Faust Prototype : %s", error_msg.c_str());
                    return -1;
                } else {
                    // And save the cache
                    display("Compiling factory finished");
                    writeDSPFactoryToMachineFile(fDSPFactory, temp_cache, "");
                }
            }
            
            // Create DSP
            fDSP = fDSPFactory->createDSPInstance();
            if (!fDSP) {
                display("ERROR: cannot create instance !");
                return -1;
            } else {
                display("Created DSP");
            }
            
            // Check inputs/outputs
            if (fDSP->getNumInputs() > NUM_ROWS) {
                display("ERROR: DSP has " + to_string(fDSP->getNumInputs()) + " inputs !");
                return -1;
            }
            
            if (fDSP->getNumOutputs() > NUM_ROWS) {
                display("ERROR: DSP has " + to_string(fDSP->getNumInputs()) + " outputs !");
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
    
        int process() override
        {
            ProcessBlock* block = getProcessBlock();
            
            // Possibly update SR
            if (block->sampleRate != fDSP->getSampleRate()) {
                fDSP->init(block->sampleRate);
            }
            
            // Update inputs controllers
            for (auto& it : fRackUI.fUpdateFunIn) it(block);
            
            // Compute samples
            fDSP->compute(block->bufferSize, fInputs, fOutputs);
            
            // Update output controllers
            for (auto& it : fRackUI.fUpdateFunOut) it(block);
            
            return 0;
        }
    
    private:
        llvm_dsp_factory* fDSPFactory;
        llvm_dsp* fDSP;
        FAUSTFLOAT** fInputs;
        FAUSTFLOAT** fOutputs;
        RackUI fRackUI;
        string fDSPLibraries;
};

__attribute__((constructor(1000)))
static void constructor() {
    addScriptEngine<FaustEngine>("dsp");
}
