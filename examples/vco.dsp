// Voltage-controlled oscillator example

import("stdfaust.lib");

// Create a phasor with a given frequency
phasor(freq) = freq/ma.SR : (+ : decimal) ~ _ with { decimal(x) = x-int(x); };

// Pitch to freq conversion (also included in the rack.lib library)
cv_pitch2freq(cv_pitch) = 440 * 2 ^ (cv_pitch - 0.75);

gain = hslider("gain [knob:1]", 0.1, 0, 1, 0.01) * 10 - 5;

pitch(x) = x + gain;

process(x) = sin(2 * ma.PI * phasor(cv_pitch2freq(pitch(x)))) * 5;

