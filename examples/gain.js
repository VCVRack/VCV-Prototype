// Simplest possible script using all variables, demonstrating buffering
// by Andrew Belt

config.frameDivider = 1
config.bufferSize = 32

function process(block) {
	// Loop through each column
	for (let i = 0; i < 6; i++) {
		// Get gain knob
		let gain = block.knobs[i]
		// Set gain light (red = 0)
		block.lights[i][0] = gain
		// Check mute switch
		if (block.switches[i]) {
			// Mute output
			gain = 0
			// Enable mute light (red = 0)
			block.switchLights[i][0] = 1
		}
		else {
			// Disable mute light
			block.switchLights[i][0] = 0
		}
		// Iterate input/output buffer
		for (let j = 0; j < block.bufferSize; j++) {
			// Get input
			let x = block.inputs[i][j]
			// Apply gain to input
			let y = x * gain
			// Set output
			block.outputs[i][j] = y
		}
	}
}
