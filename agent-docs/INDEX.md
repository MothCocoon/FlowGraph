# FlowGraph Agent Knowledge Base - Index

$KB:flow:guide:flowgraph-index

## What this is

This plugin's agent-facing knowledge base covers Flow Graph behavior shared by consuming projects.
It lives beside `docs/`, the published website. Start here for concepts and authoring guidance;
query `FindFlowNodeTypes` for facts about a particular node or addon class.

## Placement rule

Shared concepts and authoring guides live here when their behavior is part of this plugin. A
particular class's guidance lives in its `FFlowAgentDoc`, not in a separate article. An extension
plugin or consuming project keeps its own domain-specific documentation beside its code.

The `Authoring/` guide covers C++ and Blueprint. Other authoring languages belong to the plugin or
project that provides them.

## Layout

- **No per-class node/addon articles.** A class's documentation lives on the class itself
  (`FFlowAgentDoc`, read via the catalog with `FindFlowNodeTypes`), not as a markdown file.
- `Concept` articles for generic ecosystem semantics (e.g. the Flow Courier format, the addon attachment handshake, subgraph composition).
- `Authoring/` guides for how to build a new generic node/addon.

## Conventions

Each article declares one `$KB:<domain>:<kind>:<id>` tag on its own line near the top. Use `flow`
for this plugin's domain and `concept`, `pattern`, or `guide` for the kind. Resolve a reference by
searching the exact tag across installed `agent-docs/` roots. The tag id matches the filename after
case, hyphen, and underscore normalization; `INDEX.md` is exempt. Link to the tag rather than to a
section number so links survive file moves.

Keep one concept or pattern per article. Put class-specific guidance in `FFlowAgentDoc`, where the
catalog serves it beside reflected pins and properties. A class's `Articles` entries use
`concept:<id>` or `pattern:<id>` to connect it to shared articles. Mark content that agents may
change only on explicit instruction with `<!-- KB-LOCK -->` and `<!-- /KB-LOCK -->`.

## Available guides

- $KB:flow:guide:courier-text-format - the Courier v2 JSON document: what its fields mean and what
  a document does when applied (identity, apply order, merge behavior, addon parentage, pin names,
  the error model). The exact schema is published live by `describe_toolset`; read this for meaning,
  not shape.

## Scripts (`Scripts/`)

- `kb_lint.py` - generic KB consistency checker (dangling `$KB:` references, filename/tag
  mismatches, orphan articles) across any set of `agent-docs/` roots. Pure Python, no Unreal/MCP
  dependency. Run it on this root alone for a standalone check, or pass extension and project roots
  together when their articles link across roots.

## Concepts (`Concepts/`)

Generic ecosystem semantics that don't belong to any single node/addon class.

- $KB:flow:concept:subgraphs - `FlowNode_SubGraph` composition: embedding one `FlowAsset` inside
  another, the `CustomInput`/`CustomOutput` named-interface mechanism, and when to decompose a
  graph into a sub-asset vs. keep it inline.
- $KB:flow:concept:structuring-coherent-graphs - domain-agnostic composition guidance: node vs
  addon vs pin, multiple entry points vs branch inline, decomposition, readability habits. Project
  domain skills may layer their own domain-specific skeleton patterns on top of this.

## Authoring (`Authoring/`)

How to create new **generic** Flow nodes/addons (no consuming-project dependency). Project-specific
authoring lives with the owning plugin or project.

- $KB:flow:guide:authoring-a-node - node vs addon, C++ vs Blueprint, the creation workflow for each
  (incl. the Flow-Blueprint-asset-type trap), and AddOn attachment eligibility (the two-sided
  accept/reject handshake).
- $KB:flow:concept:declaring-pins - the full pin surface: exec, static, data, auto-generated, and context pins.

## Extensions

An installed extension can add another `agent-docs/` root. Read its index for the behavior it owns;
this index is sufficient for Flow Graph itself.
