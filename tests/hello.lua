
config.frameDivider = 1
config.bufferSize = 16

function process(block)
	for j=1,block.bufferSize do
		block.outputs[1][j] = math.random()
	end
end

print("Hello, world!")
