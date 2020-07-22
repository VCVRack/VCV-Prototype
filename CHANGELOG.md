### 1.3.0 (in development)
- Add libpd (Pure Data) engine.
- Add Vult engine.
- Add "New script" and "Edit script" menu items to make script creation easier.
- Allow copying display message to clipboard.
- Improve LuaJIT performance by enabling JIT compilation.
- Add `bit` library to LuaJIT engine.

### 1.2.0 (2019-11-15)
- Add Lua script engine.
- Add security warning when loading script from patch or module preset.

### 1.1.1 (2019-09-27)
- Switch JavaScript engine to QuickJS from Duktape. Supports ES2020 and is ~2x faster.
- Automatically reload script when script file changes.
- Make knobs writable.
- Allow script to be loaded by dragging-and-dropping on the panel.
- Add "Save script as" in panel context menu.
- Reload script on initialize.

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
