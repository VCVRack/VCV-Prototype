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

#define kBufferSize 64

extern rack::Plugin* pluginInstance;

// UI handler for switches, knobs and leds
struct RackUI : public GenericUI
{
    // 0|1 switches
    FAUSTFLOAT* fSwitches[NUM_ROWS];
    
    // Knobs with [0..1] <==> [min..max] mapping and medata 'scale' handling
    ConverterZoneControl* fKnobs[NUM_ROWS];
    
    // Leds
    FAUSTFLOAT* fLedRed[NUM_ROWS];
    FAUSTFLOAT* fLedGreen[NUM_ROWS];
    FAUSTFLOAT* fLedBlue[NUM_ROWS];
    
    // Switch Leds
    FAUSTFLOAT* fSwitchRed[NUM_ROWS];
    FAUSTFLOAT* fSwitchGreen[NUM_ROWS];
    FAUSTFLOAT* fSwitchBlue[NUM_ROWS];
    
    std::string fKey, fValue, fScale;
    
    void addItem(FAUSTFLOAT* table[NUM_ROWS], FAUSTFLOAT* zone, const std::string& value)
    {
        try {
            int index = std::stoi(value);
            if (index >= 0 && index <= NUM_ROWS) {
                table[index-1] = zone;
            } else {
                std::cerr << "ERROR : incorrect '" << index << "' value !\n";
            }
        } catch (std::invalid_argument& e) {
            std::cerr << "ERROR : " << e.what() << std::endl;
        }
        fValue = fKey = fScale = "";
    }
    
    void addItemConverter(ConverterZoneControl* table[NUM_ROWS], FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max, const std::string& value)
    {
        try {
            int index = std::stoi(value);
            if (index >= 0 && index <= NUM_ROWS) {
                // Select appropriate converter according to scale mode
                if (fScale == "log") {
                    table[index-1] = new ConverterZoneControl(zone, new LogValueConverter(0., 1., min, max));
                } else if (fScale == "exp") {
                    table[index-1] = new ConverterZoneControl(zone, new ExpValueConverter(0., 1., min, max));
                } else {
                    table[index-1] = new ConverterZoneControl(zone, new LinearValueConverter(0., 1., min, max));
                }
            } else {
                std::cerr << "ERROR : incorrect '" << index << "' value !\n";
            }
        } catch (std::invalid_argument& e) {
            std::cerr << "ERROR : " << e.what() << std::endl;
        }
        fValue = fKey = fScale = "";
    }
    
    RackUI()
    {
        fScale = "lin";
        for (int i = 0; i < NUM_ROWS; i++) {
            fSwitches[i] = nullptr;
            fKnobs[i] = nullptr;
            fLedRed[i] = nullptr;
            fLedGreen[i] = nullptr;
            fLedBlue[i] = nullptr;
            fSwitchRed[i] = nullptr;
            fSwitchGreen[i] = nullptr;
            fSwitchBlue[i] = nullptr;
        }
    }
    
    virtual ~RackUI()
    {
        for (int i = 0; i < NUM_ROWS; i++) {
            delete fKnobs[i];
        }
    }
    
    void addButton(const char* label, FAUSTFLOAT* zone)
    {
        if (fKey == "switch") {
            addItem(fSwitches, zone, fValue);
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
        if (fKey == "knob") {
            addItemConverter(fKnobs, zone, min, max, fValue);
        }
    }
    
    void addBarGraph(FAUSTFLOAT* zone)
    {
        if (fKey == "led_red") {
            addItem(fLedRed, zone, fValue);
        } else if (fKey == "led_green") {
            addItem(fLedGreen, zone, fValue);
        } else if (fKey == "led_blue") {
            addItem(fLedBlue, zone, fValue);
        } else if (fKey == "switch_red") {
            addItem(fSwitchRed, zone, fValue);
        } else if (fKey == "switch_green") {
            addItem(fSwitchGreen, zone, fValue);
        } else if (fKey == "switch_blue") {
            addItem(fSwitchBlue, zone, fValue);
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
        if ((std::string(key) == "switch")
            || (std::string(key) == "knob")
            || (std::string(key) == "led_red")
            || (std::string(key) == "led_green")
            || (std::string(key) == "led_blue")
            || (std::string(key) == "switch_red")
            || (std::string(key) == "switch_green")
            || (std::string(key) == "switch_blue")) {
            fKey = key;
            fValue = val;
        } else if (std::string(key) == "scale") {
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
    
        std::string getEngineName() override
        {
            return "Faust";
        }
    
        int run(const std::string& path, const std::string& script) override
        {
        #if defined ARCH_MAC
            std::string temp_cache = "/private/var/tmp/VCV_" + generateSHA1(script);
        #else
            std::string temp_cache = "" + generateSHA1(script);
        #endif
            std::string error_msg;
            
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
            
            // Prepare inputs/outputs
            if (fDSP->getNumInputs() > NUM_ROWS) {
                display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " inputs !");
                return -1;
            }
            
            if (fDSP->getNumOutputs() > NUM_ROWS) {
                display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " outputs !");
                return -1;
            }
            
            // Setup UI
            fDSP->buildUserInterface(&fRackUI);
            
            setFrameDivider(1);
            setBufferSize(kBufferSize);
        
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
            for (int i = 0; i < NUM_ROWS; i++) {
                if (fRackUI.fSwitches[i]) {
                    *fRackUI.fSwitches[i] = block->switches[i];
                }
                if (fRackUI.fKnobs[i]) {
                    fRackUI.fKnobs[i]->update(block->knobs[i]);
                }
            }
            
            // Compute samples
            fDSP->compute(kBufferSize, fInputs, fOutputs);
            
            // Update output controllers
            for (int i = 0; i < NUM_ROWS; i++) {
                if (fRackUI.fLedRed[i]) {
                    block->lights[i][0] = *fRackUI.fLedRed[i];
                }
                if (fRackUI.fLedGreen[i]) {
                    block->lights[i][1] = *fRackUI.fLedGreen[i];
                }
                if (fRackUI.fLedBlue[i]) {
                    block->lights[i][2] = *fRackUI.fLedBlue[i];
                }
                if (fRackUI.fSwitchRed[i]) {
                    block->switchLights[i][0] = *fRackUI.fSwitchRed[i];
                }
                if (fRackUI.fSwitchGreen[i]) {
                    block->switchLights[i][1] = *fRackUI.fSwitchGreen[i];
                }
                if (fRackUI.fSwitchBlue[i]) {
                    block->switchLights[i][2] = *fRackUI.fSwitchBlue[i];
                }
            }
            
            return 0;
        }
    
    private:
        llvm_dsp_factory* fDSPFactory;
        llvm_dsp* fDSP;
        FAUSTFLOAT** fInputs;
        FAUSTFLOAT** fOutputs;
        RackUI fRackUI;
        std::string fDSPLibraries;
};

__attribute__((constructor(1000)))
static void constructor() {
    addScriptEngine<FaustEngine>("dsp");
}
