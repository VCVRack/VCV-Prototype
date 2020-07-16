config.frameDivider = 1
config.bufferSize = 32

function process(block) {
	// Per-block inputs:
	// block.knobs[i]
	// block.switches[i]

	for (let j = 0; j < block.bufferSize; j++) {
		// Per-sample inputs:
		// block.inputs[i][j]

		// Per-sample outputs:
		// block.outputs[i][j]
	}

	// Per-block outputs:
	// block.lights[i][color]
	// block.switchLights[i][color]
}
