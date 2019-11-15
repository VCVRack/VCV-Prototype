// Voltage-controlled oscillator example
// by Andrew Belt

// For audio synthesis and process, JavaScript is 10-100x less efficient than C++, but it's still an easy way to learn to program DSP.

config.frameDivider = 1
config.bufferSize = 16

let phase = 0
function process(block) {
	// Knob ranges from -5 to 5 octaves
	let pitch = block.knobs[0] * 10 - 5
	// Input follows 1V/oct standard
	// Take the first input's first buffer value
	pitch += block.inputs[0][0]

	// The relationship between 1V/oct pitch and frequency is `freq = 2^pitch`.
	// Default frequency is middle C (C4) in Hz.
	// https://vcvrack.com/manual/VoltageStandards.html#pitch-and-frequencies
	let freq = 261.6256 * Math.pow(2, pitch)
	display("Freq: " + freq.toFixed(3) + " Hz")

	// Set all samples in output buffer
	let deltaPhase = config.frameDivider * block.sampleTime * freq
	for (let i = 0; i < block.bufferSize; i++) {
		// Accumulate phase
		phase += deltaPhase
		// Wrap phase around range [0, 1]
		phase %= 1

		// Convert phase to sine output
		block.outputs[0][i] = Math.sin(2 * Math.PI * phase) * 5
	}
}
