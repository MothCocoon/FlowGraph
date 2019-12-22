#pragma once

#include "ConnectionDrawingPolicy.h"
#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "Layout/ArrangedWidget.h"
#include "Widgets/SWidget.h"

class FSlateWindowElementList;
class UEdGraph;

struct FFlowGraphConnectionDrawingPolicyFactory : public FGraphPanelPinConnectionFactory
{
public:
	virtual ~FFlowGraphConnectionDrawingPolicyFactory() {}

	// FGraphPanelPinConnectionFactory
	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
	// ~FGraphPanelPinConnectionFactory
};

class FSlateWindowElementList;
class UEdGraph;

// This class draws the connections between nodes
class FFlowGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
private:
	float RecentWireDuration;

	FLinearColor RecentColor;
	FLinearColor RecordedColor;
	FLinearColor InactiveColor;

	float RecentWireThickness;
	float RecordedWireThickness;
	float InactiveWireThickness;

	// runtime values
	UEdGraph* GraphObj;
	TMap<UEdGraphPin*, UEdGraphPin*> RecentPaths;
	TMap<UEdGraphPin*, UEdGraphPin*> RecordedPaths;

public:
	FFlowGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	void BuildPaths();

	// FConnectionDrawingPolicy interface
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override;
	// End of FConnectionDrawingPolicy interface
};