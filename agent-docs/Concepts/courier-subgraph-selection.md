# Creating a SubGraph from a Selection with Flow Courier

$KB:flow:concept:courier-subgraph-selection
keywords: subgraph selection collapse courier plan create boundary

## What this is

The FlowGraphCourier toolset exposes the editor's collapse operation as two agent operations:
`PlanFlowSubgraphFromSelection` and `CreateFlowSubgraphFromSelection`. Supply the exact top-level node
GUIDs and a child name beginning with `Subgraph_`. The plan is read-only and reports boundary counts,
warnings, errors, and the names that will become the child interface. The create operation reruns that
plan before applying, replaces the source selection with one `SubGraph` node, and returns the child
asset path and replacement node GUID.

## Boundary and apply behavior

The collapse interface is exec-only. It needs exactly one incoming execution link, rejects data links
crossing the selection boundary, and keeps the source asset's `CustomInput`, `CustomOutput`, and
`Finish` nodes outside the selection. The first unnamed exit maps to the child's built-in `Finish`;
named or additional exits become deterministic `CustomOutput` nodes.

Use the mutation dry run before applying. It rolls back the source graph and removes the temporary
child package. After applying, export or diff both the source and child assets because the operation
changes two packages.

The generated child is a distinct asset package. Check its asset class and domain configuration
explicitly; do not assume those settings carry over from the parent.

## See also

- $KB:flow:concept:subgraphs
- $KB:flow:guide:courier-text-format
