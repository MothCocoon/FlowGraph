// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

class FSlateWindowElementList;
class UEdGraph;
class UEdGraphNode;

UENUM()
enum class EFlowConnectionDrawType : uint8
{
	Default,
	Circuit
};

struct FLOWEDITOR_API FFlowGraphConnectionDrawingPolicyFactory : public FGraphPanelPinConnectionFactory
{
	virtual ~FFlowGraphConnectionDrawingPolicyFactory() override
	{
	}

	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
};

/**
 * This class draws the connections between nodes.
 * Backwards connections that do not have explicit reroute nodes are automatically curved to help mantain readability.
 */
class FLOWEDITOR_API FFlowGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
	float RecentWireDuration;

	FLinearColor InactiveColor;
	FLinearColor RecentColor;
	FLinearColor RecordedColor;
	FLinearColor SelectedColor;

	float InactiveWireThickness;
	float RecentWireThickness;
	float RecordedWireThickness;
	float SelectedWireThickness;

	// Runtime values
	UEdGraph* GraphObj;
	TMap<UEdGraphPin*, UEdGraphPin*> RecentPaths;
	TMap<UEdGraphPin*, UEdGraphPin*> RecordedPaths;
	TMap<UEdGraphPin*, UEdGraphPin*> SelectedPaths;

	/* Used to help reversing pins on nodes that go backwards. */
	TMap<class UFlowGraphNode_Reroute*, bool> RerouteToReversedDirectionMap;

public:
	FFlowGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// Exporting this class forces the implicit copy operations to be emitted, and those touch FConnectionDrawingPolicy::LocalMousePosition, deprecated in UE 5.8. 
	// The factory only ever heap-allocates this policy, so drop the copy operations rather than silence the warning.
	FFlowGraphConnectionDrawingPolicy(const FFlowGraphConnectionDrawingPolicy&) = delete;
	FFlowGraphConnectionDrawingPolicy& operator=(const FFlowGraphConnectionDrawingPolicy&) = delete;

	void BuildPaths();

	// FConnectionDrawingPolicy
	virtual void DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params);
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	// --

protected:
	void DrawCircuitSpline(const int32& LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params) const;
	void DrawCircuitConnection(const int32& LayerId, const FVector2f& Start, const FVector2f& StartDirection, const FVector2f& End, const FVector2f& EndDirection, const FConnectionParams& Params) const;
	static FVector2f GetControlPoint(const FVector2f& Source, const FVector2f& Target);

	/**
	 * Automatically curves backwards connections if no reroute node is present.
	 */
	void DrawDefaultConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params);

	bool ShouldChangeTangentForReroute(class UFlowGraphNode_Reroute* Reroute);
	bool FindPinCenter(const UEdGraphPin* Pin, FVector2D& OutCenter) const;
	bool GetAverageConnectedPosition(class UFlowGraphNode_Reroute* Reroute, EEdGraphPinDirection Direction, FVector2D& OutPos) const;

	/** The top and bottom edges of a node's arranged geometry, in the same absolute/panel space as pin geometry. */
	struct FNodeVerticalExtent
	{
		float Top = 0.0f;
		float Bottom = 0.0f;
	};

	/**
	 * Finds the vertical extent of the node that owns the given pin, using the geometry cached by the most
	 * recent Draw() call.
	 * @param Pin        Pin belonging to the node to look up.
	 * @param OutExtent  Populated with the node's top/bottom edges on success.
	 * @return True if the owning node's arranged geometry was found.
	 */
	bool TryGetOwningNodeVerticalExtent(const UEdGraphPin* Pin, FNodeVerticalExtent& OutExtent) const;

private:
	/** Arranged node geometry for the panel currently being painted, indexed by the node object. Rebuilt every Draw(). */
	TMap<const UEdGraphNode*, int32> NodeToArrangedIndexMap;

	/** The ArrangedNodes array passed into the most recent Draw() call, valid only while that Draw() is on the stack. */
	FArrangedChildren* CurrentArrangedNodes = nullptr;
};
