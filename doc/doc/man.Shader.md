.title Writing a Shader

[TOC]

## Shader Language Overview

Harfang uses [bgfx](https://bkaradzic.github.io/bgfx/index.html) as its rendering system, the cross-platform shader language is based on [GLSL](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.40.pdf) with a few key differences:

* No `bool/int` uniforms, all uniforms must be `float`.
* Attributes and varyings can be accessed only from `main()` function.
* Must use `SAMPLER2D/3D/CUBE/etc.` macros instead of `sampler2D/3D/Cube/etc.` tokens.
* Must use `vec2/3/4_splat(<value>)` instead of `vec2/3/4(<value>)`.
* Must use `mtxFromCols/mtxFromRows` when constructing matrices in shaders.
* Must use `mul(x, y)` when multiplying vectors and matrices.
* Must use `varying.def.sc` to define input/output semantic and precision instead of using `attribute/in` and `varying/in/out`.
* `$input/$output` tokens must appear at the begining of shader.

## Data Flow

In a shader, vertex attributes such as position or normal are send to the vertex shader as a stream of **attributes**. The vertex shader outputs are then interpolated across the rendered primitive and passed to the fragment shader as **varyings** to compute the final pixel color.

## Writing a Shader

A shader is composed of 3 files: a definition file, the vertex and fragment source files:

- `example_vs.sc`: Source for the vertex program.
- `example_fs.sc`: Source for the fragment program.
- `example_varying.def`: Shader definition file.

Both the vertex and fragment program must declare a main function.

```glsl
void main() { ... }
```

The shader definition file must list all inputs and outputs of the shader programs and associate them with standard semantics like `POSITION` or `NORMAL` (see [HLSL Semantics](https://docs.microsoft.com/fr-fr/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics?redirectedfrom=MSDN) for a complete list).

```glsl
vec3 vNormal : NORMAL;

vec3 a_position  : POSITION;
vec3 a_normal : NORMAL;
```

In the vertex program use `$input` to declare an attribute and `$output` to declare a varying. `$input/$output` tokens must appear at the begining of the program.

By convention attributes must be named one of the following: `a_position`, `a_normal`, `a_tangent`, `a_bitangent`, `a_color0`, `a_color1`, `a_color2`, `a_color3`, `a_indices`, `a_weight`, `a_texcoord0`, `a_texcoord1`, `a_texcoord2`, `a_texcoord3`, `a_texcoord4`, `a_texcoord5`, `a_texcoord6`, `a_texcoord7`, `i_data0`, `i_data1`, `i_data2`, `i_data3` or `i_data4`.

The following vertex program declares that it takes two attributes as input and outputs to a single varying.

```glsl
$input a_position, a_normal
$output vNormal

#include <bgfx_shader.sh>

void main() {
	vNormal = mul(u_model[0], vec4(a_normal * 2.0 - 1.0, 0.0)).xyz;
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
```

**Note:** Attributes and varyings can only be accessed from the shader main function.

Outputs from the vertex program then become inputs to the fragment program. The fragment program outputs its result to standard GLSL variables such as `gl_FragColor` (see [GLSL Language Specifications](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.40.pdf)).

```glsl
$input vNormal

#include <bgfx_shader.sh>

void main() {
	vec3 normal = normalize(vNormal);
	gl_FragColor = vec4(normal.x, normal.y, normal.z, 1.0);
}
```

## Passing Constants to a Shader

Constant values can be passed to a shader programs by using **uniforms**. This is done using [UniformSetValue] and [UniformSetTexture].

### Predefined Uniforms

The `bgfx_shader.sh` include file defines the following predefined uniforms.

Type | Symbol | Description
---- | ------ | -----------
vec4 | u_viewRect | View rectangle for current view, in pixels. (x, y, width, height)
vec4 | u_viewTexel | Inverse width and height. (1.0 / width, 1.0 / height, undef, undef)
mat4 | u_view | View matrix.
mat4 | u_invView | Inverse view matrix.
mat4 | u_proj | Projection matrix.
mat4 | u_invProj | Inverse projection matrix.
mat4 | u_viewProj | Concaneted view projection matrix.
mat4 | u_invViewProj | Concatenated inverted view projection matrix.
mat4 | u_model[BGFX_CONFIG_MAX_BONES] | Array of model matrices.
mat4 | u_modelView | Concatenated model view matrix, only the first model matrix from array is used.
mat4 | u_modelViewProj | Concatenated model view projection matrix.
vec4 | u_alphaRef | Alpha reference value for alpha test.
