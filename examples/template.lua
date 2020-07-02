config.frameDivider = 1
config.bufferSize = 32

function process(block)
	for j=1,block.bufferSize do
		-- Inputs
		-- block.inputs[i][j]
		-- block.knobs[i]
		-- block.switches[i]

		-- Outputs
		-- block.outputs[i][j]
		-- block.lights[i][color]
		-- block.switchLights[i][color]
	end
end
