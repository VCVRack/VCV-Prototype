// Simplest possible script using all variables

import("stdfaust.lib");

// Switch button
switch(i) = button("switch%i [switch:%i]");

// Highlight in red
light_red(i) = hbargraph("[light_red:%i]", 0, 1);

// Gain slider
gain(i) = hslider("gain%i [knob:%i]", 0.1, 0, 1, 0.01);

process(x) = par(i, 6, x * gain(i+1) * (1-switch(i+1)) : light_red(i+1));

