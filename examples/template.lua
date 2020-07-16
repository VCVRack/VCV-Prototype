config.frameDivider = 1
config.bufferSize = 32

function process(block)
	-- Per-block inputs:
	-- block.knobs[i]
	-- block.switches[i]

	for j=1,block.bufferSize do
		-- Per-sample inputs:
		-- block.inputs[i][j]

		-- Per-sample outputs:
		-- block.outputs[i][j]
	end

	-- Per-block outputs:
	-- block.lights[i][color]
	-- block.switchLights[i][color]
end
