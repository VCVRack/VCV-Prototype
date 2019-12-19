// Voltage-controlled oscillator example
// by Andrew Belt
// adapted for SC by Brian Heim

~vcv_frameDivider = 1;
~vcv_bufferSize = 32;

~phase = 0;
~vcv_process = { |block|

	var pitch, freq, deltaPhase;

	// Knob ranges from -5 to 5 octaves
	pitch = block.knobs[0] * 10 - 5;
	// Input follows 1V/oct standard
	// Take the first input's first buffer value
	pitch = pitch + block.inputs[0][0];

	// The relationship between 1V/oct pitch and frequency is `freq = 2^pitch`.
	// Default frequency is middle C (C4) in Hz.
	// https://vcvrack.com/manual/VoltageStandards.html#pitch-and-frequencies
	freq = 261.6256 * pow(2, pitch);
	postf("Freq: % Hz", freq);

	deltaPhase = ~vcv_frameDivider * block.sampleTime * freq;

	// Set all samples in output buffer
	block.bufferSize.do { |i|
		// Accumulate phase & wrap around range [0, 1]
		~phase = (~phase + deltaPhase) % 1.0;

		// Convert phase to sine output
		block.outputs[0][i] = sin(2pi * ~phase) * 5;
	};

	block
}
