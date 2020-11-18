# Using Faust DSP language in VCV Prototype 

The [Faust audio DSP language](https://faust.grame.fr) can be used in VCV Prototype. The compiler can be embedded in applications or plugins using [libfaust](https://faustdoc.grame.fr/manual/embedding/), and DSP code can be edited and JIT compiled on the fly.

## Compilation and installation

- type `make dep && make && make install`  to build and install the module
- you can now add a Faust aware VCV Prototype in your Rack session and start coding in Faust

## Loading/editing/compiling DSP files

Faust DSP files have to be loaded in VCV Prototype and edited in a external editor (Visual Studio Code, Atom...). Each time the file is saved, it will be recompiled and executed. To possibly save compilation time, the DSP machine code is saved in a cache, and possibly restored the next time the session will be loaded.

The 6 audio inputs/outputs can be accessed in order in the Faust DSP which can thus use up to 6 channels.

The 6 *switches*, *knobs* as well as the *lights* and *switchLights* can be connected to UI controllers using metadata:

- `[switch:N]` (with N from 1 to 6) has to be used in a `button` or `checkbox` item to connect it to the prototype interface switch number N
- `[knob:N]` (with N from 1 to 6) has to be used in a `vslider`, `hslider`  or `nentry` item to connect it to the prototype interface knob number N. The knob [0..1] range will be mapped to the slider/nentry [min..max] range
- `[light_red:N|light_green:N|light_blue:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` item to connect it to the prototype interface light number N
- `[switchlight_red:N|switchlight_green:N|switchlight_blue:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` to connect it to the prototype interface switchLight number N 

So a button or checkbox UI item can use the `[switch:N]` metadata to be associated with the corresponding GUI switch, which color can be controlled using the `switchlight_xx:N` metadata. For instance:  
- `gate = button("gate [switch:1") : hbargraph("[switchlight_red:1]", 0, 1);` can be written to describe a button which become red when pressed
- `check = checkbox("check [switch:2]") : vbargraph("[switchlight_red:2]", 0, 1) : vbargraph("[switchlight_green:2]", 0, 1) : vbargraph("[switchlight_blue:2]", 0, 1);` can be written to describe a checkbox which become white when checked

Other metadata:
- `[scale:lin|log|exp]` metadata is implemented.

The [rack.lib](https://github.com/VCVRack/VCV-Prototype/blob/faust/res/faust/rack.lib) Faust library contains usefull functions to convert CV signals, and can be enriched if needed. 

## DSP examples

Here is a simple example showing how oscillators can be controlled by GUI items, associated with metadata in the DSP code:

```
import("stdfaust.lib");

// UI controllers connected using metadata
freq = hslider("freq [knob:1]", 200, 50, 5000, 0.01);
gain = hslider("gain [knob:2]", 0.5, 0, 1, 0.01);
gate = button("gate [switch:1]");

// DSP processor
process = os.osc(freq) * gain * 5, os.sawtooth(freq) * gain * gate * 5;
```

Following the VCV Prototype model, note that audio outputs **are multipled by 5** to follow the [-5v..5v] range convention. 

The standard examples ported to Faust can be seen in the examples folder:

- [gain.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/gain.dsp)
- [rainbow.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/rainbow.dsp) 
- [vco.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/vco.dsp)

Some additional examples:

- [synth.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/synth.dsp) demonstrates how to use all different VCV Prototype UI items
- [organ.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/organ.dsp) demonstrates a MIDI controllable additive synthesis based organ
- [physicalmodel.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/physicalmodel.dsp) demonstrates a modal synthesis based bell connected to a reverb


