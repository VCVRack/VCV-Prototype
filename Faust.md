# Using Faust DSP language in VCV Prototype 

The [Faust audio DSP language](https://faust.grame.fr) can be used in VCV Prototype. The compiler can be embedded in applications or plugins using [libfaust](https://faustdoc.grame.fr/manual/embedding/), and DSP code can be edited and JIT compiled on the fly.

## Compiling on OSX

The Faust *libfaust* dynamic library has to be available in your system. You can get is from a precompiled Faust release (simpler), or compile it yourself (harder). Then the [Faust aware VCV Prototype](https://github.com/sletz/VCV-Prototype) can be compiled and installed.

### Installing the *libfaust* dynamic library from a precompiled package:

- go on Faust [release page](https://github.com/grame-cncm/faust/releases) and get the latest .dmg for OSX.
- mount the disk image and copy the 3 *libfaust.dylib*, *libfaust.2.dylib* and *libfaust.x.y.z.dylib* files located in the disk image */lib* folder in your system */usr/local/lib* folder

### Compiling the *libfaust* dynamic library:

- clone the [Faust GitHub project](https://github.com/grame-cncm/faust)
- assuming you have installed the LLVM dependency (with brew or macports on OSX), type `make world` to compile the libfaust libray in dynamic mode (read https://github.com/grame-cncm/faust/wiki/Building if needed)

### Compiling the VCV Prototype:

- clone the [Faust aware VCV Prototype](https://github.com/sletz/VCV-Prototype)  then type `make && make install` to build it
- you can now add a Faust aware VCV Prototype in your Rack session and start coding in Faust.

## Loading/editing/compiling .dsp files

Faust DSP files have to be loaded in VCV Prototype and edited in a external editor (Visual Studio Code, Atom...). Each time the file is saved, it will be recompiled and executed. To possibly save compilation time, the DSP machine code is saved in a cache, and possibly restored the next time the session will be loaded.

The 6 *switches*, *knobs* as well as the *lights* and *switchLights* can be connected to UI controllers using metadata:

- `[switch:N]` (with N from 1 to 6) has to be used in a `button` or `checkbox` item to connect it to the prototype interface switch number N.
- `[knob:N]` (with N from 1 to 6) has to be used in a `vslider`, `hslider`  or `nentry` item to connect it to the prototype interface knob number N.
- `[light_red|green|red:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` item to connect it to the prototype interface light number N.
- `[switchlight_red|green|red:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` to connect it to the prototype interface switchLight number N.

The [faust_libraries/rack.lib](https://github.com/sletz/VCV-Prototype/blob/master/faust_libraries/rack.lib) Faust library contains usefull functions to convert VC signals, and can be enriched if needed. 

## DSP examples

Here is a simple example showing how oscillators can be controlled by UI items:

```
import("stdfaust.lib");

// UI controllers connected using metadata
freq = hslider("freq [knob:1]", 200, 50, 5000, 0.01);
gain = hslider("gain [knob:2]", 0.5, 0, 1, 0.01);
gate = button("gate [switch:1]");

// DSP processor
process = os.osc(freq) * gain, os.sawtooth(freq) * gain * gate;
```

Some additional files can be seen in the examples folder:

- [synth.dsp](https://github.com/sletz/VCV-Prototype/blob/master/examples/synth.dsp) demonstrates how to use all different VCV Prototype UI items
- [organ.dsp](https://github.com/sletz/VCV-Prototype/blob/master/examples/organ.dsp) demonstrates a MIDI controllable additive synthesis based organ
- [physicalmodel.dsp](https://github.com/sletz/VCV-Prototype/blob/master/examples/physicalmodel.dsp) demonstrates a modal synthesis based bell connected to a reverb
