#include "ScriptEngine.hpp"
#include "z_libpd.h"
#include "util/z_print_util.h"
using namespace rack;

#define BUFFERSIZE MAX_BUFFER_SIZE * NUM_ROWS


// there is no multi-instance support for receiving messages from libpd
// for now, received values for the prototype gui will be stored in global variables

float g_leds[NUM_ROWS][3];

std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}




struct LibPDEngine : ScriptEngine {

	~LibPDEngine() {
		libpd_free_instance(_lpd);

	}

	static void receiveLEDs(const char *s);

	bool knobChanged(const float* knobs, int idx);

	bool switchChanged(const float* knobs, int idx);

	t_pdinstance *_lpd;
	int _pd_block_size = 64;
  	int _sampleRate = 0;
  	int _ticks = 0;
  	
  	float _old_knobs[NUM_ROWS];
  	bool  _old_switches[NUM_ROWS];
  	float _output[BUFFERSIZE];
  	float _input[BUFFERSIZE];//  = (float*)malloc(1024*2*sizeof(float));
  	const static std::map<std::string, int> _led_map;


	std::string getEngineName() override {
		return "Pure Data";
	}

	int run(const std::string& path, const std::string& script) override {

		ProcessBlock* block = getProcessBlock();
	 	_sampleRate = block->sampleRate;
	 	setBufferSize(_pd_block_size);
	  	setFrameDivider(1);
	  	libpd_init();
	  	_lpd = libpd_new_instance();

	  	libpd_set_printhook((t_libpd_printhook)libpd_print_concatenator);
		libpd_set_concatenated_printhook( receiveLEDs );
	  	//libpd_set_printhook(receiveLEDs);
	  	//libpd_bind("L2");
	  	//libpd_bind("foo");
	  	//libpd_set_listhook(receiveList);
	  	//libpd_set_messagehook(receiveMessage);

	 	if(libpd_num_instances()>2)
		{
			display("Sorry, multi instance support in libpd is under development!");
			return -1;
		}

	  	//display(std::to_string(libpd_num_instances()));
	  	libpd_init_audio(NUM_ROWS, NUM_ROWS, _sampleRate);

	    // compute audio    [; pd dsp 1(
	  	libpd_start_message(1); // one enstry in list
	  	libpd_add_float(1.0f);
	  	libpd_finish_message("pd", "dsp");
	  
	    std::string name = string::filename(path);
	    std::string dir  = string::directory(path);
	  	libpd_openfile(name.c_str(), dir.c_str());
	  
		return 0;
	}

	int process() override {
		// block
		ProcessBlock* block = getProcessBlock();

		// get samples prototype
		int rows = NUM_ROWS;
		for (int s = 0; s < _pd_block_size; s++) {
		    for (int r = 0; r < rows; r++) {
		      _input[s*rows+r] = block->inputs[r][s];
		    }
		}
		
		libpd_set_instance(_lpd);

		// knobs
		for (int i=0; i<NUM_ROWS; i++){
			if( knobChanged(block->knobs, i) ){
				std::string knob = "K"+std::to_string(i+1);
				libpd_float(knob.c_str(), block->knobs[i]);
			}
		}
		// leds
		//display(std::to_string(g_leds[0]));
		for(int i=0; i<NUM_ROWS; i++){
			block->lights[i][0] = g_leds[i][0];
			block->lights[i][1] = g_leds[i][1];
			block->lights[i][2] = g_leds[i][2];
		}
		// switches
		for(int i=0; i<NUM_ROWS; i++){
			if( switchChanged(block->switches, i) ){
				std::string sw = "S"+std::to_string(i+1);
				libpd_float(sw.c_str(), block->switches[i]);
			}
		}
		// process samples in libpd
		_ticks = 1;
		libpd_process_float(_ticks, _input, _output);
		
		//return samples to prototype
		for (int s = 0; s < _pd_block_size; s++) {
		    for (int r = 0; r < rows; r++) {
		    	block->outputs[r][s] = _output[s*rows+r];
		    }
		  }

		return 0;
	}
};


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<LibPDEngine>("pd");
}


void LibPDEngine::receiveLEDs(const char *s) {
	std::string str = std::string(s);
	std::vector<std::string> atoms = split (str, ' ');

	if(atoms[0]=="toVCV:"){
		// led list
		bool led_is_valid = true;
		int led_idx = -1;
		try {
				led_idx = _led_map.at(atoms[1]);      // map::at throws an out-of-range
			}
		catch (const std::out_of_range& oor) {
			led_is_valid = false;
			//display("Warning:"+atoms[1]+" not found!");
		}
		//std::cout << v[1] << ", " << g_led_map[v[1]] << std::endl;
		if(led_is_valid && atoms.size()==5){
			g_leds[led_idx][0] = stof(atoms[2]); // red
			g_leds[led_idx][1] = stof(atoms[3]); // green
			g_leds[led_idx][2] = stof(atoms[4]); // blue
		}
		else {
			
		}
	}
	else { 
		// do nothing
	}
}

bool LibPDEngine::knobChanged(const float* knobs, int i){
	bool knob_changed = false;
	if (_old_knobs[i] != knobs[i]){
		knob_changed = true;
		_old_knobs[i] = knobs[i];
	}
	return knob_changed;
}

bool LibPDEngine::switchChanged(const float* switches, int i){
	bool switch_changed = false;
	if (_old_switches[i] != switches[i]){
		switch_changed = true;
		_old_switches[i] = switches[i];
	}
	return switch_changed;
}

const std::map<std::string, int> LibPDEngine::_led_map{
	{ "L1", 0 },
	{ "L2", 1 },
	{ "L3", 2 },
	{ "L4", 3 },
	{ "L5", 4 },
 	{ "L6", 5 }
};