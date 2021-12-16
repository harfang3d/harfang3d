Get a node by its absolute path in the node hierarchy.

A node path is constructed as follow:

- Nodes are refered to by their name.
- To address the child of a node, use the `/` delimiter between its parent name and the child name.
- To address a node inside an instance component, use the `:` delimiter.
- There is no limit on the number of delimiters you can use.

Examples:

Get the node named `child` parented to the `root` node.

```python
child = scene.GetNodeEx('root/child')
```

Get the node named `dummy` instantiated by the `root` node.

```python
dummy = my_scene.GetNodeEx('root:dummy')
```
