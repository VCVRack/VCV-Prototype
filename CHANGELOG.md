### 1.1.1 (in development)
- Add "Save script as" in panel context menu.
- Allow script to be loaded by dragging-and-dropping on the panel.

### 1.1.0 (2019-09-15)
- Add block buffering with `config.bufferSize`.
	- Scripts must change `inputs[i]` to `inputs[i][0]` and `outputs[i]` to `outputs[i][0]`.
- Allow multiple lines of text in LED display.
- Duktape (JavaScript):
	- Use array instead of object for RGB lights and switch lights.
		- Scripts must change `.r` to `[0]`, `.g` to `[1]`, and `.b` to `[2]` for `lights` and `switchLights`.
	- Use ArrayBuffers instead of arrays, improving performance \~5x.

### 1.0.0 (2019-09-15)
- Initial release.