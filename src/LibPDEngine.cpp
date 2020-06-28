#include "ScriptEngine.hpp"
#include "z_libpd.h"
#include <iostream>
#include <fstream>
using namespace std;

#define BUFFERSIZE 4096

void chair_write_patch_to_file(const string &patch, const string &path, const string &name )
{
  
  ofstream myfile;
  cout << "file to write: " << path + "/" + name << endl;
  myfile.open (path + "/" + name);
  myfile << patch;
  myfile.close();
}

void chair_load_patch(const string &patch, const string &name)
{

  string path = ".";
  chair_write_patch_to_file(patch, path, name);
  
  libpd_openfile(name.c_str(), path.c_str());
  
  remove( (path + "/" + name).c_str() );
}


struct LibPDEngine : ScriptEngine {

	~LibPDEngine() {
	}
	// // variables for libpd
	t_pdinstance *_lpd;
  	int _sampleRate = 0;
  	int _ticks = 0;
  	float _output[BUFFERSIZE];
  	float _input[BUFFERSIZE];//  = (float*)malloc(1024*2*sizeof(float));
  	// end variables for libpd


	std::string getEngineName() override {
		return "Pure Data";
	}

	int run(const std::string& path, const std::string& script) override {

		ProcessBlock* block = getProcessBlock();

	 	_sampleRate = block->sampleRate;
	  
	  	libpd_init();
	  	_lpd = libpd_new_instance();

	  	libpd_set_instance(_lpd);
	  	libpd_init_audio(2, 2, _sampleRate);
	  	//cout << "_lpd is initialized" << endl;
	  
	  	//cout << "num od pd instances: " << libpd_num_instances() << endl;
	  
	  // compute audio    [; pd dsp 1(
	  	libpd_start_message(1); // one entry in list
	  	libpd_add_float(1.0f);
	  	libpd_finish_message("pd", "dsp");
	  
	  	std::string name = "test.pd";
	  	std::string patch = "#N canvas 333 425 105 153 10;\n"
	                "#X obj 32 79 dac~;\n"
	                "#X obj 32 27 adc~;\n"
	                "#X connect 1 0 0 0;\n"
	                "#X connect 1 1 0 1;";

	  	//libpd_openfile(patch.c_str(), path.c_str());

	  	chair_load_patch(patch, name);
	  
		return 0;
	}

	int process() override {
		// block
		ProcessBlock* block = getProcessBlock();
		int curr_bufSize = 256;
		block->bufferSize = curr_bufSize;
		display(std::to_string(curr_bufSize));

		const int nChans = 2;
		  
		// pass samples to/from libpd
		_ticks = curr_bufSize / 64;
		for (int s = 0; s < curr_bufSize; s++) {
		    for (int c = 0; c < nChans; c++) {
		      _input[s*nChans+c] = block->inputs[c][s];
		    }
		}
		
		libpd_set_instance(_lpd);
		libpd_process_float(_ticks, _input, _output);
		
		for (int s = 0; s < curr_bufSize; s++) {
		    for (int c = 0; c < nChans; c++) {
		    	block->outputs[c][s] = _output[s*nChans+c];
		    }
		  }

		return 0;
	}

	
};


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<LibPDEngine>("pd");
}
