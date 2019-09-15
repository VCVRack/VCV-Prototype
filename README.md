# VCV Prototype

Scripting language host for [VCV Rack](https://vcvrack.com/) containing:
- 6 inputs
- 6 outputs
- 6 knobs
- 6 lights (RGB LEDs)
- 6 switches with RGB LEDs

### Scripting API

This is a reference API for the `DuktapeEngine` (JavaScript).
Other script engines may vary in their syntax (e.g. `block.inputs[i][j]` vs `block.getInput(i, j)` vs `input(i, j)`).

```js
// Display message on LED display.
display(message)
// Skip this many sample frames before running process().
// For sequencers, 32 is reasonable since process() will be called every 0.7ms with a 44100kHz sample rate.
// For audio generators and processors, 1 is recommended. If this is too slow for your purposes, write a C++ plugin.
config.frameDivider = 1
// Number of samples to store each block passed to process().
// Latency introduced by buffers is `bufferSize * frameDivider * sampleTime`.
config.bufferSize = 1

// Called when the next block is ready to be processed.
function process(block) {
	// Engine sample rate in Hz. Read-only.
	block.sampleRate
	// Equal to `1 / sampleRate`. Read-only.
	block.sampleTime
	// The actual buffer size, requested by `config.bufferSize`. Read-only.
	block.bufferSize
	// Voltage of the input port of row `i` and buffer index `j`. Read-only.
	block.inputs[i][j]
	// Voltage of the output port of row `i` and buffer index `j`. Writable.
	block.outputs[i][j]
	// Value of the knob of row `i`. Between 0 and 1. Read-only.
	block.knobs[i]
	// Pressed state of the switch of row `i`. Read-only.
	block.switches[i]
	// Brightness of the RGB LED of row `i` and color index `c`. Writable.
	// `c=0` for red, `c=1` for green, `c=2` for blue.
	block.lights[i][c]
	// Brightness of the switch RGB LED of row `i` and color index `c`. Writable.
	block.switchLights[i][c]
}
```

### Adding a script engine

- Add your scripting language library to the build system so it builds with `make dep`, following the Duktape example in the `Makefile`.
- Create a `MyEngine.cpp` file in `src/` with a `ScriptEngine` subclass defining the virtual methods, following `src/DuktapeEngine.cpp` as an example.
- Add your engine to the "List of ScriptEngines" in `src/ScriptEngine.cpp`.
- Build and test VCV Prototype.
- Add a few example scripts and tests to `examples/`. These will be included in the plugin package for the user.
- Add your name to the Contributers list below.
- Send a pull request. Once merged, you will be added as a repo maintainer. Be sure to "watch" this repo to be notified of bugs in your engine.

### Maintainers

- [Wes Milholen](https://grayscale.info/): panel design
- [Andrew Belt](https://github.com/AndrewBelt): host code, `DuktapeEngine` (JavaScript)
- add your name here