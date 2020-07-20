import("stdfaust.lib");
import("rack.lib");

frenchBell_ui = pm.frenchBell(strikePosition,strikeCutoff,strikeSharpness,gain,gate)
with {
  	strikePosition = nentry("v:frenchBell/[0]strikePosition", 0,0,4,1);
  	strikeCutoff = hslider("v:frenchBell/[1]strikeCutOff", 6500,20,20000,1);
  	strikeSharpness = hslider("v:frenchBell/[2]strikeSharpness", 0.5,0.01,5,0.01);
  	
  	// Connection with VCV knob and switch
	gain = hslider("v:frenchBell/[3]gain [knob:1]",1,0,1,0.01);
	gate = button("v:frenchBell/[4]gate [switch:1]");
};

freeverb_demo = _,_ <: (*(g)*fixedgain,*(g)*fixedgain : 
	re.stereo_freeverb(combfeed, allpassfeed, damping, spatSpread)), 
	*(1-g), *(1-g) :> _,_
with{
	scaleroom   = 0.28;
	offsetroom  = 0.7;
	allpassfeed = 0.5;
	scaledamp   = 0.4;
	fixedgain   = 0.1;
	origSR = 44100;

	parameters(x) = hgroup("Freeverb",x);
	knobGroup(x) = parameters(vgroup("[0]",x));
	
	// Connection with VCV knobs
	damping = knobGroup(vslider("[0] Damp [knob:2] [style: knob] [tooltip: Somehow control the 
		density of the reverb.]",0.5, 0, 1, 0.025)*scaledamp*origSR/ma.SR);
		
	combfeed = knobGroup(vslider("[1] RoomSize [knob:3] [style: knob] [tooltip: The room size 
		between 0 and 1 with 1 for the largest room.]", 0.5, 0, 1, 0.025)*scaleroom*
		origSR/ma.SR + offsetroom);
		
	spatSpread = knobGroup(vslider("[2] Stereo Spread [knob:4] [style: knob] [tooltip: Spatial 
		spread between 0 and 1 with 1 for maximum spread.]",0.5,0,1,0.01)*46*ma.SR/origSR 
		: int);
	g = parameters(vslider("[1] Wet [knob:5] [tooltip: The amount of reverb applied to the signal 
		between 0 and 1 with 1 for the maximum amount of reverb.]", 0.3333, 0, 1, 0.025));
};

process = frenchBell_ui <: freeverb_demo;
