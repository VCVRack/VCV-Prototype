#include "ScriptEngine.hpp"

#include <faust/dsp/llvm-dsp.h>
#include <faust/dsp/libfaust.h>

#include <iostream>

#define kBufferSize 64

class FaustEngine : public ScriptEngine {
    
    public:
    
        FaustEngine():fDSPFactory(nullptr), fDSP(nullptr), fInputs(nullptr), fOutputs(nullptr)
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
                fDSPFactory = createDSPFactoryFromString("FaustDSP", script, 0, NULL, "", error_msg, -1);
                if (!fDSPFactory) {
                    display("ERROR: cannot create factory !");
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
            if (fDSP->getNumInputs() > 6) {
                display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " inputs !");
                return -1;
            }
            
            if (fDSP->getNumOutputs() > 6) {
                display("ERROR: DSP has " + std::to_string(fDSP->getNumInputs()) + " outputs !");
                return -1;
            }
            
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
            // Possibly update SR
            if (getProcessBlock()->sampleRate != fDSP->getSampleRate()) {
                fDSP->init(getProcessBlock()->sampleRate);
            }
            fDSP->compute(kBufferSize, fInputs, fOutputs);
            return 0;
        }
    
    private:
        llvm_dsp_factory* fDSPFactory;
        llvm_dsp* fDSP;
        FAUSTFLOAT** fInputs;
        FAUSTFLOAT** fOutputs;
};

__attribute__((constructor(1000)))
static void constructor() {
    addScriptEngine<FaustEngine>("dsp");
}
