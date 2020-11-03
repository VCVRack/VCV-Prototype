// Import libraries
import("stdfaust.lib");
import("rack.lib");

// Switch button
switch(i) = button("switch%i [switch:%i]");

// Param slider
param(i) = hslider("param%i [knob:%i]", 0.1, 0, 1, 0.01);

// Highlight
l_red(i) = hbargraph("[light_red:%i]", 0, 1);
l_green(i) = hbargraph("[light_green:%i]", 0, 1);
l_blue(i) = hbargraph("[light_blue:%i]", 0, 1);

swl_red(i) = hbargraph("[switchlight_red:%i]", 0, 1);
swl_green(i) = hbargraph("[switchlight_green:%i]", 0, 1);
swl_blue(i) = hbargraph("[switchlight_blue:%i]", 0, 1);

// Process definition
process = par(i, 6, _);

