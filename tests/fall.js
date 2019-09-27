config.frameDivider = 1
config.bufferSize = 32

var v = 0
function process(block) {
	for (var i = 0; i < block.bufferSize; i++) {
		block.outputs[0][i] = v
		v -= block.sampleTime * 0.1
	}
}