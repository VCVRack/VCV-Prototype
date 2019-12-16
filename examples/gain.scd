// Simplest possible script using all variables, demonstrating buffering
// by Brian Heim

~i = 0;
a = 0;
// ~process = {|x| a = max(a, bench { 12.do { 256.do { |i| sin(i)} }; post(x); }); post(a) }
~vcv_bufferSize = 64;
~vcv_frameDivider = 1;

~text = "Hi! I am the first working proof of concept of the SuperCollider prototyper for VCV Rack!";
~textPos = 0;
~textWin = 22;
~counter = 1;
~text = String.fill(~textWin, $~) ++ ~text ++ String.fill(~textWin, $~);
~textLen = ~text.size - ~textWin;

~nSamp = 100;
~phases = ~nSamp.collect { |i| sin(i / ~nSamp * 2pi); };
~vcv_process = {|block|
	block.bufferSize.do { |i|
		block.outputs[0][i] = ~phases[~i] * block.knobs[0];
		~i = ~i + block.knobs[1];
		~i = ~i mod: ~nSamp;

		block.outputs[1][i] = block.inputs[0][i] * block.outputs[0][i];
		block.outputs[1][i] = block.outputs[1][i].squared;
	};

	block.switchLights[3][0] = 1.0.rand();
	block.switchLights[3][1] = 1.0.rand();
	block.switchLights[3][2] = 1.0.rand();

	~counter = ~counter + 1;
	if (~counter == 100) {
		(~text[~textPos..(~textPos+~textWin)]).post;
		~textPos = ~textPos + 1;
		~textPos = ~textPos mod: ~textLen;
		~counter = 0;
	};


	block.knobs[4] = a / ~nSamp;
	a = a + 0.15;
	a = a mod: ~nSamp;

	block.lights[2][2] = a * ~i / 1000;

	block.switchLights[5][0] = block.switches[2].if { rand(3/4) } { 0 };
	block.outputs[0][3] = block.switches[1].if { rand(block.knobs[1]) } { block.outputs[0][3] };
	block
}
