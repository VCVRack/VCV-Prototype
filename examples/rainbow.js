// Rainbow RGB LED example
// by Andrew Belt

// Call process() every 256 audio samples
config.frameDivider = 256


// From https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
function hsvToRgb(h, s, v) {
	h *= 6
	let c = v * s
	let x = c * (1 - Math.abs(h % 2 - 1))
	let rgb;
	if (h < 1) rgb = [c, x, 0]
	else if (h < 2) rgb = [x, c, 0]
	else if (h < 3) rgb = [0, c, x]
	else if (h < 4) rgb = [0, x, c]
	else if (h < 5) rgb = [x, 0, c]
	else rgb = [c, 0, x]
	let m = v - c
	rgb[0] += m
	rgb[1] += m
	rgb[2] += m
	return rgb
}


let phase = 0
function process(block) {
	phase += block.sampleTime * config.frameDivider * 0.5
	phase %= 1

	for (let i = 0; i < 6; i++) {
		let h = (1 - i / 6 + phase) % 1
		let rgb = hsvToRgb(h, 1, 1)
		for (let c = 0; c < 3; c++) {
			block.lights[i][c] = rgb[c]
			block.switchLights[i][c] = rgb[c]
		}
		block.outputs[i][0] = Math.sin(2 * Math.PI * h) * 5 + 5
	}
}

display("Hello, world!")
