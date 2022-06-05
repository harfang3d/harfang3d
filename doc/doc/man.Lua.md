.title Harfang for Lua

Harfang for Lua is distributed as a dynamic library compatible with official builds of the Lua interpreter version 5.3.

## Installation

The mecanisms used by the Lua interpreter to locate binary extensions are detailed in the [Lua 5.3 Reference Manual](https://www.lua.org/manual/5.3/manual.html#pdf-LUA_CPATH).

- On Windows, deploying the Harfang binaries alongside the interpreter executable will usually suffice.
- On other platforms, you may have to manually modify the interpreter CPath variable to include the path to the Harfang binaries.

## Testing your Installation

Start the Lua interpreter and type `hg = require("harfang")`, you should get an output similar to the following:

```text
Harfang 2.0.0 for Lua 5.3 on windows-x64 (build ba08463ee9e6c0c93960230fb880c1d9b230610d Sep 30 2020 16:08:22)
See https://www.harfang3d.com/license for licensing terms
```

## First Program

Let's write a simple test program, create a new file named `test.lua` and paste the following code into it.

```lua
hg = require('harfang')

hg.InputInit()
hg.WindowSystemInit()

win = hg.RenderInit(400, 300, hg.RF_None)

while not hg.ReadKeyboard():Key(hg.K_Escape) do
	hg.SetViewRect(0, 0, 0, 400, 300)
	hg.SetViewClear(0, hg.CF_Color, hg.RGBA32(96, 32, 255))

	hg.Touch(0)

	hg.Frame()
	hg.UpdateWindow(win)
end

hg.DestroyWindow(win)
```

When executed, this program demonstrates how to create an output window, initialize the input and render systems and paint the whole window in blue until the escape key is pressed.

In order to draw anything on screen we will need to create at least a shader and compile it before using it in our program, this process is documented in the [man.Assets] page.

[man.Quickstart] documents how to quickly get the more interesting tutorial programs running.