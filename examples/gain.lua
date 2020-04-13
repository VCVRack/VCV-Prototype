-- Simplest possible script using all variables, demonstrating buffering
-- by Andrew Belt

config.frameDivider = 1
config.bufferSize = 32

function process(block)
	-- Loop through each column
	for i=1,6 do
		-- Get gain knob
		gain = block.knobs[i]
		-- Set gain light (red = 1)
		block.lights[i][1] = gain
		-- Check mute switch
		if block.switches[i] then
			-- Mute output
			gain = 0
			-- Enable mute light (red = 1)
			block.switchLights[i][1] = 1
		else
			-- Disable mute light
			block.switchLights[i][1] = 0
		end
		-- Iterate input/output buffer
		for j=1,block.bufferSize do
			-- Get input
			x = block.inputs[i][j]
			-- Apply gain to input
			y = x * gain
			-- Set output
			block.outputs[i][j] = y
		end
	end
end
