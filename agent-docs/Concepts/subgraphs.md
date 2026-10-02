# Subgraphs (Concept)

$KB:flow:concept:subgraphs
keywords: subgraph sub-graph FlowNode_SubGraph CustomInput CustomOutput compose decompose nested asset interface context pin

## What this is

`FlowNode_SubGraph` embeds another `FlowAsset` as a single node in the parent graph - the parent
graph runs the sub-asset as a nested Flow instance and continues once it finishes (or via whatever
named exit points the sub-asset declares). This is the generic Flow mechanism for **graph
decomposition**: splitting a large or reusable piece of behavior into its own asset instead of
inlining it everywhere it's needed.

## The pieces

- **`UFlowNode_SubGraph`** (`Flow` module, `Nodes/Graph/FlowNode_SubGraph.h`) - the parent-side
  node. Key properties: `Asset` (`TSoftObjectPtr<UFlowAsset>`, the sub-asset to instantiate and
  run), `AssetParams` (`TSoftObjectPtr<UFlowAssetParams>`, optional data-pin value supplier for the
  sub-asset), `bCanInstanceIdenticalAsset` (guards against a sub-asset instancing itself -
  **enabling this can cause infinite recursion if the graph spawns itself unconditionally**,
  leave it off unless you specifically need self-referential recursion). Editor-only
  `AllowedAssignedAssetClasses`/`DeniedAssignedAssetClasses` restrict which `FlowAsset` subclasses
  are legal to assign, per context.
- **`FlowNode_CustomInput`** / **`FlowNode_CustomOutput`** (sub-asset side) - place one of these
  inside the sub-asset, with its `EventName` property set to the desired pin name. This is what
  creates a **named entry or exit point** beyond the sub-asset's built-in `Start`/`Finish` exec
  pins.

## How the interface is exposed

Every `FlowNode_CustomInput`/`FlowNode_CustomOutput` in the sub-asset automatically becomes a
matching **context pin** on the parent's `FlowNode_SubGraph` node - a `CustomInput` named `Cancel`
becomes an input pin named `Cancel` on the `SubGraph` node; a `CustomOutput` named `Done` becomes
an output pin named `Done`. Wire them like any other exec pin:
```
ParentNode.Cancel -> SubGraphNode.Cancel
SubGraphNode.Done -> NextNode.In
```
This list of context pins is rebuilt by `UFlowAsset::RebuildCustomInterfaceLists` - called after
import/patch (`FlowGraphImporter.cpp`'s `PopulateFlowAssetFromText`/`ApplyMutationText` paths both
call it under `WITH_EDITOR`). If a `SubGraph` node's context pins look stale right after an
edit to the sub-asset's `CustomInput`/`CustomOutput` set, this is the mechanism to check - the
parent asset needs its own regraph/reload to pick up an interface change made in the sub-asset.

## Create a sub-graph from a selection

The Flow Courier toolset also exposes the editor's collapse convenience as two agent operations:
`PlanFlowSubgraphFromSelection` and `CreateFlowSubgraphFromSelection`. The caller supplies exact
top-level node GUIDs and a child name beginning with `Subgraph_`. The plan is read-only and reports
the boundary counts, warnings, errors, and names that will become the child interface. The create
operation reruns that plan before applying, replaces the source selection with one `SubGraph` node,
and returns the generated child asset path and replacement node GUID.

The collapse interface is deliberately exec-only. It needs exactly one incoming execution link,
rejects data links crossing the boundary, and keeps the source asset's CustomInput, CustomOutput,
and Finish nodes outside the selection. The first unnamed exit maps to the child's built-in Finish;
named or additional exits become deterministic Custom Outputs. Use the mutation dry run before
applying: it rolls back the source graph and removes the temporary child package. After an apply,
export or diff both the source and child assets because this operation changes two packages.

The generated child is a distinct asset package. Do not assume that asset-level configuration or
domain context is inherited from the parent just because the child runs inside the parent graph.
Inspect the source asset's default subgraph class and configure any required child context explicitly.
The generic operation preserves Flow graph content and interfaces; domain-specific configuration
belongs to the domain's own authoring guidance.

## Function-like reusable chunks

When the same behavior appears several times with only small data or tuning differences, prefer
one sub-graph with a narrow explicit interface over copied node sequences. Treat the sub-graph as a
function-like unit: the parent supplies inputs, the child performs one coherent piece of work, and
the child returns only the results the parent needs.

- Put data inputs on the child `Start` node. The parent `SubGraph` node exposes corresponding data
  pins. Use `AssetParams` when the values are naturally represented by a reusable
  `UFlowAssetParams` asset rather than by per-instance wiring.
- Declare typed child results in `UFlowAsset::OutputDataPinDeclarations`. Write those results with
  `Set Graph Output` or `Finish`; the parent `SubGraph` node then exposes the resulting output data
  pins.
- Use named `CustomInput` and `CustomOutput` for execution control and lifecycle exits, not as data
  arguments. `CustomInput` cannot currently add data pins.
- A `CustomInput` triggers an already-running child instance. It is not automatically a reentrant
  function call that starts a fresh child with a new set of inputs. If independent invocations are
  required, use separate `SubGraph` node instances or a lifecycle and parameter design supported
  by the domain.
- Keep the interface small and semantic. Do not expose every internal value or turn the child into
  a parameterized mini-language. Keep a variant inline or make a separate variant when its topology,
  asset class, or lifecycle meaning is materially different.

This pattern is useful both across parent assets and within one large parent where repeated sections
would otherwise drift. It is a reuse abstraction only when the differences can be expressed through
the declared inputs and outputs; extracting a one-off section for readability is a separate, valid
reason to use a sub-graph.

## When to decompose into a sub-graph vs. keep it inline

- **Reuse across multiple parent graphs** - the strongest signal. If the same chunk of behavior
  needs to appear in more than one asset, it belongs in a sub-graph, not copy-pasted.
- **A large graph that reads better in named sections** - even without reuse, splitting a big
  graph at natural named boundaries (the `CustomInput`/`CustomOutput` names document *why* the
  split happens there) can be more maintainable than one sprawling canvas. Weigh this against the
  cost of an extra asset to open when tracing logic - don't over-decompose a graph that's small
  enough to read in one pass.
- **Isolating a self-contained sub-behavior with its own local state** - a sub-graph is a distinct
  `FlowAsset` instance; anything it does is spatially and temporally contained inside it, which can
  make a self-contained mini-behavior (e.g. a shared side-quest chain, or a reusable lifecycle
  stage) easier to reason about than the same logic inlined into a larger graph. A project's own
  domain-specific Pattern articles may document a shipped example of this shape - check there for
  a real exemplar before hand-writing one.
- **Keep inline when**: the logic is small, used exactly once, and splitting it out would only add
  an extra asset to navigate without a real reuse or readability win.

Additional signals include a repeated section with a stable contract, where variations can be
represented by `Start` data inputs, `AssetParams`, execution entry/exit pins, and declared output
data pins. Setup, phase, cleanup, success, and failure are also useful boundaries when the child can
describe its completion and failure paths without relying on hidden parent implementation details.
Keep a section inline when the boundary would mostly pass through pins, expose tightly coupled
parent-local state, or add an asset hop without making the behavior easier to understand.

## Refactoring checklist

1. Inspect and export the parent graph. Identify the exact top-level region and decide whether the
   goal is readability, reuse, or both.
2. Design the child exec and data interface before moving nodes. Name entry/exit pins by behavior,
   keep data inputs and outputs narrow, and decide whether `AssetParams` is appropriate.
3. Use `PlanFlowSubgraphFromSelection` to check boundary constraints and the generated interface;
   then dry-run `CreateFlowSubgraphFromSelection` before applying it. The collapse operation is
   exec-only, so it cannot create a crossing data-pin interface. Create or patch that child and
   parent interface separately after the extraction when data pins are part of the abstraction.
4. After applying, export or diff both parent and child. Verify the child class/domain settings,
   every entry and completion path, output declarations, and all connections.
5. For deduplication, inspect every former call site. Confirm that each variation is represented by
   an explicit input or output rather than by a hidden copy or parent-specific assumption.

## Pitfalls / notes

- A `CustomInput`/`CustomOutput` name collision inside one sub-asset (two nodes with the same
  `EventName`) is a real risk if hand-editing Flow Courier text - the parent's context pin naming
  depends on `EventName` being unique per direction within that sub-asset.
- Don't confuse this with the `ApplyFlowPatch` reconciler's scoped patching (`scopeNodeGuids`,
  `newAlias` for a new node) - that's a *patching* mechanism for editing one asset's nodes,
  unrelated to `SubGraph`'s *composition* mechanism for nesting one asset inside another.
- `bCanInstanceIdenticalAsset` is off by default for a reason - only enable it with a clear
  termination condition in mind (e.g. a decrementing counter passed via `AssetParams`), never for
  unconditional self-reference.

## Ecosystem links

- $KB:flow:guide:courier-text-format (Subgraph Interface section - Courier JSON fields)

## See also

- $KB:flow:guide:flowgraph-index
