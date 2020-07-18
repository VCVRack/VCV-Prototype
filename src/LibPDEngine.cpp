#include "ScriptEngine.hpp"
#include "z_libpd.h"
#include "util/z_print_util.h"
using namespace rack;

static const int BUFFERSIZE = MAX_BUFFER_SIZE * NUM_ROWS;


// there is no multi-instance support for receiving messages from libpd
// for now, received values for the prototype gui will be stored in global variables

static float g_lights[NUM_ROWS][3] = {};
static float g_switchLights[NUM_ROWS][3] = {};
static std::string g_utility[2] = {};
static bool g_display_is_valid = false;

static std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}

	return result;
}


struct LibPDEngine : ScriptEngine {
	t_pdinstance* _lpd = NULL;
	int _pd_block_size = 64;
	int _sampleRate = 0;
	int _ticks = 0;
	bool _init = true;

	float _old_knobs[NUM_ROWS] = {};
	bool  _old_switches[NUM_ROWS] = {};
	float _output[BUFFERSIZE] = {};
	float _input[BUFFERSIZE] = {};//  = (float*)malloc(1024*2*sizeof(float));
	const static std::map<std::string, int> _light_map;
	const static std::map<std::string, int> _switchLight_map;
	const static std::map<std::string, int> _utility_map;

	~LibPDEngine() {
		if (_lpd)
			libpd_free_instance(_lpd);
	}

	void sendInitialStates(const ProcessBlock* block);
	static void receiveLights(const char* s);
	bool knobChanged(const float* knobs, int idx);
	bool switchChanged(const bool* knobs, int idx);
	void sendKnob(const int idx, const float value);
	void sendSwitch(const int idx, const bool value);

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
		libpd_set_concatenated_printhook(receiveLights);


		if (libpd_num_instances() > 2) {
			display("Sorry, multi instance support in libpd is under development!");
			return -1;
		}

		//display(std::to_string(libpd_num_instances()));
		libpd_init_audio(NUM_ROWS, NUM_ROWS, _sampleRate);

		// compute audio    [; pd dsp 1(
		libpd_start_message(1); // one enstry in list
		libpd_add_float(1.0f);
		libpd_finish_message("pd", "dsp");

		std::string version = "pd " + std::to_string(PD_MAJOR_VERSION) + "." +
		                      std::to_string(PD_MINOR_VERSION) + "." +
		                      std::to_string(PD_BUGFIX_VERSION);

		display(version);

		std::string name = string::filename(path);
		std::string dir  = string::directory(path);
		libpd_openfile(name.c_str(), dir.c_str());

		sendInitialStates(block);

		return 0;
	}

	int process() override {
		// block
		ProcessBlock* block = getProcessBlock();

		// get samples prototype
		int rows = NUM_ROWS;
		for (int s = 0; s < _pd_block_size; s++) {
			for (int r = 0; r < rows; r++) {
				_input[s * rows + r] = block->inputs[r][s];
			}
		}

		libpd_set_instance(_lpd);

		// knobs
		for (int i = 0; i < NUM_ROWS; i++) {
			if (knobChanged(block->knobs, i)) {
				sendKnob(i, block->knobs[i]);
			}
		}
		// lights
		for (int i = 0; i < NUM_ROWS; i++) {
			block->lights[i][0] = g_lights[i][0];
			block->lights[i][1] = g_lights[i][1];
			block->lights[i][2] = g_lights[i][2];
		}
		// switch lights
		for (int i = 0; i < NUM_ROWS; i++) {
			block->switchLights[i][0] = g_switchLights[i][0];
			block->switchLights[i][1] = g_switchLights[i][1];
			block->switchLights[i][2] = g_switchLights[i][2];
		}
		// switches
		for (int i = 0; i < NUM_ROWS; i++) {
			if (switchChanged(block->switches, i)) {
				sendSwitch(i, block->switches[i]);
			}
		}

		// display
		if (g_display_is_valid) {
			display(g_utility[1]);
			g_display_is_valid = false;
		}
		// process samples in libpd
		_ticks = 1;
		libpd_process_float(_ticks, _input, _output);

		//return samples to prototype
		for (int s = 0; s < _pd_block_size; s++) {
			for (int r = 0; r < rows; r++) {
				block->outputs[r][s] = _output[s * rows + r]; // scale up again to +-5V signal
				// there is a correction multilpier, because libpd's output is too quiet(?)
			}
		}

		return 0;
	}
};


void LibPDEngine::receiveLights(const char* s) {
	std::string str = std::string(s);
	std::vector<std::string> atoms = split(str, ' ');

	if (atoms[0] == "toRack:") {
		// parse lights list
		bool light_is_valid = true;
		int light_idx = -1;
		try {
			light_idx = _light_map.at(atoms[1]);      // map::at throws an out-of-range
		}
		catch (const std::out_of_range& oor) {
			light_is_valid = false;
			//display("Warning:"+atoms[1]+" not found!");
		}
		//std::cout << v[1] << ", " << g_led_map[v[1]] << std::endl;
		if (light_is_valid && atoms.size() == 5) {
			g_lights[light_idx][0] = stof(atoms[2]); // red
			g_lights[light_idx][1] = stof(atoms[3]); // green
			g_lights[light_idx][2] = stof(atoms[4]); // blue
		}
		else {
			// error
		}
		// parse switch lights list
		bool switchLight_is_valid = true;
		int switchLight_idx = -1;
		try {
			switchLight_idx = _switchLight_map.at(atoms[1]);      // map::at throws an out-of-range
		}
		catch (const std::out_of_range& oor) {
			switchLight_is_valid = false;
			//display("Warning:"+atoms[1]+" not found!");
		}
		//std::cout << v[1] << ", " << g_led_map[v[1]] << std::endl;
		if (switchLight_is_valid && atoms.size() == 5) {
			g_switchLights[switchLight_idx][0] = stof(atoms[2]); // red
			g_switchLights[switchLight_idx][1] = stof(atoms[3]); // green
			g_switchLights[switchLight_idx][2] = stof(atoms[4]); // blue
		}
		else {
			// error
		}

		// parse switch lights list
		bool utility_is_valid = true;
		try {
			_utility_map.at(atoms[1]);      // map::at throws an out-of-range
		}
		catch (const std::out_of_range& oor) {
			utility_is_valid = false;
			//g_display_is_valid = true;
			//display("Warning:"+atoms[1]+" not found!");
		}
		//std::cout << v[1] << ", " << g_led_map[v[1]] << std::endl;
		if (utility_is_valid && atoms.size() >= 3) {
			g_utility[0] = atoms[1]; // display
			g_utility[1] = {""};
			for (unsigned i = 0; i < atoms.size() - 2; i++) {
				g_utility[1] += " " + atoms[i + 2]; // concatenate message
			}
			g_display_is_valid = true;
		}
		else {
			// error
		}
	}
	else {
		// print out on command line
		std::cout << "libpd prototype unrecognizes message: " << std::string(s) << std::endl;
	}
}

bool LibPDEngine::knobChanged(const float* knobs, int i) {
	bool knob_changed = false;
	if (_old_knobs[i] != knobs[i]) {
		knob_changed = true;
		_old_knobs[i] = knobs[i];
	}
	return knob_changed;
}

bool LibPDEngine::switchChanged(const bool* switches, int i) {
	bool switch_changed = false;
	if (_old_switches[i] != switches[i]) {
		switch_changed = true;
		_old_switches[i] = switches[i];
	}
	return switch_changed;
}

const std::map<std::string, int> LibPDEngine::_light_map{
	{ "L1", 0 },
	{ "L2", 1 },
	{ "L3", 2 },
	{ "L4", 3 },
	{ "L5", 4 },
	{ "L6", 5 }
};

const std::map<std::string, int> LibPDEngine::_switchLight_map{
	{ "S1", 0 },
	{ "S2", 1 },
	{ "S3", 2 },
	{ "S4", 3 },
	{ "S5", 4 },
	{ "S6", 5 }
};

const std::map<std::string, int> LibPDEngine::_utility_map{
	{ "display", 0 }
};


void LibPDEngine::sendKnob(const int idx, const float value) {
	std::string knob = "K" + std::to_string(idx + 1);
	libpd_start_message(1);
	libpd_add_float(value);
	libpd_finish_message("fromRack", knob.c_str());
}

void LibPDEngine::sendSwitch(const int idx, const bool value) {
	std::string sw = "S" + std::to_string(idx + 1);
	libpd_start_message(1);
	libpd_add_float(value);
	libpd_finish_message("fromRack", sw.c_str());
}

void LibPDEngine::sendInitialStates(const ProcessBlock* block) {
	// knobs
	for (int i = 0; i < NUM_ROWS; i++) {
		sendKnob(i, block->knobs[i]);
		sendSwitch(i, block->knobs[i]);
	}

	for (int i = 0; i < NUM_ROWS; i++) {
		g_lights[i][0] = 0;
		g_lights[i][1] = 0;
		g_lights[i][2] = 0;
		g_switchLights[i][0] = 0;
		g_switchLights[i][1] = 0;
		g_switchLights[i][2] = 0;
	}

	//g_utility[0] = "";
	//g_utility[1] = "";

	//g_display_is_valid = false;
}


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<LibPDEngine>("pd");
}
