#include "ScriptEngine.hpp"

#include <faust/dsp/llvm-dsp.h>
#include <iostream>

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
            display("Compiling...");
            
            std::string error_msg;
            fFactory = createDSPFactoryFromString("FaustDSP", script, 0, NULL, "", error_msg, -1);
            if (!fFactory) {
                display("Cannot create Faust factory");
                return -1;
            } else {
                display("Compiling finished");
            }
            
            fDSP = fFactory->createDSPInstance();
            if (!fDSP) {
                display("Cannot create Faust instance");
                return -1;
            } else {
                display("Creating DSP");
            }
            
            fInputs = new FAUSTFLOAT*[fDSP->getNumInputs()];
            fOutputs = new FAUSTFLOAT*[fDSP->getNumOutputs()];
            
            setFrameDivider(1);
            setBufferSize(64);
            
            // TODO
            //ProcessBlock* block = getProcessBlock();
            fDSP->init(44100);
            return 0;
        }
    
        int process() override
        {
            ProcessBlock* block = getProcessBlock();
            
            for (int chan = 0; chan < fDSP->getNumInputs(); chan++) {
                fInputs[chan] = block->inputs[chan];
            }
            for (int chan = 0; chan < fDSP->getNumOutputs(); chan++) {
                fOutputs[chan] = block->outputs[chan];
            }
            
            fDSP->compute(block->bufferSize, fInputs, fOutputs);
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
