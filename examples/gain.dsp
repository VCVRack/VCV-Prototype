// Simplest possible script using all variables

import("stdfaust.lib");

// Switch button, highlight in red
switch(i) = button("switch%i [switch:%i]") : hbargraph("[switchlight_red:%i]", 0, 1);

// Gain slider, highlight in red
gain(i) = hslider("gain%i [knob:%i]", 0.1, 0, 1, 0.01) : hbargraph("[light_red:%i]", 0, 1);

process(x) = par(i, 6, x * gain(i+1) * (1-switch(i+1)));

