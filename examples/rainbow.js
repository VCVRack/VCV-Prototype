// Call process() every 256 audio samples
config.frameDivider = 256


// From https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
function hsvToRgb(h, s, v) {
	h *= 6
	var c = v * s
	var x = c * (1 - Math.abs(h % 2 - 1))
	var rgb;
	if (h < 1) rgb = [c, x, 0]
	else if (h < 2) rgb = [x, c, 0]
	else if (h < 3) rgb = [0, c, x]
	else if (h < 4) rgb = [0, x, c]
	else if (h < 5) rgb = [x, 0, c]
	else rgb = [c, 0, x]
	var m = v - c
	return {r: rgb[0] + m, g: rgb[1] + m, b: rgb[2] + m}
}


var phase = 0
function process(args) {
	phase += args.sampleTime * config.frameDivider * 0.5
	phase %= 1

	for (var i = 0; i < 6; i++) {
		var h = (1 - i / 6 + phase) % 1
		var rgb = hsvToRgb(h, 1, 1)
		args.lights[i] = rgb
		args.switchLights[i] = rgb
		args.outputs[i] = Math.sin(2 * Math.PI * h) * 5 + 5
	}
}

display("Hello, world!")
