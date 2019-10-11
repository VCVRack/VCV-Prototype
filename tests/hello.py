frame = 0

print(config)
print(config.frameDivider)
print(config.bufferSize)

def process(block):
	global frame
	frame += 1
	display(frame)
	# print(block)
	# block.switch_lights[:, 2] = 1
	print(block.inputs)
	print(block.outputs)
	print(block.knobs)
	print(block.switches)
	print(block.lights)
	print(block.switch_lights)
	# print("===")

print("Hello, world!")
