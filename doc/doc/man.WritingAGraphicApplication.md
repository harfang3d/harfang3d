.title Writing a graphic application

There many ways to write a graphic application using the library. Depending the needs of your application one approach might be much better suited than the others. The following options are available:

Note: You can find many examples of graphic applications in the [man.Tutorials].

## Renderer-only application

This is the most low-level application where most manual work is required. But for very specific needs it might make much more sense to directly use the renderer instead of trying to bend the [RenderSystem] or even a [Scene] to achieve your goals.

This type of application is advised if you:

* Need to render a lot of custom primitives or very dynamic content,
* Do not require complex rendering functionalities such as shadows,
* Do not need scene functionalities such as animation or scripts,
* Have sufficient knowledge to work at such a low-level.

## RenderSystem application

The [RenderSystem] without a [Scene] is mostly useful for the high-level drawing code it provides over the [Renderer] API. It comes at the cost of having to bundle the core resources package and, as with most high-level APIs, somewhat reduced performances and higher memory consumption.

This type of application is advised if you:

* Need to render a moderate amount of standard primitives with common attributes,
* Do not require complex rendering functionalities such as shadows,
* Do not need scene functionalities such as animation or scripts.

## Scene application

Applications making use of a [Scene] object can leverage the full set of functionalities the engine has to offer. Complete scene management extensible with Lua scripts ([man.ScriptComponent]), support for advanced rendering features such as hardware skinning, shadow-mapping or post-processing.

Using a scene also automatically provides multi-threading benefits as the engine has knowledge of most your application graphical objects and can manage them more efficiently.

This type of application is advised if you:

* Need to display complex world efficiently with the typical feature set of real time modern 3d applications,

## Plus application

The [Plus] API is a high-level wrapper over the Harfang API. It simplifies the use of all previously mentionned APIs and is usually a good starting point for any project.

It is however very limited in scope and complex requirements will usually require dealing directly with the low-level APIs.

## Mixing application types

Since all of the previously described approaches are build on the top of each other ([Scene] is renderer by the [RenderSystem] through [Renderer] with [Plus] wrapping all of those APIs) it is safe to mix different approaches to build an application.