// Rainbow RGB LED example
// by Andrew Belt
// adapted for SC by Brian Heim

// Call process() every 256 audio samples
~vcv_frameDivider = 256;
~vcv_bufferSize = 1;

// From https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
~hsvToRgb = { |h, s, v|
	var c, x, rgb, m;
	h = h * 6;
	c = v * s;
	x = c * (1 - abs(h % 2 - 1));
	rgb = case
		{ h < 1 } { [c, x, 0] }
		{ h < 2 } { [x, c, 0] }
		{ h < 3 } { [0, c, x] }
		{ h < 4 } { [0, x, c] }
		{ h < 5 } { [x, 0, c] }
		{ [c, 0, x] };

	rgb + (v - c);
};

~phase = 0;
~vcv_process = { |block|
	~phase = ~phase + block.sampleTime * ~vcv_frameDivider * 0.5;
	~phase = ~phase % 1.0;

	VcvPrototypeProcessBlock.numRows.do { |i|
		var h = (1 - i / 6 + ~phase) % 1;
		var rgb = ~hsvToRgb.value(h, 1, 1);
		3.do { |c|
			block.lights[i][c] = rgb[c];
			block.switchLights[i][c] = rgb[c];
		};
		block.outputs[i][0] = sin(2pi * h) * 5 + 5;
	};

	block
}
