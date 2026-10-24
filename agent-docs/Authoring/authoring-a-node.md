# Authoring a Node/AddOn (Guide)

$KB:flow:guide:authoring-a-node
keywords: create new node addon author cpp blueprint native subclass uflownode uflownodeaddon addon attach eligibility accept reject handshake

## Node or AddOn?

- **`UFlowNode`** - a standalone step placed directly in the graph; owns its own input/output pins
  and transitions.
- **`UFlowNodeAddOn`** - attaches *to* a node (or to another AddOn), and only exists in the context
  of its parent. Use an AddOn when the behavior is a modifier/condition/listener on an existing node
  rather than its own step - see "AddOn attachment eligibility" below for how a node/addon controls
  which AddOns may attach to it.

## C++ or Blueprint?

- **C++** when: the logic needs to be perf-sensitive, needs engine-level access not exposed to
  Blueprint, or is a *base class* other Blueprint nodes will derive from (a base class must be
  native or a Blueprint others can further subclass - native is simpler for a shared base).
- **Blueprint** when: the logic is one-off, designer-iterable, and doesn't need to be a further base
  class. Read the Blueprint section below before starting - Flow requires its own Blueprint asset
  type; a plain Blueprint subclassing `UFlowNode` will not appear in the graph editor's palette.

## Authoring in C++

1. Subclass `UFlowNode` or `UFlowNodeAddOn` - or an existing intermediate base if one fits.
2. **No asset step is needed.** A native class is discovered automatically once compiled: the graph
   schema gathers native nodes/addons straight from the reflection system
   (`GatherNativeNodesOrAddOns`), not the asset registry.
3. Override the execution surface you need (all from `IFlowCoreExecutableInterface`):
   - `ExecuteInput(PinName)` - a signal arrived on an input pin; the main entry point.
   - `TriggerOutput(PinName, bFinish)` / `TriggerFirstOutput(bFinish)` - fire an output pin.
   - `OnActivate`, `Cleanup`, `Finish`, `InitializeInstance`/`DeinitializeInstance` as needed.
4. Declare pins - see $KB:flow:concept:declaring-pins (exec, data, auto, context).
5. Restrict which AddOns may attach (if relevant) by overriding `AcceptFlowNodeAddOnChild` /
   `AcceptFlowNodeAddOnParent` - see "AddOn attachment eligibility" below.
6. Editor presentation is set in the constructor under `WITH_EDITOR`/`WITH_EDITORONLY_DATA`:
   `Category`, `NodeDisplayStyle`, node color, tooltip. Add `Keywords` UCLASS metadata so the node is
   discoverable by intent, not just class name.
7. SaveGame/preload hooks (`OnSave`/`OnLoad`, `IFlowPreloadableInterface`) only if the node needs them.

## Authoring in Blueprint

**The trap (read first):** a plain Blueprint that subclasses `UFlowNode` will never appear in the
Flow graph editor's node palette. Flow uses its own Blueprint asset classes -
`UFlowNodeBlueprint` and `UFlowNodeAddOnBlueprint` - and both return
`SupportedByDefaultBlueprintFactory() -> false`, so the standard "Blueprint Class" content-browser
picker cannot create the right asset. The graph schema discovers Blueprint nodes/addons by filtering
the asset registry **specifically for those two asset classes** (`UFlowGraphSchema::GatherNodes`),
so a normal Blueprint asset is invisible to it no matter what it derives from.

1. Create the asset via the **Flow** asset category (the "Flow Node Blueprint" / "Flow Node AddOn
   Blueprint" asset-type actions), *not* the generic Blueprint Class option. This routes through
   `UFlowNodeBlueprintFactory` / `UFlowNodeAddOnBlueprintFactory`.
2. The factory shows a **parent-class picker** - choose the `UFlowNode` (or `UFlowNodeAddOn`)
   subclass to derive from. It must be a `CanCreateBlueprintOfClass` class descending the correct
   default parent, or the factory refuses.
3. The new Blueprint is seeded with default `ExecuteInput` and `Cleanup` event nodes (if
   `bSpawnDefaultBlueprintNodes` is on). Implement your logic in the EventGraph off those events.
4. Declare pins - see $KB:flow:concept:declaring-pins. In Blueprint you get data pins by adding a
   `FFlowDataPinValue_*`-typed property (Blueprint can't set property metadata, so it uses the typed
   wrapper), and exec pins by editing the `InputPins`/`OutputPins` arrays in class defaults.
5. AddOn eligibility overrides (`AcceptFlowNodeAddOnChild`) are `BlueprintNativeEvent`, so they *can*
   be overridden in Blueprint - see "AddOn attachment eligibility" below.
6. Set category/keywords/tooltip in class defaults for discoverability.

**Constraints:** `SupportsDelegates() -> false` on these Blueprint types - Flow node/addon Blueprints
do not support Blueprint delegates. Logic lives in the EventGraph; there is no separate
construction-script story to rely on for runtime behavior.

## AddOn attachment eligibility

How a node or addon **author controls which AddOns may attach to it** (and which parents an AddOn
will accept). This is the *authoring* side of the eligibility question - for a KB consumer, "can
addon X attach to node Y" is answered by the `CheckAddonAttachmentEligibility` op, which wraps
`UFlowNodeBase::CheckAcceptFlowNodeAddOnChild`, but the *rule* it enforces is the one you write here.
Only `TentativeAccept` counts as eligible - a rejection or an unanswered vote from either side blocks
the attachment.

### The two-sided handshake

Attachment is agreed by **both** sides, and either can veto:

- **Parent's vote:** `AcceptFlowNodeAddOnChild(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren)` -
  overridden on the node (or the addon, since AddOns can nest inside AddOns).
- **Child's vote:** `AcceptFlowNodeAddOnParent(ParentTemplate, ...)` - overridden on the AddOn.

Both return `EFlowAddOnAcceptResult`:
- `Undetermined` - no opinion (defer to the other side / default).
- `TentativeAccept` - accept if all other conditions are met.
- `Reject` - veto outright, even if the other side tentatively accepted.

Results are **priority-combined** (`CombineFlowAddOnAcceptResult` takes the max; `Reject` >
`TentativeAccept` > `Undetermined`), so any `Reject` from either side wins. Both are
`BlueprintNativeEvent` (`_Implementation` in C++), so they can be overridden in C++ **or** Blueprint.

The `AdditionalAddOnsToAssumeAreChildren` parameter carries the whole incoming set during a
multi-paste, so a rule that depends on sibling AddOns can decide based on the *whole* set being
pasted, not just the one instance in front of it.

### Common patterns

- **Exclusive child type:** a node accepts *only* one AddOn class and `Reject`s everything else.
- **Family accept:** a node `TentativeAccept`s any AddOn of a given base family (its logic is driven
  by the attached AddOns), deferring to `Super` otherwise.
- **Additive accept:** a node `TentativeAccept`s an extra AddOn family on top of whatever the base
  class already allows (calls `Super` for the rest).
- **Child-side veto:** an AddOn overrides `AcceptFlowNodeAddOnParent` so it only attaches to a
  specific parent type - the child enforcing its own valid parent.

### Guidance

- Always call `Super::AcceptFlowNodeAddOnChild_Implementation(...)` for cases you don't explicitly
  handle, so base-class rules still apply.
- Prefer `TentativeAccept` (not a hard accept) so other conditions and the other side's vote still
  get to weigh in; use `Reject` only to actively forbid.

## Project-layer note

An extension plugin or consuming project documents its own base classes and dynamic pin behavior
beside the code that defines them.

## See also

- $KB:flow:concept:declaring-pins
- $KB:flow:guide:flowgraph-index
