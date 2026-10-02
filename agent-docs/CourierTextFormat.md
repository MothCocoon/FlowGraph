# Flow Courier v2

$KB:flow:guide:courier-text-format
keywords: courier v2 json format document patch mode assetclass worldbound ops

## What this is

What the fields of a Courier v2 document mean, and the rules that decide what a document does when
it is applied. Courier v2 is JSON, and one document type serves both a complete graph dump and a
narrow patch.

The exact schema - every field, every type - is published live by `describe_toolset`, reflected from
the C++ structs. Read that for shapes. Read this for meaning.

## The document

```json
{
  "formatVersion": 2,
  "mode": "Patch",
  "assetClass": "/Script/Flow.FlowAsset",
  "bWorldBound": true,
  "ops": []
}
```

`formatVersion` must be 2. Any other value is rejected; there is no version coercion.

`mode` is `Full` or `Patch`, and it changes only what **omission** means. In `Patch`, anything the
document does not mention is left alone. In `Full`, the document is the whole graph, and anything on
the asset it does not mention is deleted - nodes and connections alike.

`assetClass` is the full class path. It is required when creating an asset, and on an existing asset
it must match.

`bWorldBound` and `expectedOwnerClass` are asset header values that round-trip.

`scopeNodeGuids` belongs to `Patch` mode only. It names the nodes this document is authoritative
over: a listed node with no `UpsertNode` op is deleted, exactly as if it had been delete-marked.
Omitting it, or leaving it empty, means nothing is implicitly deleted. Supplying it in `Full` mode is
an error, because `Full` already claims authority over everything.

`ops` is the list of operations.

## Ops

Six kinds: `UpsertNode`, `DeleteNode`, `UpsertAddon`, `DeleteAddon`, `AddConnection`,
`RemoveConnection`.

**Identity is two fields, and exactly one is set.** `guid` names something that already exists.
`newAlias` names something being created, and gives it a document-local handle other ops can point
at. Setting both, or neither, is an error rather than a guess. A delete op takes `guid` only.

**Addon ops name their owner, and never nest.** Every `UpsertAddon` and `DeleteAddon` carries
`parentGuid` or `parentAlias` - again exactly one - and that parent may be a node or another addon.
Arbitrary depth is expressed by parentage alone. There is no nesting in the document and no
significance to ordering.

**Connection ops carry `source` and `target`**, each an object of `nodeGuid` or `nodeAlias`, plus
`pin`.

**`type` is required when creating**, and when supplied on an update it must equal the existing
object's class. A node's class cannot be changed while keeping its GUID, so a mismatch is a hard
error rather than a field that quietly does nothing.

A field that a kind does not use must be absent. Supplying one is an error, which is what stops a
misplaced field from silently having no effect.

### Upsert is a merge

An `UpsertNode` changes only what it carries. An absent property is untouched, not cleared. Identity,
editor position and unmentioned addons all survive. This is why the smallest document that expresses
the change is also the safest one.

The exception is `bReplaceAddons` on an `UpsertNode`, which makes that document's addon ops the
complete list for that node: any existing addon it does not mention is deleted. It defaults to false,
which is merge, and it governs addons only - never pins or connections.

### Apply order is fixed

Ops apply in order of kind, not in the order written:

1. `UpsertNode`
2. `UpsertAddon`
3. `AddConnection`
4. `RemoveConnection`
5. `DeleteAddon`
6. `DeleteNode`
7. Deletions implied by `scopeNodeGuids`, or by `Full` mode

Document order is preserved within a kind. Because deletes run last, a document can safely connect
around a node it also deletes, with no ordering care.

## Pins

A pin is an object: `name`, `type`, and an optional `subCategoryPath` for a pin carrying a specific
struct or enum type that must survive round-trip.

**Pin names are carried verbatim.** They are never trimmed, normalised or special-cased. Some nodes
deliberately use a whitespace-only name to render a blank pin label, and `"name": " "` expresses that
with no ceremony.

Declare a creating node's pins in `inputPins` and `outputPins`. An export carries them, so carrying
them forward is the reliable move; the importer creates any declared pin that the class defaults do
not already provide, and a declared-but-unconnected pin round-trips with its type intact.

After a patch that adds a node and wires it up, confirm on the dry run that `connectionsAdded`
contains the connections you expect. A connection to a pin that did not materialise is the one
failure that can be reported as a success.

## Property values

Property values are **Unreal export text**, carried inside JSON strings. The JSON layer owns quoting
and escaping; the value inside is whatever Unreal's own text format says it is. Do not write JSON
structures for a property value.

- Array: `(a,b,c)` - parentheses, no spaces after commas. Not `[a, b, c]`.
- Struct: `(Field1=Value1,Field2=Value2)` - `=` for assignment, parentheses not braces.
- Array of structs: `((Field1=A,Field2=B),(Field1=C,Field2=D))`.
- Instanced struct: the **full** struct path followed by its fields -
  `/Script/Flow.FlowDataPinValue_Text(Values=("text"))`, never the short C++ name.
- Booleans are lowercase `true`/`false`. Integers and floats are bare. Names are unquoted. Strings
  and `FText` are quoted, and a string value round-trips whether or not you supply the quotes.
- An object reference is a full object path.
- An enum is its value name.

A text-valued data pin needs its `Values` array wrapper:
`((Name="PropName",DataPinValue=/Script/Flow.FlowDataPinValue_Text(Values=("text value"))))`.

Generated state is not exported. Only editable properties are written out, on the principle that a
node rebuilds what it generates. Prefer setting a property directly over wiring a data pin whenever
the value is known at design time; reserve a pin for a value another node computes.

## Fan-out

**An exec output pin drives one target. A data output pin may feed several.** `ApplyFlowPatch`
retargets an existing exec connection when asked to add a different target from the same output;
the plan reports the old connection as removed. A final graph with two distinct targets from one
exec output is invalid. Data fan-out is legal and does not warn.

To fan exec flow out, route it through a node built for it. `FlowNode_ExecutionSequence` has exactly
two output pins, `0` and `1`, fixed by its class defaults; chain a second one into `1` for a third
branch.

## When something is wrong

Pre-commit validation findings are returned as structured data. Each finding carries a severity,
a stable code, the offending location, and a message. A malformed document or an execution failure
can instead raise a tool error, depending on the MCP host.

An `Error` validation finding blocks the whole apply and leaves the asset untouched. Check
`bApplySucceeded` and the findings rather than treating a successful transport call as proof that
the graph changed.

Codes cover the shapes worth naming: an unknown field, a bad format version, an ambiguous or missing
identity or parent, a field illegal for its op kind, a missing required field, an unparseable GUID,
an alias referenced but never defined, a connection to a node that will not exist, a class mismatch,
an addon the attachment handshake rejects, exec fan-out, and scope supplied in `Full` mode. Unknown
input is always an error and never ignored.

## Worked examples

Change one property:

```json
{ "formatVersion": 2, "mode": "Patch",
  "ops": [ { "kind": "UpsertNode", "guid": "77EF54AD451D28507162C089F5EDAE8D",
             "properties": { "CooldownSeconds": "4.0" } } ] }
```

Insert a node between two others:

```json
{ "formatVersion": 2, "mode": "Patch", "ops": [
  { "kind": "UpsertNode", "newAlias": "gate", "type": "/Script/Flow.FlowNode_Branch",
    "inputPins":  [ { "name": "In",   "type": "Exec" } ],
    "outputPins": [ { "name": "True", "type": "Exec" }, { "name": "False", "type": "Exec" } ] },
  { "kind": "AddConnection",
    "source": { "nodeGuid": "3EDEC25D4ACBF28B7722319EA818506E", "pin": "Next" },
    "target": { "nodeAlias": "gate", "pin": "In" } },
  { "kind": "AddConnection",
    "source": { "nodeAlias": "gate", "pin": "True" },
    "target": { "nodeGuid": "8A036D354A528E37C2C7D5B086FBA0AF", "pin": "Enter" } } ] }
```

Attach an addon, then nest one under it:

```json
{ "formatVersion": 2, "mode": "Patch", "ops": [
  { "kind": "UpsertAddon", "newAlias": "outer", "parentGuid": "8A036D354A528E37C2C7D5B086FBA0AF",
    "type": "/Script/Flow.FlowNodeAddOn_SwitchCase", "properties": { "CaseName": "Outer" } },
  { "kind": "UpsertAddon", "newAlias": "inner", "parentAlias": "outer",
    "type": "/Script/Flow.FlowNodeAddOn_SwitchCase", "properties": { "CaseName": "Inner" } } ] }
```

Delete a node and bridge around it - no ordering care needed, because deletes run last:

```json
{ "formatVersion": 2, "mode": "Patch", "ops": [
  { "kind": "AddConnection",
    "source": { "nodeGuid": "AAAAAAAA4ACBF28B7722319EA818506E", "pin": "Next" },
    "target": { "nodeGuid": "CCCCCCCC4A528E37C2C7D5B086FBA0AF", "pin": "Enter" } },
  { "kind": "DeleteNode", "guid": "BBBBBBBB451D28507162C089F5EDAE8D" } ] }
```

## Subgraph interface

A `FlowAsset` used as a subgraph exposes named entry and exit points beyond the built-in `Start` and
`Finish`. Add a `FlowNode_CustomInput` with its `EventName` set, and the referencing
`FlowNode_SubGraph` node gains a matching input pin; add a `FlowNode_CustomOutput` for an output pin.
Connect to them like any other exec pin.

Set `EventName` with `"properties": { "EventName": "<name>" }` on that node's `UpsertNode` op - this
works despite `EventName` being absent from both `FindFlowNodeTypes`' `properties` section and
`ExportFlowAsset`'s exported `properties` bag for these two classes. Applying it changes the node's
title (e.g. `"<name> Input"`), although neither read operation exposes the field itself. Do not
conclude from an
empty `properties` object that `EventName` is unset or unreachable - it means the read side can't
show it, not that the node has no name. To find a `FlowNode_CustomInput`/`FlowNode_CustomOutput` by
its `EventName` without already knowing its GUID, use `FindFlowNodes` and read `title`, not
`ExportFlowAsset`'s `properties`.

## See also

- $KB:flow:guide:flowgraph-index
