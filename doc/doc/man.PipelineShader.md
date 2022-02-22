.title Writing a Pipeline Shader

A pipeline shader solves the problem of having a single shader source code accepting optional parameters.

For example, a pipeline shader might accept a diffuse texture *or* a diffuse color. Having this level of flexibility in a single shader program is performance costly if done at runtime.

Instead, _a pipeline shader abstracts a set of variants_ each corresponding to a unique combination of its supported features.

[TOC]

## Features

A pipeline shader may declare one or more features it supports. Each feature corresponds to a set of exclusive values.

For example, the `OptionalDiffuseMap` feature has two possible values:

1. A texture is available and the diffuse attribute should be fetched from there.
2. No texture is available and the diffuse attribute should be fetched from the uDiffuseColor uniform.

To account for both scenarios in a static way, two variants of the pipeline shader will be compiled. The correct code path to execute in each variant is controlled by defines such as `USE_DIFFUSE_MAP=1`.

## Pipeline Stages

The rendering pipeline might also require variants of the shader to be generated.

Let's take a simple pipeline capable of rendering a model lit by a single light with optional shadow mapping. This pipeline must handle two possible scenarios:

1. If the light has shadow mapping enabled it should generate a shadow map and render it as it draws models.
2. Otherwise it may skip on the shadow map generation and draw models without accessing the shadow map.

## Feature & Stages Combinations

Continuing on the previous setup such a pipeline shader would produce *4 shader variants* when compiled:

- 2 for the `OptionalDiffuseMap` feature (ie. we either have a diffuse map or we don't),
- 2 for the pipeline stages (ie. one to render using the shadow map, the other without using it).

The following matrix illustrates this scenario.

Stage/Feature | USE_DIFFUSE_MAP=0 | USE_DIFFUSE_MAP=1
------------- | ----------------- | -----------------
Shadow pass   | 0                 | 2
Light pass    | 1                 | 3

## Writing a Pipeline Shader

A pipeline shader is a regular shader plus an extra definition file with the `.hps` extension.

This file is a JSON file listing the supported features and additional uniforms.

```json
{
	"features": [
		"OptionalDiffuseMap",
		"OptionalSpecularMap",
		"OptionalNormalMap",
		"OptionalLightMap"
	]
}
```

The following features are supported and have the following effect on the compilation process.

Feature | States | Preprocessor Directive | Uniform | Stage channel
------- | ----------- | ---------------------- | ------- |--------------
OptionalBaseColorOpacityMap | 2 | USE_BASE_COLOR_OPACITY_MAP=[1 or 0] | uBaseOpacityMap | 0
OptionalOcclusionRoughnessMetalnessMap  | 2 | USE_OCCLUSION_ROUGHNESS_METALNESS_MAP=[1 or 0] | uOcclusionRoughnessMetalnessMap| 1
OptionalDiffuseMap | 2 | USE_DIFFUSE_MAP=[1 or 0] | uDiffuseMap | 3
OptionalSpecularMap | 2 | USE_SPECULAR_MAP=[1 or 0] | uSpecularMap | 1
OptionalLightMap | 2 | USE_LIGHT_MAP=[1 or 0] | uLightMap | 3
OptionalSelfMap | 2 | USE_SELF_MAP=[1 or 0] | uSelfMap | 4
OptionalOpacityMap | 2 | USE_OPACITY_MAP=[1 or 0] | uOpacityMap | 5
OptionalAmbientMap | 2 | USE_AMBIENT_MAP=[1 or 0] | uAmbientMap | 6
OptionalReflectionMap | 2 | USE_REFLECTION_MAP=[1 or 0] | uReflectionMap | 7
OptionalNormalMap | 2 | USE_NORMAL_MAP=[1 or 0] | uNormalMap | 2
NormalMapInWorldSpace | 2 | NORMAL_MAP_IN_WORLD_SPACE=[1 or 0] | - | -
DiffuseUV1 | 2 | DIFFUSE_UV_CHANNEL=[1 or 0] | - | -
SpecularUV1 | 2 | SPECULAR_UV_CHANNEL=[1 or 0] | - | -
AmbientUV1 | 2 | AMBIENT_UV_CHANNEL=[1 or 0] | - | -
OptionalSkinning | 2 | ENABLE_SKINNING=[1 or 0] | - | -
OptionalAlphaCut | 2 | ENABLE_ALPHA_CUT=[1 or 0] | - | -

Obviously, a shader declaring all possible features will generate a considerable amount of variants.

*Note:* For backward compatibility reasons `AMBIENT_UV_CHANNEL=1` is always defined when compiling a pipeline shader without the AmbientUV1 feature.

### User Uniforms

Along features you can specify custom uniforms in the pipeline shader definition file. Three uniform types are supported: `Vec4`, `Color` and `Sampler`.

```json
{
	"uniforms": [
					{"name": "uCutoff", "type": "Vec4"}
				]
}
```

Defining a custom uniform will not make it available in the scope of the pipeline shader, you still have to declare it in the shader source. Custom uniforms do not increase the amount of variants generated.

When declaring a sampler uniform you need to specify the texture channel.

```json
{
	"uniforms": [
					{"name": "uOcclusionMap", "type": "Sampler", "channel": 10}
				]
}
```

You can also provide a default value for uniforms at this point.

```json
{
	"uniforms": [
					{"name": "uCutoff", "type": "Vec4", "value": [0.5, 0.5, 0.9, 1.0]},
					{"name": "uOcclusionMap", "type": "Sampler", "channel": 10, "value": "texture.jpg"}
				]
}
```

### Uniform Naming Convention

Uniforms are named differently whether they originate from the Bgfx backend or not.

- `uDiffuseMap/uLightMap/...` belong to the HARFANG pipeline,
- `u_view/u_invView/...` belong to the BGFX backend.

## Interaction with Materials

When updating material uniform values or textures you must call [UpdateMaterialPipelineProgramVariant] to update the pipeline shader variant used by the material.
