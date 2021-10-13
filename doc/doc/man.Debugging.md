.title Debugging

## Debugging general issues

The first thing to check when your program fails is the engine log output. The log output sends all engine debugging messages to the console.

The log system defaults to only displaying warning and error level messages. Enabling debug and standard level messages by calling `hg.SetLogLevel(hg.LogAll)` (see [SetLogLevel]) will simplify identifying why something is going wrong.

Complex error messages might include detailed information on the error. By default, details are filtered out by the system and must be enabled using `hg.SetLogIsDetailed(True)` (see [SetLogIsDetailed]).

## Debugging scene issues

A scene is to complex an object to debug through the log system. The engine debugger includes a scene debugger which is the perfect tool for dwelving into the data structures of a scene. Refer to the [man.EngineDebugger] manual page for how to use it.