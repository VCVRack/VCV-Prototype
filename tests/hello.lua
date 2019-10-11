
config.frameDivider = 1
config.bufferSize = 16

function process(block)
	for i=1,6 do
		for j=1,block.bufferSize do
			block.outputs[i][j] = block.inputs[i][j]
		end
	end
end

print("Hello, world!")
