-- Rainbow RGB LED example
-- by Andrew Belt

-- Call process() every 256 audio samples
config.frameDivider = 256


-- From https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
function hsvToRgb(h, s, v)
	h = h * 6
	c = v * s
	x = c * (1 - math.abs(h % 2 - 1))
	if (h < 1) then rgb = {c, x, 0}
	elseif (h < 2) then rgb = {x, c, 0}
	elseif (h < 3) then rgb = {0, c, x}
	elseif (h < 4) then rgb = {0, x, c}
	elseif (h < 5) then rgb = {x, 0, c}
	else rgb = {c, 0, x}
	end
	m = v - c
	rgb[1] = rgb[1] + m
	rgb[2] = rgb[2] + m
	rgb[3] = rgb[3] + m
	return rgb
end


phase = 0
function process(block)
	phase = phase + block.sampleTime * config.frameDivider * 0.5
	phase = phase % 1

	for i=1,6 do
		h = (1 - i / 6 + phase) % 1
		rgb = hsvToRgb(h, 1, 1)
		for c=1,3 do
			block.lights[i-1][c-1] = rgb[c]
			block.switchLights[i-1][c-1] = rgb[c]
		end
		block.outputs[i-1][0] = math.sin(2 * math.pi * h) * 5 + 5
	end
end

display("Hello, world!")
