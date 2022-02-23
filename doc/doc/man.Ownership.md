.title Ownership & References

Many resource types are returned by value or as generational reference to your program. A generational reference is a weak pointer to a resource. It has no control over the resource lifetime but can be queried for its availability.

Using a reference to a resource after it has been disposed of will result in no operation being carried out.

## Generational Reference

A generational reference is a 64 bit integer comprised of two 32 bit integers: the *generation* of the reference and its *index* inside the container.

A generational container will ensure that the reference's and container's generation both match for an index to be considered valid.

It is possible for a long standing reference to be incorrectly considered as valid although very unlikely. This would require holding a reference for 4 billion generations of its index.

## Managing Rendering Resources

When returned by value, rendering resources are owned by your program. The underlying resource will usually be destroyed when the last reference to it in your program is disposed of.

Functions returning rendering resource by value include [LoadTextureFromAssets], [LoadModelFromAssets] or [LoadPipelineProgramFromAssets].

To prevent ownership issues however, Harfang mostly tracks rendering resources using a [PipelineResources] object which has strong ownership of the resources it manages.

Functions returning rendering resource by reference include [LoadTextureFromAssets] and [LoadPipelineProgramRefFromAssets].

Use the methods of [PipelineResources] to destroy the resources it manages.
