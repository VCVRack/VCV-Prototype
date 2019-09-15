
### 1.1.0 (2019-09-15)
- Add block buffering with `config.bufferSize`.
	- Scripts must change `inputs[i]` to `inputs[i][0]` and `outputs[i]` to `outputs[i][0]`.
- Duktape (JavaScript):
	- Use array instead of object for RGB lights and switch lights.
		- Scripts must change `.r` to `[0]`, `.g` to `[1]`, and `.b` to `[2]` for `lights` and `switchLights`.

### 1.0.0 (2019-09-15)
- Initial release.