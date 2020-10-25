// Simplest possible script using all variables, demonstrating buffering
// by Andrew Belt
// adapted for SC by Brian Heim

~vcv_frameDivider = 1;
~vcv_bufferSize = 32;

~vcv_process = { |block|
	// Loop through each row
	VcvPrototypeProcessBlock.numRows.do { |i|
		// Get gain knob
		var gain = block.knobs[i];
		// Set gain light (red = 0)
		block.lights[i][0] = gain;

		// Check mute switch
		block.switchLights[i][0] = if (block.switches[i]) {
			// Mute output
			gain = 0;
			// Enable mute light (red = 0)
			1
		} {
			// Disable mute light
			0
		};

		// Iterate input/output buffer
		block.outputs[i] = block.inputs[i] * gain;
	};

	block
}
