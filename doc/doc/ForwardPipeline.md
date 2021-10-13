Rendering pipeline implementing a forward rendering strategy.

The main characteristics of this pipeline are:

- Render in two passes: opaque display lists then transparent ones.
- Fixed 8 light slots supporting 1 linear light with PSSM shadow mapping, 1 spot with shadow mapping and up to 6 point lights with no shadow mapping.