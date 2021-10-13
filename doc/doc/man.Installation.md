.title Installation

## Quick Install

* **Using PIP in a command line:** `pip install harfang`
* **Or download the wheel from :** [Downloads](https://www.harfang3d.com/downloads)

If anything goes wrong, please look at the [TroubleShooting](#TroubleShooting) section.<br/>
Further details on the installation are available below.

## Prerequisites

The following dependencies must be installed on your system for any Harfang project to work properly.

* Functional OpenGL 3.3 hardware and drivers

### Windows

* OpenAL redistributable (`oalinst.exe`)
* Visual C++ 2017 redistributable (`vcredist.exe`)

### Linux

* OpenAL (`sudo apt-get install libopenal1`)

## Installation

Harfang is available for several programming languages as an extension or as a standalone executable. The following sections describe the installation procedure for each variant.

### Python

* Download the `.whl` package for your OS and Python version. ([Downloads](https://www.harfang3d.com/downloads))

#### Windows

1. Open a command prompt as Administrator (`Win+X` then `Command Prompt (Admin)`).
1. Switch to the download directory and execute `pip install <your_harfang_version>.whl --user`.

#### OSX

1. Open a terminal window (in `Applications/Utilities/Terminal.app`).
1. Switch to the download directory and execute `pip install <your_harfang_version>.whl --user`.

#### Linux

1. Open a terminal window.
1. Switch to the download directory and execute `pip install <your_harfang_version>.whl --user`.

**Note:** You might need to explicitly use `pip3` to install the module if your system has both Python 2 and 3 installed.

#### Confirm your installation ####

Confirm your installation by starting your Python 3 interpreter and execute the following statement `import harfang as hg`. If you receive no error message, the installation was successful.

### Lua

Deploy the binary extension to your Lua interpreter or use the provided interpreter.

## <a name="Troubleshooting"></a>Troubleshooting

### Python

#### 1. `pip install` fails with a message saying `harfang is not a supported wheel on this platform`.

Make sure that your pip install is up to date. Outdated pip versions have been known to cause such problems.

#### 2. The dynamic library fails to load when importing the `harfang` module in Python.

Make sure your system has the required runtime dependencies installed. It should have OpenAL and on Windows the Visual C++ 2017 redistributable installed.

#### 3. `ImportError: DLL load failed: %1 is not a valid Win32 application.` error when importing the `harfang` module.

This error usually happens when installing the incorrect version of Harfang for your Python version. For example when installing the 64 bit version of Harfang on a 32 bit install of the Python interpreter.
