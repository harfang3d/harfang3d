.title Harfang for CPython

Harfang for Python is distributed as a wheel package `(.whl)` compatible with official builds of the CPython interpreter version 3.4 or newer.

## Installation

* On Windows, install from the command line by typing `pip install harfang`.
* On Linux, download the wheel for your distribution then install from a terminal by typing `pip install /path/to/harfang.whl`.

## Testing your Installation

Start the python interpreter and type `import harfang`, you should get an output similar to the following:

```text
Harfang 2.0.0 for CPython 3.2+ on windows-x64 (build ba08463ee9e6c0c93960230fb880c1d9b230610d Sep 30 2020 16:08:22)
See https://www.harfang3d.com/license for licensing terms
```

### Troubleshooting

> _`pip install` fails with a message saying `harfang is not a supported wheel on this platform`._
>
> Make sure that your pip install is up to date. Outdated pip versions have been known to cause such problems.

> _The dynamic library fails to load when importing the `harfang` module in Python._
>
> Make sure your system has the required runtime dependencies installed. It should have OpenAL and on Windows the Visual C++ 2017 redistributable installed.

> _`ImportError: DLL load failed: %1 is not a valid Win32 application.` error when importing the `harfang` module._
>
> This error usually happens when installing the incorrect version of Harfang for your Python version. For example when installing the 64 bit version of Harfang on a 32 bit install of the Python interpreter.

## First Program

Let's write a simple test program, create a new file named `test.py` and paste the following code into it.

```python
import harfang as hg

hg.InputInit()
hg.WindowSystemInit()

win = hg.RenderInit(400, 300, hg.RF_None)

while not hg.ReadKeyboard().Key(hg.K_Escape):
	hg.SetViewRect(0, 0, 0, 400, 300)
	hg.SetViewClear(0, hg.CF_Color, hg.RGBA32(96, 32, 255))

	hg.Touch(0)

	hg.Frame()
	hg.UpdateWindow(win)

hg.DestroyWindow(win)
```

When executed, this program demonstrates how to create an output window, initialize the input and render systems and paint the whole window in blue until the escape key is pressed.

In order to draw anything on screen we will need to create at least a shader and compile it before using it in our program, this process is documented in the [man.Assets] page.

[man.Quickstart] documents how to quickly get the more interesting tutorial programs running.