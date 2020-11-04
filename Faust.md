# Using Faust DSP language in VCV Prototype 

The [Faust audio DSP language](https://faust.grame.fr) can be used in VCV Prototype. The compiler can be embedded in applications or plugins using [libfaust](https://faustdoc.grame.fr/manual/embedding/), and DSP code can be edited and JIT compiled on the fly.

## Compilation

- type `make && make install` to build it
- you can now add a Faust aware VCV Prototype in your Rack session and start coding in Faust

## Loading/editing/compiling .dsp files

Faust DSP files have to be loaded in VCV Prototype and edited in a external editor (Visual Studio Code, Atom...). Each time the file is saved, it will be recompiled and executed. To possibly save compilation time, the DSP machine code is saved in a cache, and possibly restored the next time the session will be loaded.

The 6 *switches*, *knobs* as well as the *lights* and *switchLights* can be connected to UI controllers using metadata:

- `[switch:N]` (with N from 1 to 6) has to be used in a `button` or `checkbox` item to connect it to the prototype interface switch number N. Pushed buttons will become red  when on, and checkboxes will become white when on.
- `[knob:N]` (with N from 1 to 6) has to be used in a `vslider`, `hslider`  or `nentry` item to connect it to the prototype interface knob number N. The knob [0..1] range will be mapped to the slider/nentry [min..max] range.
- `[light_red:N|light_green:N|light_blue:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` item to connect it to the prototype interface light number N.
- `[switchlight_red:N|switchlight_green:N|switchlight_blue:N]` (with N from 1 to 6) has to be used in a `vbargraph` or `hbargraph` to connect it to the prototype interface switchLight number N.

Other metadata:
- `[scale:lin|log|exp]` metadata is implemented.

The [res/faust/rack.lib](https://github.com/VCVRack/VCV-Prototype/blob/faust/res/faust/rack.lib) Faust library contains usefull functions to convert VC signals, and can be enriched if needed. 

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

- [synth.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/synth.dsp) demonstrates how to use all different VCV Prototype UI items
- [organ.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/organ.dsp) demonstrates a MIDI controllable additive synthesis based organ
- [physicalmodel.dsp](https://github.com/VCVRack/VCV-Prototype/blob/faust/examples/physicalmodel.dsp) demonstrates a modal synthesis based bell connected to a reverb
