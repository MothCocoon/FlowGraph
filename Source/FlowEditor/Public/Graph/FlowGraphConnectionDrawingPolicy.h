// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

UENUM()
enum class EFlowConnectionDrawType : uint8
{
	Default,
	Circuit
};

struct FFlowGraphConnectionDrawingPolicyFactory : public FGraphPanelPinConnectionFactory
{
	virtual ~FFlowGraphConnectionDrawingPolicyFactory() override
	{
	}

	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
};

class FSlateWindowElementList;
class UEdGraph;

// This class draws the connections between nodes
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

	// runtime values
	UEdGraph* GraphObj;
	TMap<UEdGraphPin*, UEdGraphPin*> RecentPaths;
	TMap<UEdGraphPin*, UEdGraphPin*> RecordedPaths;
	TMap<UEdGraphPin*, UEdGraphPin*> SelectedPaths;

	//Used to help reversing pins on nodes that go backwards
	TMap<class UFlowGraphNode_Reroute*, bool> RerouteToReversedDirectionMap;

public:
	FFlowGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	void BuildPaths();

	// FConnectionDrawingPolicy interface
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	// End of FConnectionDrawingPolicy interface

protected:
	void DrawCircuitSpline(const int32& LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) const;
	void DrawCircuitConnection(const int32& LayerId, const FVector2D& Start, const FVector2D& StartDirection, const FVector2D& End, const FVector2D& EndDirection, const FConnectionParams& Params) const;
	static FVector2D GetControlPoint(const FVector2D& Source, const FVector2D& Target);

	bool ShouldChangeTangentForReroute(class UFlowGraphNode_Reroute* Reroute);
	bool FindPinCenter(UEdGraphPin* Pin, FVector2D& OutCenter) const;
	bool GetAverageConnectedPosition(class UFlowGraphNode_Reroute* Reroute, EEdGraphPinDirection Direction, FVector2D& OutPos) const;
};
