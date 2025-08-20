// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"
#include "Runtime/Launch/Resources/Version.h"

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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
#else
	virtual void DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params);
#endif
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	// End of FConnectionDrawingPolicy interface

protected:
	void DrawCircuitSpline(const int32& LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params) const;
	void DrawCircuitConnection(const int32& LayerId, const FVector2f& Start, const FVector2f& StartDirection, const FVector2f& End, const FVector2f& EndDirection, const FConnectionParams& Params) const;
	static FVector2f GetControlPoint(const FVector2f& Source, const FVector2f& Target);

	bool ShouldChangeTangentForReroute(class UFlowGraphNode_Reroute* Reroute);
	bool FindPinCenter(const UEdGraphPin* Pin, FVector2D& OutCenter) const;
	bool GetAverageConnectedPosition(class UFlowGraphNode_Reroute* Reroute, EEdGraphPinDirection Direction, FVector2D& OutPos) const;
};
