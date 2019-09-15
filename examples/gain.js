// Simplest possible script using all variables
// by Andrew Belt

function process(block) {
	// Loop through each column
	for (var i = 0; i < 6; i++) {
		// Get input
		var x = block.inputs[i][0]
		// Get gain knob
		var gain = block.knobs[i]
		// Apply gain to input
		var y = x * gain
		// Set gain light (red = 0)
		block.lights[i][0] = gain
		// Check mute switch
		if (block.switches[i]) {
			// Mute output
			y = 0
			// Enable mute light (red = 0)
			block.switchLights[i][0] = 1
		}
		else {
			// Disable mute light
			block.switchLights[i][0] = 0
		}
		// Set output
		block.outputs[i][0] = y
	}
}
