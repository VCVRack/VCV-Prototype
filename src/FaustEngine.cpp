#include "ScriptEngine.hpp"

#include <faust/dsp/llvm-dsp.h>
#include <iostream>

#define kBufferSize 64

class FaustEngine : public ScriptEngine {
    
    public:
    
        FaustEngine():fFactory(nullptr), fDSP(nullptr), fInputs(nullptr), fOutputs(nullptr)
        {}
    
        ~FaustEngine()
        {
            delete [] fInputs;
            delete [] fOutputs;
            delete fDSP;
            deleteDSPFactory(fFactory);
        }
    
        std::string getEngineName() override
        {
            return "Faust";
        }
    
        int run(const std::string& path, const std::string& script) override
        {
            std::string error_msg;
            fFactory = createDSPFactoryFromString("FaustDSP", script, 0, NULL, "", error_msg, -1);
            if (!fFactory) {
                display("ERROR: cannot create Faust factory !");
                return -1;
            } else {
                display("Compiling factory finished");
            }
            
            fDSP = fFactory->createDSPInstance();
            if (!fDSP) {
                display("ERROR: cannot create Faust instance !");
                return -1;
            } else {
                display("Created DSP");
            }
            
            // Prepare inputs/ouputs
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
            fOutputs = new FAUSTFLOAT*[fDSP->getNumOutputs()];
            
            for (int chan = 0; chan < fDSP->getNumInputs(); chan++) {
                fInputs[chan] = block->inputs[chan];
            }
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
        llvm_dsp_factory* fFactory;
        llvm_dsp* fDSP;
        FAUSTFLOAT** fInputs;
        FAUSTFLOAT** fOutputs;
};

__attribute__((constructor(1000)))
static void constructor() {
    addScriptEngine<FaustEngine>("dsp");
}
