// Voltage-controlled oscillator example
// by Andrew Belt

// JavaScript isn't ideal for audio generating and processing due to it being 10-100 less efficient than C++, but it's still an easy way to learn simple DSP.

config.frameDivider = 1
config.bufferSize = 16

var phase = 0
function process(block) {
	// Knob ranges from -5 to 5 octaves
	var pitch = block.knobs[0] * 10 - 5
	// Input follows 1V/oct standard
	pitch += block.inputs[0][0]

	// The relationship between 1V/oct pitch and frequency is `freq = 2^pitch`.
	// Default frequency is middle C (C4) in Hz.
	// https://vcvrack.com/manual/VoltageStandards.html#pitch-and-frequencies
	var freq = 261.6256 * Math.pow(2, pitch)
	display("Freq: " + freq.toFixed(3) + " Hz")

	// Set all output samples in block
	var deltaPhase = block.sampleTime * config.frameDivider * freq
	for (var i = 0; i < block.bufferSize; i++) {
		// Accumulate phase
		phase += deltaPhase
		// Wrap phase around range [0, 1]
		phase %= 1

		// Convert phase to sine output
		block.outputs[0][i] = Math.sin(2 * Math.PI * phase) * 5
	}
}