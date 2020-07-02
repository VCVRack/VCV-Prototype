# VCV Prototype

Scripting language host for [VCV Rack](https://vcvrack.com/) containing:
- 6 inputs
- 6 outputs
- 6 knobs
- 6 lights (RGB LEDs)
- 6 switches with RGB LEDs

Supported scripting languages:
- JavaScript (ES2020) (.js)
- [Lua](https://www.lua.org/) (.lua)
- [Vult](https://github.com/modlfo/vult) (.vult)
- [Pure Data](https://puredata.info) (.pd)
- [Add your own below](#adding-a-script-engine)

[Discussion thread](https://community.vcvrack.com/t/vcv-prototype/3271)

## Scripting API

This is the reference API for the JavaScript script engine, along with default property values.
Other script engines may vary in their syntax (e.g. `block.inputs[i][j]` vs `block.getInput(i, j)` vs `input(i, j)`), but the functionality should be similar.

```js
/** Display message on LED display.
*/
display(message)

/** Skip this many sample frames before running process().
For CV generators and processors, 256 is reasonable.
For sequencers, 32 is reasonable since process() will be called every 0.7ms with
a 44100kHz sample rate, which will capture 1ms-long triggers.
For audio generators and processors, 1 is recommended, but use `bufferSize` below.
If this is too slow for your purposes, consider writing a C++ plugin, since
native VCV Rack plugins have 10-100x better performance.
*/
config.frameDivider // 32

/** Instead of calling process() every sample frame, hold this many input/output
voltages in a buffer and call process() when it is full.
This decreases CPU usage, since processing buffers is faster than processing one
frame at a time.
The total latency of your script in seconds is
`config.frameDivider * config.bufferSize * block.sampleTime`.
*/
config.bufferSize // 1

/** Called when the next block is ready to be processed.
*/
function process(block) {
	/** Engine sample rate in Hz. Read-only.
	*/
	block.sampleRate

	/** Engine sample timestep in seconds. Equal to `1 / sampleRate`. Read-only.
	Note that the actual time between process() calls is
	`block.sampleTime * config.frameDivider`.
	*/
	block.sampleTime

	/** The actual size of the input/output buffers.
	*/
	block.bufferSize

	/** Voltage of the input port of column `i`. Read-only.
	*/
	block.inputs[i][bufferIndex] // 0.0

	/** Voltage of the output port of column `i`. Read/write.
	*/
	block.outputs[i][bufferIndex] // 0.0

	/** Value of the knob of column `i`. Between 0 and 1. Read/write.
	*/
	block.knobs[i] // 0.0

	/** Pressed state of the switch of column `i`. Read-only.
	*/
	block.switches[i] // false

	/** Brightness of the RGB LED of column `i`, between 0 and 1. Read/write.
	*/
	block.lights[i][0] // 0.0 (red)
	block.lights[i][1] // 0.0 (green)
	block.lights[i][2] // 0.0 (blue)

	/** Brightness of the switch RGB LED of column `i`. Read/write.
	*/
	block.switchLights[i][0] // 0.0 (red)
	block.switchLights[i][1] // 0.0 (green)
	block.switchLights[i][2] // 0.0 (blue)
}
```

*The Vult API is slightly different than Prototype's scripting API.
See `examples/template.vult` for a reference of the Vult API.*

## Build dependencies

### Windows
```bash
pacman -S mingw-w64-x86_64-premake
```

### Mac
```bash
brew install premake
```

### Ubuntu 16.04+
```bash
sudo apt install premake4
```

### Arch Linux
```bash
sudo pacman -S premake
```

## Build
### Add path to Rack-SDK
```bash
export RACK_DIR=/set/path/to/Rack-SDK/
```

### load submodules
```bash
git submodule update --init --recursive
```

### Make
```bash
make dep
make
```

## Adding a script engine

- Add your scripting language library to the build system so it builds with `make dep`, following the Duktape example in `Makefile`.
- Create a `MyEngine.cpp` file (for example) in `src/` with a `ScriptEngine` subclass defining the virtual methods, using `src/DuktapeEngine.cpp` as an example.
- Build and test the plugin.
- Add a few example scripts and tests to `examples/`. These will be included in the plugin package for the user.
- Add your name to the Contributors list below.
- Send a pull request. Once merged, you will be added as a repo maintainer. Be sure to "watch" this repo to be notified of bug reports and feature requests for your engine.

## Contributors

- [Wes Milholen](https://grayscale.info/): panel design
- [Andrew Belt](https://github.com/AndrewBelt): host code, Duktape (JavaScript, not used), LuaJIT (Lua), Python (in development)
- [Jerry Sievert](https://github.com/JerrySievert): QuickJS (JavaScript)
- [Leonardo Laguna Ruiz](https://github.com/modlfo): Vult
- [CHAIR](https://chair.audio) [Clemens Wegener (libpd), Max Neupert (patches)] : libpd
- add your name here
