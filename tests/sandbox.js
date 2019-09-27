
config.frameDivider = 256

var knobPresets = []
for (var i = 0; i < 6; i++) {
	knobPresets[i] = [0, 0, 0, 0, 0, 0]
}
var lastI = 0

function process(block) {
	for (var j = 0; j < 6; j++) {
		knobPresets[lastI][j] = block.knobs[j]
		block.lights[j][0] = block.knobs[j]
	}
	for (var i = 0; i < 6; i++) {
		if (block.switches[i]) {
			for (var j = 0; j < 6; j++) {
				block.knobs[j] = knobPresets[i][j]
			}
			lastI = i
		}
		block.switchLights[i][0] = (lastI == i) ? 1 : 0
	}
}