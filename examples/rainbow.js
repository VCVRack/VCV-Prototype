config.frameDivider = 1

function process(block) {
	for (var i = 0; i < 6; i++) {
		var v = block.inputs[i]
		v *= block.knobs[i]
		block.outputs[i] = v
		block.lights[i].r = 1
		block.switchLights[i].g = 1
		block.switchLights[i].b = 1
	}
}

display("Hello, world!")
