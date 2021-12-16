Create a projection frustum. This object can then be used to perform culling using [TestVisibility].

```python
# Compute a perspective matrix
proj = hg.ComputePerspectiveProjectionMatrix(0.1, 1000, hg.FovToZoomFactor(math.pi/4), 1280/720)
# Make a frustum from this projection matrix
frustum = hg.MakeFrustum(proj)
```