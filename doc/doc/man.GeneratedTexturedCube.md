.title Generated textured cube

The [Geometry] class contains the functions needed to generate meshes. 

## Generated textured cube

To generate a textured cube using [Geometry] API, you need the following functions:

### Vertices
* [Geometry_AllocateVertex] : Set the number of vertices.
* [Geometry_SetVertex] : Set vertex coordinates.

### Polygons
* [Geometry_AllocatePolygon] : Set number of polygons.
* [Geometry_SetPolygon] : Set the number of vertices and the polygon material.
* [Geometry_AllocatePolygonBinding] : Allocate memory to store the geometry polygon binding table.
* [Geometry_SetPolygonBinding] : Set the polygon binding table.

### Normals
* [Geometry_AllocateVertexNormal] : Set the number of normals.
* [Geometry_SetVertexNormal] : Set normal coordinates.

### UVs
* [Geometry_AllocateUVChannels] : Set number of UVs.
* [Geometry_SetUV] : Set UV coordinates.

### Materials
* [Geometry_AllocateMaterialTable] : Set number of materials.
* [Geometry_SetMaterial] : Set material definition file path.

## Code example

### Vertices and polygons

```python

	cube = hg.Geometry()
	
	# Create vertex
	
	s = hg.Vector3(1,1,1) # dimensions
	cube.AllocateVertex(8)
	ube.SetVertex(0, hg.Vector3(-s.x, -s.y, -s.z))
	cube.SetVertex(1, hg.Vector3(-s.x, -s.y, s.z))
	cube.SetVertex(2, hg.Vector3(-s.x, s.y, -s.z))
	cube.SetVertex(3, hg.Vector3(-s.x, s.y, s.z))
	cube.SetVertex(4, hg.Vector3(s.x, -s.y, -s.z))
	cube.SetVertex(5, hg.Vector3(s.x, -s.y, s.z))
	cube.SetVertex(6, hg.Vector3(s.x, s.y, -s.z))
	cube.SetVertex(7, hg.Vector3(s.x, s.y, s.z))
	
	# Create polygons
	
	cube.AllocatePolygon(6)
	cube.SetPolygon(0, 4, 0)
	cube.SetPolygon(1, 4, 1)
	cube.SetPolygon(2, 4, 2)
	cube.SetPolygon(3, 4, 3)
	cube.SetPolygon(4, 4, 4)
	cube.SetPolygon(5, 4, 5)
	
	# Polygons bindings
	
	cube.AllocatePolygonBinding()
	cube.SetPolygonBinding(0, hg.IntList([0, 2, 6, 4]))
	cube.SetPolygonBinding(1, hg.IntList([4, 6, 7, 5]))
	cube.SetPolygonBinding(2, hg.IntList([5, 7, 3, 1]))
	cube.SetPolygonBinding(3, hg.IntList([1, 3, 2, 0]))
	cube.SetPolygonBinding(4, hg.IntList([2, 3, 7, 6]))
	cube.SetPolygonBinding(5, hg.IntList([4, 5, 1, 0]))

```

### Normals. Each vertex for each polygon has a normal. So, 6 polygons X 4 vertices = 24 normals 

```python

	# Normals
	
	cube.AllocateVertexNormal(24)
	cube.SetVertexNormal(0, hg.Vector3(0, 0, -1))
	cube.SetVertexNormal(1, hg.Vector3(0, 0, -1))
	cube.SetVertexNormal(2, hg.Vector3(0, 0, -1))
	cube.SetVertexNormal(3, hg.Vector3(0, 0, -1))
	cube.SetVertexNormal(4, hg.Vector3(1, 0, 0))
	cube.SetVertexNormal(5, hg.Vector3(1, 0, 0))
	cube.SetVertexNormal(6, hg.Vector3(1, 0, 0))
	cube.SetVertexNormal(7, hg.Vector3(1, 0, 0))
	cube.SetVertexNormal(8, hg.Vector3(0, 0, 1))
	cube.SetVertexNormal(9, hg.Vector3(0, 0, 1))
	cube.SetVertexNormal(10, hg.Vector3(0, 0, 1))
	cube.SetVertexNormal(11, hg.Vector3(0, 0, 1))
	cube.SetVertexNormal(12, hg.Vector3(-1, 0, 0))
	cube.SetVertexNormal(13, hg.Vector3(-1, 0, 0))
	cube.SetVertexNormal(14, hg.Vector3(-1, 0, 0))
	cube.SetVertexNormal(15, hg.Vector3(-1, 0, 0))
	cube.SetVertexNormal(16, hg.Vector3(0, 1, 0))
	cube.SetVertexNormal(17, hg.Vector3(0, 1, 0))
	cube.SetVertexNormal(18, hg.Vector3(0, 1, 0))
	cube.SetVertexNormal(19, hg.Vector3(0, 1, 0))
	cube.SetVertexNormal(20, hg.Vector3(0, -1, 0))
	cube.SetVertexNormal(21, hg.Vector3(0, -1, 0))
	cube.SetVertexNormal(22, hg.Vector3(0, -1, 0))
	cube.SetVertexNormal(23, hg.Vector3(0, -1, 0))
	
```

### UVs and materials

```python
	# Create UVs
	
	cube.AllocateUVChannels(1, 24)
	cube.SetUV(0, 0, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	cube.SetUV(0, 1, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	cube.SetUV(0, 2, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	cube.SetUV(0, 3, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	cube.SetUV(0, 4, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	cube.SetUV(0, 5, hg.Vector2List([hg.Vector2(0, 0), hg.Vector2(0, 1), hg.Vector2(1, 1), hg.Vector2(1, 0)]))
	
	# Create materials
	
	cube.AllocateMaterialTable(6)
	cube.SetMaterial(0, "assets/materials/face1.mat")
	cube.SetMaterial(1, "assets/materials/face2.mat")
	cube.SetMaterial(2, "assets/materials/face3.mat")
	cube.SetMaterial(3, "assets/materials/face4.mat")
	cube.SetMaterial(4, "assets/materials/face5.mat")
	cube.SetMaterial(5, "assets/materials/face6.mat")

```

### Validate Geometry and create Node

After having determined the geometry, it's possible to validate your structure. If all is ok, you can create [Object] and [Node].

```python
	if cube.Validate():
		geo = plus.GetRenderSystem().CreateGeometry(cube,False)
		obj = hg.Object()
		obj.SetGeometry(geo)
		node = hg.Node()
		node.SetName("generated_cube")
		transform = hg.Transform(hg.Vector3(0,3,0))
		node.AddComponent(transform)
		node.AddComponent(obj)
		scene.AddNode(node)
		return node
```



