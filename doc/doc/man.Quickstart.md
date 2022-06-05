.title Quickstart

Once you have a functioning installation of Harfang for your language of choice:

* [man.CPython]
* [man.Lua]

Follow the following steps:

1. Download the tutorials from Github [here](https://github.com/harfang3d/tutorials-hg2.git) and unzip them to your computer _(eg. in `d:/tutorials-hg2`)_.
1. Download _assetc_ for your platform from [here](https://www.harfang3d.com/releases) to compile the tutorial resources.
1. Drag and drop the tutorial resources folder on the assetc executable **-OR-** execute assetc passing it the path to the tutorial resources folder _(eg. `assetc d:/tutorials-hg2/resources`)_.

![assetc drag & drop](/images/docs/${HG_VERSION}/assetc.gif)

After the compilation process finishes, you will see a `resources_compiled` folder next to the tutorials resources folder.

You can now execute the tutorials from the folder you unzipped them to.

```bash
D:\tutorials-hg2>python draw_lines.py
```

Alternatively, you can open the tutorial folder using [Visual Studio Code](https://code.visualstudio.com/) and use the provided debug targets to run the tutorials.

It is strongly advised that you read the next chapters of this documentation starting with [man.Assets].
