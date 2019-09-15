config.frameDivider = 1
config.bufferSize = 1

var frame = 0
function process(block) {
	frame += 1
	for (var i = 0; i < 6; i++) {
		for (var j = 0; j < block.bufferSize; j++) {
			var v = block.inputs[i][j]
			v *= block.knobs[i]
			block.outputs[i][j] = v
		}
		// block.lights[i][2] = 1
		// block.switchLights[i][1] = 1
	}

	display(frame)
}

display("Hello, world!")
