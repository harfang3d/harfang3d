Provides a way to capture the different rendering stages of the forward pipeline when rendering a scene.

The output of the opaque, transparent and depth passes can be captured to separate textures. You must explicitly flag as readback the stage textures you intend to convert to [Picture].