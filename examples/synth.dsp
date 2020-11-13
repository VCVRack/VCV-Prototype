/*
 All controllers of the VCV Prototype are accessed using metadata.
 */

import("stdfaust.lib");
import("rack.lib");

// Using knobs ([knob:N] metadata with N from 1 to 6). Knob [0..1] range is mapped on [min..max] slider range, taking 'scale' metadata in account
vol1 = hslider("volume1 [knob:1]", 0.1, 0, 1, 0.01);
freq1 = hslider("freq1 [knob:2] [unit:Hz] [scale:lin]", 300, 200, 300, 1);

vol2 = hslider("volume2 [knob:3]", 0.1, 0, 1, 0.01);
freq2 = hslider("freq2 [knob:4] [unit:Hz] ", 300, 200, 300, 1);

// Using switches ([switch:N] metadata with N from 1 to 6)
gate = button("gate [switch:1]") : vbargraph("[switchlight_red:1]", 0, 1);

// Checkbox can be used, the switch button will be white when checked
check = checkbox("check [switch:2]") 
	: vbargraph("[switchlight_red:2]", 0, 1) 
	: vbargraph("[switchlight_green:2]", 0, 1)
	: vbargraph("[switchlight_blue:2]", 0, 1);

// Using bargraph to control lights ([light_red:N|light_green:N|light_blue:N] metadata with N from 1 to 6, to control 3 colors)
l_red(i) = vbargraph("[light_red:%i]", 0, 1);
l_green(i) = vbargraph("[light_green:%i]", 0, 1);
l_blue(i) = vbargraph("[light_blue:%i]", 0, 1);

// Using bargraph to control switchlights ([switchlight_red:N|switchlight_green:N|switchlight_blue:N] metadata with N from 1 to 6, to control 3 colors)
swl_red(i) = vbargraph("[switchlight_red:%i]", 0, 1);
swl_green(i) = vbargraph("[switchlight_green:%i]", 0, 1);
swl_blue(i) = vbargraph("[switchlight_blue:%i]", 0, 1);

process = os.osc(freq1) * vol1 * 5, os.sawtooth(freq2) * vol2 * gate * 5, os.square(freq2) * vol2 * check * 5;
