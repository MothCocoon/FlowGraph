# Structuring Coherent Graphs (Concept)

$KB:flow:concept:structuring-coherent-graphs
keywords: structure compose graph coherent decompose node addon pin entry point readability organize

## What this is

Composition guidance for building a Flow graph that stays readable as it grows - the generic
decision points every graph author faces regardless of `FlowAsset` subclass: when to add a new
node vs. an addon vs. a new pin, when to split into multiple entry points vs. branch inline, and
when to decompose into a sub-graph. This is the domain-agnostic counterpart to whatever
project-specific graph-skeleton patterns exist for a project's own `FlowAsset` subclasses - those
are *instances* of the general principles below; check a project's own domain-specific KB/skill
content for concrete examples.

## New node, new addon, or new pin?

- **New node** when the behavior is its own step with its own place in the execution order - it
  needs to *run* at a specific point, not just modify or gate something that's already running.
- **New addon** when the behavior modifies, gates, or listens to an *existing* node rather than
  being its own step - see $KB:flow:guide:authoring-a-node's "Node or AddOn?" section for the
  authoring-side version of this same decision, and its attachment-eligibility section for how a
  node controls which addons may legally attach to it.
- **New pin** (on an existing node) only when the node's own class needs to *expose* a new value or
  branch as part of its contract - see $KB:flow:concept:declaring-pins for the full pin
  taxonomy (exec/static/data/auto-generated/context). Don't reach for a new pin as a workaround for
  "I want this node to also do X" - that's almost always a new node or addon instead.

## Multiple entry points vs. branch inline

A graph with several distinct triggers (e.g. "the normal path" and "an interrupt/cancel path") has
two shapes available:
- **Multiple independent entry-rooted chains** sharing one graph canvas but not connected to each
  other - each entry Event/CustomInput roots its own sub-graph. This is the right shape when the
  triggers are genuinely independent and can fire in any order/combination, not sequenced relative
  to each other. A project's own domain-specific entry-chain patterns (multiple named events or
  phases sharing one graph) are typically instances of this shape.
- **A single entry with an inline `Branch`/`Switch`** - the right shape when the "different cases"
  are mutually exclusive alternatives *within* one triggered sequence, not independently-triggerable
  events. The underlying `FlowNode_Branch`/`FlowNode_Switch`/predicate-addon mechanics are generic
  even where a project's own conditional-routing guidance illustrates them with project-specific
  exemplars.

Don't force independent triggers into a single entry point with a big branch just to avoid multiple
entry nodes - that couples otherwise-unrelated trigger conditions into one node's gating logic, and
new triggers can't be added without touching an established node.

## When to decompose into a sub-graph

See $KB:flow:concept:subgraphs in full, especially "Function-like reusable chunks", "When to
decompose", and "Refactoring checklist". Reuse across multiple parent graphs is the strongest
signal, followed by readability at natural named boundaries, isolating a self-contained
sub-behavior with its own local state, and extracting repeated behavior whose small variations can
be expressed through a narrow input/output contract. Don't decompose a small, single-use chunk of
logic just because it's *possible* to - that adds an asset to navigate without a real win.

## Readability habits worth adopting regardless of domain

- **Name entry/exit `CustomInput`/`CustomOutput`/Event nodes for what triggers or completes them**,
  not for their position in the graph (`MainContent`, not `Section2`) - the name is often the only
  documentation a reader gets before opening the node.
- **Keep an entry chain's node count proportional to what it actually does** - if one chain has
  grown to do three unrelated things, that's a signal it should split into three chains (multiple
  entry points) or delegate two of them to addons/a sub-graph, not a signal to add more branches.
- **Prefer `FlowNode_Reroute` for pure wire-tidiness over letting connections cross the whole
  canvas** - reroutes carry no behavior and exist purely to keep a graph's layout legible. After
  placing one, run the `AutoFormatFlowGraph` op (see the `flow` skill's ops reference) to recompute
  node positions for the new layout - it does not place reroute nodes itself, only lays out what's
  already there.

## Ecosystem links

- $KB:flow:concept:subgraphs
- $KB:flow:concept:declaring-pins
- $KB:flow:guide:authoring-a-node

## See also

- $KB:flow:guide:flowgraph-index
