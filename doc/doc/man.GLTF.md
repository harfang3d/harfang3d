.title Importing from GLTF

The GLTF importer is a command-line tool to convert a GLTF file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations.

[TOC]

## Command-Line

```text
gltf-import <input> [-out PATH] [-base-resource-path PATH] [-name SCENE_NAME] [-prefix PATH] [-shader PATH]
            [-material-policy POLICY] [-geometry-policy POLICY] [-texture-policy POLICY] [-scene-policy POLICY]
            [-geometry-scale FLOAT] [-recalculate-normal] [-recalculate-tangent] [-quiet] [-help]
```

Option        | Shortcut | Description
--------------|----------|------------
`-out`        | `-o`     | Output directory for the converted resource files.
`-base-resource-path` |  | Transform references to assets in this directory to be relative.
`-name`       |          | Scene name if not specified in the GLTF container.
`-prefix`     |          | Specify the file system prefix from which relative assets are to be loaded from.
`-shader`     | `-s`     | Override the shader used by exported materials.
`-all-policy` |          | All file type import policy.
`-material-policy` |     | Material file import policy.
`-geometry-policy` |     | Geometry file import policy.
`-texture-policy` |      | Texture file import policy.
`-scene-policy` |        | Scene file import policy.
`-geometry-scale` |      | Factor used to scale exported geometries.
`-recalculate-normal` |  | Recreate the vertex normals of exported geometries.
`-recalculate-tangent` | | Recreate the vertex tangent frames of exported geometries.
`-quiet`      | `-q`     | Quiet output.
`-help`       | `-h`     | Display help message.

The following policies are available: `skip`, `overwrite`, `rename` and `skip_always`. The default policy is `skip`.

## Return code

The executable returns `1` if no error occurred, `0` otherwise.

To notify the caller about the conversion result the converter sends special markers to the standard output:

* `[ImportScene: OK]` if the import succeeded.
* `[ImportScene: KO]` if the import failed.

## Examples

Import `scene.gltf` to `./export`, overwrite existing geometry, skip everything else if it exists:

```shell
gltf-import scene.gltf -o export -texture-policy overwrite
```

Import `scene.gltf` to `./export`, only import scene, skip everything else:

```shell
gltf-import scene.fbx -o export -material-policy skip_always -geometry-policy skip_always -texture-policy skip_always
```
