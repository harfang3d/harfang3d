.title Using the Forward Pipeline

The forward pipeline implements multi-pass drawing of model batches.

[TOC]

## Features

- Separate opaque/transparent passes.
- 8 light slots: 1 linear, 3 spot/point and 4 point lights.
- PBR support.

## Pipeline Shaders

This pipeline comes with two compatible pipeline shaders available in the core resources package.

### default.hps (core/shader/default)

This shader supports the following features:

Feature | Description | Fallback
----|----|----
OptionalDiffuseMap | Diffuse texture on UV0 | uDiffuseColor uniform value
OptionalSpecularMap | Specular texture on UV0 | uSpecularColor uniform value
OptionalReflectionMap | Reflection texture on normal reflection | -
OptionalNormalMap | Normal texture on UV0 | Geometric normal
OptionalSelfMap | Self-emissive texture on UV0 | uSelfColor uniform value
OptionalLightMap | Light modulation texture on UV1 | vec3(0, 0, 0)
OptionalAmbientMap | Ambient occlusion texture on UV1 | vec3(1, 1, 1)
OptionalOpacityMap | Opacity texture on UV0 | 1

This shader implements traditional Phong rendering.

### pbr.hps (core/shader/pbr)

This shader supports the following features:

Feature | Description | Fallback
----|----|----
OptionalBaseColorOpacityMap | Texture on UV0 with color on `xyz` and opacity on `w` | uBaseOpacityColor uniform value
OptionalOcclusionRoughnessMetalnessMap | ORM texture on UV0 | uOcclusionRoughnessMetalnessColor uniform value
OptionalSelfMap | Self-emissive texture on UV0 | uSelfColor uniform value
OptionalNormalMap | Normal texture on UV0 | Geometric normal

This shader implements image-based lighting.

## Drawing a Scene

See [man.DrawingScene].
