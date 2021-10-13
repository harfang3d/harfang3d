Compute an orthographic projection matrix.

An orthographic projection has no perspective and all lines parrallel in 3d space will still appear parrallel on screen after projection using the returned matrix.

The `size` parameter controls the extends of the projected view. When projecting a 3d world this parameter is expressed in meters. Use the `aspect_ratio` parameter to prevent distortion from induced by non-square viewport.

See [ComputeAspectRatioX] or [ComputeAspectRatioY] to compute an aspect ratio factor in paysage or portrait mode.