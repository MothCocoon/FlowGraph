#include "Graph/FlowGraphConnectionDrawingPolicy.h"

#include "FlowEditorSettings.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "Misc/App.h"

FConnectionDrawingPolicy* FFlowGraphConnectionDrawingPolicyFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UFlowGraphSchema::StaticClass()))
	{
		return new FFlowGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
	return nullptr;
}

/////////////////////////////////////////////////////
// FFlowGraphConnectionDrawingPolicy

class UGraphEditorSettings;

FFlowGraphConnectionDrawingPolicy::FFlowGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	// Cache off the editor options
	RecentWireDuration = UFlowEditorSettings::Get()->RecentWireDuration;

	RecentColor = UFlowEditorSettings::Get()->RecentWireColor;
	RecordedColor = UFlowEditorSettings::Get()->RecordedWireColor;
	InactiveColor = UFlowEditorSettings::Get()->InactiveWireColor;

	RecentWireThickness = UFlowEditorSettings::Get()->RecentWireThickness;
	RecordedWireThickness = UFlowEditorSettings::Get()->RecordedWireThickness;
	InactiveWireThickness = UFlowEditorSettings::Get()->InactiveWireThickness;

	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}

void FFlowGraphConnectionDrawingPolicy::BuildPaths()
{
	if (const UFlowAsset* FlowInstance = CastChecked<UFlowGraph>(GraphObj)->GetFlowAsset()->GetInspectedInstance())
	{
		const double CurrentTime = FApp::GetCurrentTime();

		TArray<UFlowNode*> Nodes;
		FlowInstance->GetRecordedNodes(Nodes);
		for (const UFlowNode* Node : Nodes)
		{
			const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node->GetGraphNode());

			for (const TPair<uint8, FPinRecord>& Record : Node->GetWireRecords())
			{
				if (UEdGraphPin* OutputPin = FlowGraphNode->OutputPins[Record.Key])
				{
					// check if Output pin is connected to anything
					if (OutputPin->LinkedTo.Num() > 0)
					{
						RecordedPaths.Add(OutputPin, OutputPin->LinkedTo[0]);

						if (CurrentTime < Record.Value.Time + RecentWireDuration)
						{
							RecentPaths.Add(OutputPin, OutputPin->LinkedTo[0]);
						}
					}
				}
			}
		}
	}
}

void FFlowGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	BuildPaths();

	// Draw everything
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
void FFlowGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;

	// Get the schema and grab the default color from it
	check(OutputPin);
	check(GraphObj);
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	if (OutputPin->bOrphanedPin || (InputPin && InputPin->bOrphanedPin))
	{
		Params.WireColor = FLinearColor::Red;
	}
	else
	{
		Params.WireColor = Schema->GetPinTypeColor(OutputPin->PinType);

		if (InputPin)
		{
			// recent paths
			if (RecentPaths.Contains(OutputPin) && RecentPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecentColor;
				Params.WireThickness = RecentWireThickness;
				Params.bDrawBubbles = true;

				return;
			}

			// all paths, showing graph history
			if (RecordedPaths.Contains(OutputPin) && RecordedPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecordedColor;
				Params.WireThickness = RecordedWireThickness;
				Params.bDrawBubbles = false;

				return;
			}

			// It's not followed, fade it and keep it thin
			Params.WireColor = InactiveColor;
			Params.WireThickness = InactiveWireThickness;
		}
	}
}
