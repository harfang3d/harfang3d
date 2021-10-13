.title Importing from FBX

The FBX converter is a command-line tool to convert an FBX file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations.

[TOC]

## Command-Line

```text
fbx-convert <input> [-out PATH] [-test-import] [-base-resource-path PATH]
            [-prefix PATH] [-profile PROFILE] [-shader PATH]
            [-material-policy POLICY] [-geometry-policy POLICY] [-texture-policy POLICY] [-scene-policy POLICY]
            [-scale FLOAT] [-geometry-scale FLOAT] [-fix-geometry-orientation] [-max_smoothing_angle FLOAT]
			[-calculate-normal-if-missing] [-calculate-tangent-if-missing]
            [-recalculate-normal] [-recalculate-tangent]
            [-detect-geometry-instances]
            [-quiet] [-help]
```

Option        | Shortcut | Description
--------------|----------|------------
`-out`        | `-o`     | Output directory for the converted resource files.
`-test-import`|          | Perform conversion but do not output any file.
`-base-resource-path` |  | Transform references to assets in this directory to be relative.
`-prefix`     |          | Specify the file system prefix from which relative assets are to be loaded from.
`-profile`    | `-p`     | Material profile (`default` or `pbr_default`).
`-shader`     | `-s`     | Override the shader used by exported materials.
`-all-policy` |          | All file type import policy.
`-material-policy` |     | Material file import policy.
`-geometry-policy` |     | Geometry file import policy.
`-texture-policy` |      | Texture file import policy.
`-scene-policy` |        | Scene file import policy.
`-scale`      |          | Factor used to scale the scene nodes.
`-geometry-scale` |      | Factor used to scale exported geometries.
`-fix-geometry-orientation` | | Bake a 90° rotation on the X axis of exported geometries.
`-max_smoothing_angle` | | Maximum smoothing angle between two faces when computing vertex normals.
`-calculate-normal-if-missing` |  | Compute missing vertex normals.
`-calculate-tangent-if-missing` | | Compute missing vertex tangents.
`-recalculate-normal` |  | Recreate the vertex normals of exported geometries.
`-recalculate-tangent` | | Recreate the vertex tangent frames of exported geometries.
`-detect-geometry-instances` |    | Detect and optimize geometry instances.
`-quiet`      | `-q`     | Quiet output.
`-help`       | `-h`     | Display help message.

The following policies are available: `skip`, `overwrite`, `rename` and `skip_always`. The default policy is `skip`.

## Return code

The executable returns `1` if no error occurred, `0` otherwise.

To notify the caller about the conversion result the converter sends special markers to the standard output:

* `[TestImport: OK]` if the import test went well.
* `[TestImport: KO]` if the import test failed.
* `[ImportScene: OK]` if the import succeeded.
* `[ImportScene: KO]` if the import failed.

## Examples

Import `scene.fbx` to `./export`, overwrite existing geometry, skip everything else if it exists:

```shell
fbx-convert scene.fbx -o export -texture-policy overwrite
```

Import `scene.fbx` to `./export`, only import scene, skip everything else:

```shell
fbx-convert scene.fbx -o export -material-policy skip_always -geometry-policy skip_always -texture-policy skip_always
```

## A word on scale

When working with assets from different sources it is not uncommon to encounter considerable scale differences for similar objects. To compensate for this the importer can adjust both the scene scale and the geometry scale. As a rule of thumb, even if the imported scene looks on scale, you should make sure that the geometry itself uses a correct scale.

Failing to do so will pose many problems for example when sharing animations between models.

## Autodesk FBX SDK Notice

This software contains Autodesk® FBX® code developed by Autodesk, Inc. Copyright 2014 Autodesk, Inc. All rights, reserved.

Such code is provided "as is" and Autodesk, Inc. disclaims any and all warranties, whether express or implied, including without limitation the implied warranties of merchantability, fitness for a particular purpose or non-infringement of third party rights. In no event shall Autodesk, Inc. be liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of such code.
