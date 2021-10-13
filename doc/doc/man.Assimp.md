.title Importing from Assimp

The Assimp converter is a command-line tool to convert an Assimp file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations.

[TOC]

## Command-Line

```text
assimp_convert <input> [-out PATH] [-test-import] [-base-resource-path PATH]
            [-prefix PATH] [-profile PROFILE] [-shader PATH]
            [-material-policy POLICY] [-geometry-policy POLICY] [-texture-policy POLICY] [-scene-policy POLICY]
            [-scene-scale FLOAT] [-max_smoothing_angle FLOAT]
			[-calculate-normal-if-missing] [-calculate-tangent-if-missing]
            [-recalculate-normal] [-recalculate-tangent]
            [-quiet] [-help]
```

Option        | Shortcut | Description
--------------|----------|------------
`-out`        | `-o`     | Output directory for the converted resource files.
`-base-resource-path` |  | Transform references to assets in this directory to be relative.
`-prefix`     |          | Specify the file system prefix from which relative assets are to be loaded from.
`-profile`    | `-p`     | Material profile (`default` or `pbr_default`).
`-shader`     | `-s`     | Override the shader used by exported materials.
`-all-policy` |          | All file type import policy.
`-material-policy` |     | Material file import policy.
`-geometry-policy` |     | Geometry file import policy.
`-texture-policy` |      | Texture file import policy.
`-scene-policy` |        | Scene file import policy.
`-scene-scale` |         | Factor used to scale the scene.
`-max_smoothing_angle` | | Maximum smoothing angle between two faces when computing vertex normals.
`-calculate-normal-if-missing` |  | Compute missing vertex normals.
`-calculate-tangent-if-missing` | | Compute missing vertex tangents.
`-recalculate-normal` |  | Recreate the vertex normals of exported geometries.
`-recalculate-tangent` | | Recreate the vertex tangent frames of exported geometries.
`-anim-simplify-translation-tolerance` | | Tolerance on translation animations.
`-anim-simplify-rotation-tolerance` | | Tolerance on rotation animations.
`-anim-simplify-scale-tolerance` | | Tolerance on scale animations.

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

Import `scene.obj` to `./export`, overwrite existing geometry, skip everything else if it exists:

```shell
assimp_convert scene.obj -o export -texture-policy overwrite
```

Import `scene.obj` to `./export`, only import scene, skip everything else:

```shell
assimp_convert scene.obj -o export -material-policy skip_always -geometry-policy skip_always -texture-policy skip_always
```

## A word on scale

When working with assets from different sources it is not uncommon to encounter considerable scale differences for similar objects. To compensate for this the importer can adjust both the scene scale and the geometry scale. As a rule of thumb, even if the imported scene looks on scale, you should make sure that the geometry itself uses a correct scale.

Failing to do so will pose many problems for example when sharing animations between models.

