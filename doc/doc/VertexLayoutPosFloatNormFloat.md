Simple vertex layout with float position and normal.

```python
vtx_layout = VertexLayout()
vtx_layout.Begin()
vtx_layout.Add(hg.A_Position, 3, hg.AT_Float)
vtx_layout.Add(hg.A_Normal, 3, hg.AT_Float)
vtx_layout.End()
```
