.title Drawing to Views

Views are an essential mechanism of the 3d rendering backend and must be thoroughly understood to perform any form of complex rendering.

## Overview

Views are refered to by index as an integer value between 0 and 255. There can be no more than 256 views defined.

Each call to a drawing function such as [DrawLines] or [DrawModel] is *queued* as multiple draw commands on the specified view. When [Frame] is called all views are processed in order, from the lowest to the highest id.

Keep in mind that rendering is deferred and no actual drawing occurs immediately after calling a draw function.

Drawing happens when [Frame] is called and all views are reset after that.

## View Properties

The following properties are specified *per view*:

- View and projection matrices set by [SetViewTransform].
- Clipping rectangle set by [SetViewRect].
- Color/depth/stencil clear set by [SetViewClear].
- Draw command ordering set by [SetViewMode].
- Output buffer set by [SetViewFrameBuffer].

A view accepts a single value for each of these properties. *Multiple calls to change the same property of a view will override its current value.*

If you need to use different values for a view property you must use additional views even if all other properties remain unchanged.

Views with no queued draw commands are skipped. If you need to force the processing of a view, use [Touch].

## Draw Command Queue

Draw commands queued on a view are processed according to the view mode, they are executed:

- In submission order if the view mode is [VM_Sequential].
- In ascending depth order if the view mode is [VM_Ascending].
- In descending depth order if the view mode is [VM_Descending].

When the view mode is [VM_Ascending] or [VM_Descending], the depth value used to sort draw commands is derived from the draw call *depth* parameter.

When no depth is specified, 0 is implied.
