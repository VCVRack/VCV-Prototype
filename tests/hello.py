frame = 0

def process(block):
	global frame
	frame += 1
	print(frame)
	# print(block)
	# block.switch_lights[:, 2] = 1
	print(block.inputs)
	print(block.outputs)
	print(block.knobs)
	print(block.switches)
	print(block.lights)
	print(block.switch_lights)
	print("===")

print("Hello, world!")
