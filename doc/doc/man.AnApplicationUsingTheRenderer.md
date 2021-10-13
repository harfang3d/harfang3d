.title An application using the renderer

This page describes a complete Harfang application in Python displaying a triangle using the [Renderer].

The complete source for this application can be found in the [man.Tutorials].

## Program overview

To display a triangle using the renderer we will need to:

1. Create the renderer,
* Create a window and ,
* Describe the geometry we intend to draw,
* Provide the geometry to the renderer along with a shader to draw it.
* Display the triangle in a loop until the end of execution condition is met.

## Creating the renderer

We first need to create an object of [Renderer] type. By default this application uses the OpenGL implementation of the renderer interface, but any other available implementation can be used in its place.

```python
renderer = hg.CreateRenderer()
renderer.Open()
```

The [Renderer] object is first created then its [Renderer_Open] member function is called. At this point no window is created.

## Creating the window

We create a new [Window] using the [NewWindow] function.

```python
win = hg.NewWindow(640, 480)
```

We create a [Surface] for the newly created window and set it as the [Renderer] new output surface.

```python
surface = renderer.NewOutputSurface(win)
renderer.SetOutputSurface(surface)
```

## Describing the geometry to draw

The [Renderer] API works at the lowest level of abstraction and uses [GpuBuffer] and [VertexLayout] together with a [Shader] to build and draw primitives.

### Index buffer

A triangle is build from 3 vertices which are connected in sequential order (vertex 0 to vertex 1 to vertex 2). So we need a vertex buffer of 3 vertices and an index buffer of 3 indexes containing the following values: 0, 1 and 2.

The index values need to be packed into a memory buffer before they can be send to the gpu. The [BinaryBlob] class can be used to this effect.

```python
data = gs.BinaryData()
data.WriteUInt16s([0, 1, 2])  # we use 16 bit packing of the index values by writing shorts
```

The index buffer is then very easily constructed from the binary blob.

```python
idx = renderer.NewBuffer()
renderer.CreateBuffer(idx, data, hg.GpuBufferIndex)
```

### Vertex buffer

A vertex can be made of any number of attributes (position, color, UV, etc...) which are then fed into the shader used to draw the primitives. To specify the layout of a vertex buffer we use the [VertexLayout] class.

The triangle we will display is the simplest one possible, displaying a single color provided as a shader parameter, so its vertices only need a position attribute. The position of each vertex will be stored using 3 float values.

```python
vtx_layout = hg.VertexLayout()
vtx_layout.AddAttribute(hg.VertexPosition, 3, hg.VertexFloat)
```

We then prepare the vertex buffer content using another binary blob.

```python
data = hg.BinaryData()
data.WriteFloats([-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5])
```

We can now create the vertex buffer.

```python
vtx = renderer.NewBuffer()
renderer.CreateBuffer(vtx, data, hg.GpuBufferVertex)
```

The buffers are complete and ready to render.

## Create a shader to render the geometry

The shader we use to display the triangle takes the vertex position as it is and output the same color for each pixel. The output color is an input parameter named `u_color` which we will programmatically change.

```glsl
in { vec4 u_color; }

variant {
	vertex {
		out { vec4 v_color; }

		source %{
			v_color = u_color;
			%out.position% = vec4(vPosition, 1.0);
		%}
	}

	pixel {
		in { vec4 v_color; }

		source %{
			%out.color% = v_color;
		%}
	}
}
```

For more information on the shader structure, refer to the [man.Shader] page.

To load the shader from a file we first mount a file driver (cf. [man.Assets]).

```python
hg.MountFileDriver(hg.StdFileDriver())

shader_path = os.path.join(os.getcwd(), "../_data/shader_2d_color.isl")
shader = renderer.LoadShader(shader_path)
```

**Note:** A default value for `u_color` can be specified in the shader declaration so that we do not have to set it programmatically.

## The application render loop

Everything is now ready so we enter the main application loop in which we render the triangle. The application loops until the default renderer window is closed and starts by clearing the render target to a solid red color.

```python
while hg.IsWindowOpen(win):
	renderer.Clear(hg.Color.Red)
```

Next, the shader and its `u_color` input value are set and the index/vertex buffers are drawn to the render target.

```python
	renderer.SetShader(shader)
	renderer.SetShaderFloat4("u_color", 0, 1, 0, 1)
	hg.DrawBuffers(renderer, 3, idx, vtx, vtx_layout)
```

**Note:** The [DrawBuffers] call specifies the drawing of 3 indexes and uses the default value for the index type and primitive type. Those are 16 bit indexes and triangle primitives.

Finally, the loop ends by commiting the draw call, showing the draw result and updating the renderer output window.

```python
	renderer.DrawFrame()
	renderer.ShowFrame()

	hg.UpdateWindow(win)
	hg.EndFrame()
```