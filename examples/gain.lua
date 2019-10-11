-- Simplest possible script using all variables
-- by Andrew Belt

function process(block)
	-- Loop through each column
	for i=1,6 do
		-- Get input
		x = block.inputs[i][1]
		-- Get gain knob
		gain = block.knobs[i]
		-- Apply gain to input
		y = x * gain
		-- Set gain light (red = 0)
		block.lights[i][1] = gain
		-- Check mute switch
		if block.switches[i] then
			-- Mute output
			y = 0
			-- Enable mute light (red = 0)
			block.switchLights[i][1] = 1
		else
			-- Disable mute light
			block.switchLights[i][1] = 0
		end
		-- Set output
		block.outputs[i][1] = y
	end
end
