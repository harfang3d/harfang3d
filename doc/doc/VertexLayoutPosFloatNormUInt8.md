Simple vertex layout with float position and 8-bit unsigned integer normal.

```python
vtx_layout = VertexLayout()
vtx_layout.Begin()
vtx_layout.Add(hg.A_Position, 3, hg.AT_Float)
vtx_layout.Add(hg.A_Normal, 3, hg.AT_Uint8, True, True)
vtx_layout.End()
```
