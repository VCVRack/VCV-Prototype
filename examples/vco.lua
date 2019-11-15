-- Voltage-controlled oscillator example
-- by Andrew Belt

-- For audio synthesis and process, Lua is 10-100x less efficient than C++, but it's still an easy way to learn to program DSP.

config.frameDivider = 1
config.bufferSize = 16

phase = 0
function process(block)
	-- Knob ranges from -5 to 5 octaves
	pitch = block.knobs[1] * 10 - 5
	-- Input follows 1V/oct standard
	-- Take the first input's first buffer value
	pitch = pitch + block.inputs[1][1]

	-- The relationship between 1V/oct pitch and frequency is `freq = 2^pitch`.
	-- Default frequency is middle C (C4) in Hz.
	-- https://vcvrack.com/manual/VoltageStandards.html#pitch-and-frequencies
	freq = 261.6256 * math.pow(2, pitch)
	display("Freq: " .. string.format("%.3f", freq) .. " Hz")

	-- Set all samples in output buffer
	deltaPhase = config.frameDivider * block.sampleTime * freq
	for i=1,block.bufferSize do
		-- Accumulate phase
		phase = phase + deltaPhase
		-- Wrap phase around range [0, 1]
		phase = phase % 1

		-- Convert phase to sine output
		block.outputs[1][i] = math.sin(2 * math.pi * phase) * 5
	end
end