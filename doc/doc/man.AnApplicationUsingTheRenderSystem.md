.title An application using the render system

This page describes a complete Harfang application in Python displaying a triangle using the [RenderSystem].

.img("man.AnApplicationUsingTheRenderSystem.png")

The complete source for this application can be found in the [man.Tutorials].

## Program overview

To display a triangle using the render system we will need to:

1. Create the renderer and a render system wrapping it,
* Display the triangle in a loop until the end of execution condition is met.

Most steps of this program are explained in details in the [man.AnApplicationUsingTheRenderer] page.

## Creating the render system

This application uses [Renderer] and wraps it with [RenderSystem].

```python
# create the renderer
renderer = hg.CreateRenderer()
renderer.Open()

# open a new window
win = hg.NewWindow(480, 240)

# create a new output surface for the newly opened window
surface = renderer.NewOutputSurface(win)
renderer.SetOutputSurface(surface)

# initialize the render system, which is used to draw through the renderer
render_system = hg.RenderSystem()
render_system.Initialize(renderer)
```

The render system is ready to work.

## The application render loop

### A word on vertex transformation

Since we will be displaying the triangle using the render system we have less control over the shader that is going to be used. The render system core resources include shader to render all the common combination of vertex attributes.

However, unlike the the shader we used in the equivalent renderer program, _all render system shaders make use of the renderer ModelViewProjection matrix_. So we first need to initialize it.

For the purpose of this program a simple 2D projection system will do. The following call will set a projection matrix that maps vertex coordinate to pixels with (0;0) in the lower-left corner of the viewport with +X going right and +Y going up.

```python
renderer.Set2DMatrices()
```

### Drawing the triangle

The application loops until the default renderer window is closed and starts by clearing the render target to a solid green color.

```python
while hg.IsWindowOpen(win):
	renderer.Clear(hg.Color.Green)
```

Next, we tell the render system to draw the triangle using the helper function it provides for this task.

```python
	vertices = [hg.Vector3(0, 0, 0), hg.Vector3(0, 240, 0), hg.Vector3(480, 240, 0)]
	render_system.DrawTriangleAuto(1, vertices, color)
```

Finally, the loop ends by showing the draw result and updating the renderer output window.

```python
	hg.Frame()
	hg.UpdateWindow(win)
```
