# VCV Prototype

Scripting language host for [VCV Rack](https://vcvrack.com/) containing:
- 6 inputs
- 6 outputs
- 6 knobs
- 6 lights (RGB LEDs)
- 6 switches with RGB LEDs

[Discussion thread](https://community.vcvrack.com/t/vcv-prototype/3271/1)

## Scripting API

This is the reference API for the JavaScript script engine, along with default property values.
Other script engines may vary in their syntax (e.g. `args.inputs[i][j]` vs `args.getInput(i, j)` vs `input(i, j)`), but the functionality should be similar.

```js
/** Display message on LED display.
*/
display(message)

/** Skip this many sample frames before running process().
For CV generators and processors, 256 is reasonable.
For sequencers, 32 is reasonable since process() will be called every 0.7ms with a 44100kHz sample rate, which will capture 1ms-long triggers.
For audio generators and processors, 1-8 is recommended, but it will consume lots of CPU.
If this is too slow for your purposes, consider writing a C++ plugin, since native VCV Rack plugins are 10-100 faster.
*/
config.frameDivider // 32

/** Called when the next args is ready to be processed.
*/
function process(args) {
	/** Engine sample rate in Hz. Read-only.
	*/
	args.sampleRate

	/** Engine sample timestep in seconds. Equal to `1 / sampleRate`. Read-only.
	*/
	args.sampleTime

	/** Voltage of the input port of row `i`. Read-only.
	*/
	args.inputs[i] // 0.0

	/** Voltage of the output port of row `i`. Writable.
	*/
	args.outputs[i] // 0.0

	/** Value of the knob of row `i`. Between 0 and 1. Read-only.
	*/
	args.knobs[i] // 0.0

	/** Pressed state of the switch of row `i`. Read-only.
	*/
	args.switches[i] // false

	/** Brightness of the RGB LED of row `i`. Writable.
	*/
	args.lights[i].r // 0.0
	args.lights[i].g // 0.0
	args.lights[i].b // 0.0

	/** Brightness of the switch RGB LED of row `i`. Writable.
	*/
	args.switchLights[i].r // 0.0
	args.switchLights[i].g // 0.0
	args.switchLights[i].b // 0.0
}
```

## Adding a script engine

- Add your scripting language library to the build system so it builds with `make dep`, following the Duktape example in the `Makefile`.
- Create a `MyEngine.cpp` file (for example) in `src/` with a `ScriptEngine` subclass defining the virtual methods, possibly using `src/DuktapeEngine.cpp` as an example.
- Add your engine to the "List of ScriptEngines" in `src/ScriptEngine.cpp`.
- Build and test the plugin.
- Add a few example scripts and tests to `examples/`. These will be included in the plugin package for the user.
- Add your name to the Contributers list below.
- Send a pull request. Once merged, you will be added as a repo maintainer. Be sure to "watch" this repo to be notified of bugs in your engine.

## Maintainers

- [Wes Milholen](https://grayscale.info/): panel design
- [Andrew Belt](https://github.com/AndrewBelt): host code, Duktape (JavaScript)
- add your name here

## License

All **source code** is copyright © 2019 VCV Prototype Maintainers and licensed under the [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause).

The **panel graphics** in the `res` directory are copyright © 2019 [Grayscale](http://grayscale.info/) and licensed under [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
You may not distribute modified adaptations of these graphics.

**Dependencies** included in the binary distributable may have other licenses.
For example, if a GPL library is included in the distributable, the entire work is covered by the GPL.
See [LICENSE-dist.txt](LICENSE-dist.txt) for a full list.