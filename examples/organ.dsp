import("stdfaust.lib");
import("rack.lib");

vol = hslider("vol [knob:1]", 0.3, 0, 10, 0.01);	
pan = hslider("pan [knob:2]", 0.5, 0, 1, 0.01);	

attack = hslider("attack", 0.01, 0, 1, 0.001);	
decay = hslider("decay", 0.3, 0, 1, 0.001);	
sustain = hslider("sustain", 0.5, 0, 1, 0.01);	
release = hslider("release", 0.2, 0, 1, 0.001);	

panner(c) = _ <: *(1-c), *(c);

voice(freq) = os.osc(freq) + 0.5*os.osc(2*freq) + 0.25*os.osc(3*freq);

/*
Additive synth: 3 sine oscillators with adsr envelop.
Use the 3 first VC inputs to control pitch, gate and velocity.
*/

process(pitch, gate, vel) = voice(freq) * en.adsr(attack, decay, sustain, release, gate) * vel : *(vol) : panner(pan)
with {
	freq = cv_pitch2freq(pitch);	
};