# Declaring Pins on a Node/AddOn (Concept)

$KB:flow:concept:declaring-pins
keywords: pin pins input output exec execution data pin context pin auto pin numbered pin FlowPinType SourceForOutputFlowPin DefaultForInputFlowPin FFlowDataPinValue TrySupplyDataPin TryResolveDataPin

## What this is

The full set of ways a `UFlowNode`/`UFlowNodeAddOn` author declares pins. This is **not one
mechanism** - there are several overlapping systems (exec pins, static pins, typed data pins, several
auto-generation pathways, context pins) and which one to reach for depends on the pin's purpose and
whether you're authoring in C++ or Blueprint.

Two orthogonal axes run through everything below:
- **Direction:** input vs output.
- **Kind:** *exec* pin (a `FFlowPin` whose type is `Exec` - carries execution/transition flow) vs
  *data* pin (any other `FFlowPin` type - carries a typed value). `FFlowPin::IsExecPin()` /
  `IsDataPin()` is literally "is the type Exec or not".

## 1. Exec pins (execution flow)

The default execution pins. `UFlowNode::DefaultInputPin` / `DefaultOutputPin` are the implicit
single in/out most nodes have. To declare your own:

- **Named exec pins (C++):** add `FFlowPin`s to the `InputPins` / `OutputPins` arrays in the
  constructor, or call `AddInputPins(...)` / `AddOutputPins(...)`. `InputPins`/`OutputPins` are
  `EditDefaultsOnly`, so a Blueprint author sets them in the class-defaults panel instead of code.
- **Numbered exec pins:** `SetNumberedInputPins(First, Last)` / `SetNumberedOutputPins(First, Last)`
  generate a contiguous numbered range (e.g. an Execution Sequence's `0,1,2,...`). `CountNumbered*`
  reports how many exist.
- **User-addable pins (editor):** override `CanUserAddInput()` / `CanUserAddOutput()` (or the
  `K2_` Blueprint events) to let a designer add/remove pins on a placed instance; `RemoveUserInput/
  Output` handle removal.

## 2. Data pins (typed values)

A data pin carries a typed value. Supported types (`EFlowPinType`): `Bool`, `Int`, `Float`, `Name`,
`String`, `Text`, `Enum`, `Vector`, `Rotator`, `Transform`, `GameplayTag`, `GameplayTagContainer`,
`InstancedStruct`, `Object`, `Class`. Each can be **Single** or **Array** (`EFlowDataMultiType` /
`EPinContainerType`).

Two sides to every data pin:
- **Consuming a value (input side):** call `TryResolveDataPinValue<TPinType>(PinName, OutValue)` /
  `TryResolveDataPinValues<...>` (array). Returns an `EFlowDataPinResolveResult` (`Success`,
  `FailedUnknownPin`, `FailedMismatchedType`, `FailedNotConnected`, ...). If the input pin is
  connected, the upstream supplier's value is used; if not, the pin's bound default is used.
- **Supplying a value (output side):** either bind a property (auto pins, below) or override
  `TrySupplyDataPin(PinName)` to return an `FFlowDataPinResult` by hand (see the manual pathway).

## 3. Auto-generated data pins (property binding)

The most common way to get data pins: **bind a pin to a property** and let the node auto-generate the
pin. The binding metadata can live in **two different places**, which is the key thing to understand:

### 3a. Metadata on the UPROPERTY (C++ only)

C++ can put the metadata directly on a specific property instance, which gives the most control:

- `meta = (FlowPinType = "Bool")` - auto-generate a data pin of this type bound to the property.
- `meta = (SourceForOutputFlowPin, FlowPinType = "Bool")` - the pin is an **output** whose value is
  *sourced from* this property.
- `meta = (DefaultForInputFlowPin, FlowPinType = "Bool")` - the pin is an **input** whose *default
  value* is this property (used only when the pin is unconnected).
- A **string value** on `SourceForOutputFlowPin`/`DefaultForInputFlowPin` renames the pin
  (e.g. `SourceForOutputFlowPin = "Renamed Bool Output"`); with no string the property's display/
  authored name is used.
- Works on a plain scalar **or** a `TArray<>` (array container pin).

(See `FlowNode_DataPinsTester_Bool` in the test plugin for all of these side by side.)

### 3b. Metadata on the USTRUCT/UCLASS via a typed wrapper property (`FFlowDataPinValue_*`)

Blueprint cannot put arbitrary metadata on a property. Instead it declares
a property of a **typed wrapper struct** - `FFlowDataPinValue_Bool`, `FFlowDataPinValue_Int`, etc. -
whose `FlowPinType` metadata lives on the *struct declaration*. The pin auto-generates from the
struct's type; the wrapper's `bIsInputPin` / `MultiType` fields choose direction and single-vs-array.
These wrappers work in C++ too, and are the same `FFlowDataPinValue` variant type the resolve/supply
pipeline passes around internally (`FFlowDataPinResult::ResultValue` is a `FFlowDataPinValue`), so
they double as a convenient value container.

> **This is the C++-vs-Blueprint difference in one sentence:** C++ can annotate any property
> (3a); Blueprint instead declares a `FFlowDataPinValue_*`-typed property (3b). Both
> produce the same auto-generated pin.

- **Legacy:** `FFlowDataPinInputProperty_*` / `FFlowDataPinOutputProperty_*` are an older wrapper
  pathway (`#FlowDataPinLegacy`) - recognize them in existing code, prefer `FFlowDataPinValue_*` for
  new work.

### 3c. Under the hood

Auto pins are (re)built in-editor into the `AutoInputDataPins` / `AutoOutputDataPins` arrays, with a
`MapDataPinNameToPropertySource` recording non-trivial pin-name -> property-owner mappings
(`FFlowAutoDataPinsWorkingData` does the build, handling duplicate-name disambiguation). You don't
call this directly; it's driven by the metadata/wrapper declarations above.

## 4. Manual data pins (declare + supply by hand)

For full control, declare the pin yourself and supply its value yourself - no property binding:

1. In the constructor, add the pin with an explicit type:
   `OutputPins.Add(FFlowPin(PinName, FFlowPinType_String::GetPinTypeNameStatic()))`.
2. Override `TrySupplyDataPin(PinName)` to return `FFlowDataPinResult(FFlowDataPinValue_String(Value))`
   for your pin, deferring to `Super` otherwise.

(See `FlowNode_DataPinsTester_Manual`.) Use this when the value is computed, not stored in a bindable
property.

## 5. Instance-level dynamic pins (`FFlowNamedDataPinProperty`)

For nodes that let a designer add **named data properties (and their pins) on a placed instance** at
edit time (rather than compile time) - e.g. a Flow Asset's parameters or start node. Each entry pairs
a `Name` with a `FFlowDataPinValue` payload and generates a matching pin. This is a niche authoring
surface; reach for it only when instance-authored pins are the actual requirement.

## 6. Context pins (`IFlowContextPinSupplierInterface`)

Pins that a node adds/removes **dynamically based on its own configuration or subobjects** - e.g. a
node referencing a Level Sequence exposing one pin per sequence event. Implement:
`SupportsContextPins()`, `GetContextInputs()`, `GetContextOutputs()` (all `BlueprintNativeEvent`,
editor-only). Unlike auto pins (driven by property metadata) these are computed by your own code.
`CanRefreshContextPinsOnLoad()` is off by default and should stay off unless cheap - refreshing can
force-load referenced assets.

## Choosing

- Execution flow -> **exec pins** (§1).
- A fixed typed value stored in a property -> **auto pin** (§3): property metadata in C++ (3a), or a
  `FFlowDataPinValue_*` property in Blueprint (3b).
- A computed typed value -> **manual supply** (§4).
- Pins whose *set* depends on config/subobjects -> **context pins** (§6).
- Designer-authored pins on the instance -> **`FFlowNamedDataPinProperty`** (§5).

## Project-layer note

An extension plugin or consuming project may add dynamic pin sources beyond those described here.
Read its own authoring guide for those declarations and generation hooks.

## See also

- $KB:flow:guide:authoring-a-node
