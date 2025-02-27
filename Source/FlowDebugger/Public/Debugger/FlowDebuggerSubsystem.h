// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "Debugger/FlowDebuggerTypes.h"
#include "FlowDebuggerSubsystem.generated.h"

class UEdGraphNode;
class UEdGraphPin;

/**
** Persistent subsystem supporting Flow Graph debugging
 */
UCLASS()
class FLOWDEBUGGER_API UFlowDebuggerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UFlowDebuggerSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void PausePlaySession() {}
	virtual bool IsPlaySessionPaused() { return false; }

	/** Adds trait with provided Type to OwnerNode. Node cannot accept traits of the same type */
	virtual void CreateTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bEnabled);
	/** Adds trait with provided Type to OwnerPin. Pin cannot accept traits of the same type */
	virtual void CreateTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bEnabled);

	virtual void RemoveTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type);
	virtual void RemoveTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	virtual void RemoveNodeTraitByPredicate(const UEdGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);
	virtual void RemovePinTraitByPredicate(const UEdGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);
	virtual void RemovePinTraitByPredicate(const UEdGraphPin* OwnerPin, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);

	virtual void ClearNodeTraits(const UEdGraphNode* OwnerNode);
	virtual void ClearPinTraits(const UEdGraphNode* OwnerNode);
	virtual void ClearPinTraits(const UEdGraphPin* OwnerPin);
	/** Removes stale pin traits for provided OwnerNode. Pin list can be changed after node reconstructing and traits for removed pins can stay in PerNodeSettings.
	 * There is no need to clean node's traits here, cause all node events can be processed right away and there will be no stale nodes in PerNodeSettings */
	virtual void CleanupTraits(const UEdGraphNode* OwnerNode);

	/** Finds OwnerNode's trait with provided Type
	 * returns null if there are no OwnerNode's trait with provided Type */
	virtual FFlowDebugTrait* FindTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type);
	/** Finds OwnerPin's trait with Type
	 * returns null if there are no OwnerPin's trait with provided Type */
	virtual FFlowDebugTrait* FindTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	virtual void SetTraitEnabled(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bIsEnabled);
	virtual void SetTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bIsEnabled);

	virtual bool IsTraitEnabled(const UEdGraphNode* OwnerNode, EFlowTraitType Type);
	virtual bool IsTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	virtual void ToggleTrait(const UEdGraphNode* OwnerNode, EFlowTraitType Type);
	virtual void ToggleTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	virtual TArray<EFlowTraitType> SetAllTraitsHit(const UEdGraphNode* OwnerNode, bool bHit);
	virtual TArray<EFlowTraitType> SetAllTraitsHit(const UEdGraphPin* OwnerPin, bool bHit);

	virtual bool SetTraitHit(const UEdGraphNode* OwnerNode, EFlowTraitType Type, bool bHit);
	virtual bool SetTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bHit);

	virtual bool IsTraitHit(const UEdGraphNode* OwnerNode, EFlowTraitType Type);
	virtual bool IsTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/**	Retrieves the user settings associated with a FlowGraphNode.
	*	returns null if the FlowGraphNode has default settings (no nodes and pins traits) */
	virtual FFlowTraitSettings* GetPerNodeSettings(const UEdGraphNode* OwnerNode);

	/**	Retrieves the Array of node's traits associated with a FlowGraphNode.
	*	returns null if there are no node's traits associated with this FlowGraphNode */
	virtual TArray<FFlowDebugTrait>* GetNodeTraits(const UEdGraphNode* OwnerNode);

	/**	Retrieves the Array of pins' traits associated with a FlowGraphNode.
	*	returns null if there are no pins' traits associated with this FlowGraphNode */
	virtual TArray<FFlowDebugTrait>* GetPinTraits(const UEdGraphNode* OwnerNode);

protected:	
	/** Saves any modifications made to traits */
	virtual void SaveSettings();
};
