// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphConnectionDrawingPolicy.h"

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "FlowEditorLogChannels.h"
#include "Graph/Nodes/FlowGraphNode_Reroute.h"
#include "Nodes/FlowNode.h"

#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphConnectionDrawingPolicy)

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

FFlowGraphConnectionDrawingPolicy::FFlowGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	// Cache off the editor options
	RecentWireDuration = UFlowGraphSettings::Get()->RecentWireDuration;

	InactiveColor = UFlowGraphSettings::Get()->InactiveWireColor;
	RecentColor = UFlowGraphSettings::Get()->RecentWireColor;
	RecordedColor = UFlowGraphSettings::Get()->RecordedWireColor;
	SelectedColor = UFlowGraphSettings::Get()->SelectedWireColor;

	InactiveWireThickness = UFlowGraphSettings::Get()->InactiveWireThickness;
	RecentWireThickness = UFlowGraphSettings::Get()->RecentWireThickness;
	RecordedWireThickness = UFlowGraphSettings::Get()->RecordedWireThickness;
	SelectedWireThickness = UFlowGraphSettings::Get()->SelectedWireThickness;

	// Don't want to draw ending arrowheads
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}

void FFlowGraphConnectionDrawingPolicy::BuildPaths()
{
	if (const UFlowAsset* FlowInstance = CastChecked<UFlowGraph>(GraphObj)->GetFlowAsset()->GetInspectedInstance())
	{
		const double CurrentTime = FApp::GetCurrentTime();

		for (const UFlowNode* Node : FlowInstance->GetRecordedNodes())
		{
			const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node->GetGraphNode());

			for (const TPair<uint8, FPinRecord>& Record : Node->GetWireRecords())
			{
				if (!FlowGraphNode->OutputPins.IsValidIndex(Record.Key))
				{
					UE_LOG(LogFlowEditor, Error, TEXT("Flow node '%s' has an invalid pin connection.  This is probably an flow editor code bug."), *Node->GetName());

					continue;
				}

				if (UEdGraphPin* OutputPin = FlowGraphNode->OutputPins[Record.Key])
				{
					// check if Output pin is connected to anything
					if (OutputPin->LinkedTo.Num() > 0)
					{
						RecordedPaths.Emplace(OutputPin, OutputPin->LinkedTo[0]);

						if (CurrentTime < Record.Value.Time + RecentWireDuration)
						{
							RecentPaths.Emplace(OutputPin, OutputPin->LinkedTo[0]);
						}
					}
				}
			}
		}
	}

	if (GraphObj && (UFlowGraphEditorSettings::Get()->bHighlightInputWiresOfSelectedNodes || UFlowGraphEditorSettings::Get()->bHighlightOutputWiresOfSelectedNodes))
	{
		const TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GraphObj);
		if (FlowGraphEditor.IsValid())
		{
			for (UFlowGraphNode* SelectedNode : FlowGraphEditor->GetSelectedFlowNodes())
			{
				for (UEdGraphPin* Pin : SelectedNode->Pins)
				{
					if ((Pin->Direction == EGPD_Input && UFlowGraphEditorSettings::Get()->bHighlightInputWiresOfSelectedNodes)
						|| (Pin->Direction == EGPD_Output && UFlowGraphEditorSettings::Get()->bHighlightOutputWiresOfSelectedNodes))
					{
						for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
						{
							SelectedPaths.Emplace(Pin, LinkedPin);
						}
					}
				}
			}
		}
	}
}

void FFlowGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	switch (UFlowGraphSettings::Get()->ConnectionDrawType)
	{
		case EFlowConnectionDrawType::Default:
			FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
			break;
		case EFlowConnectionDrawType::Circuit:
			DrawCircuitSpline(LayerId, Start, End, Params);
			break;
		default: ;
	}
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

		if (Cast<UFlowGraphNode>(OutputPin->GetOwningNode())->GetSignalMode() == EFlowSignalMode::Disabled)
		{
			Params.WireColor *= 0.5f;
			Params.WireThickness = 0.5f;
		}
		else if (InputPin && FFlowPin::IsExecPinCategory(InputPin->PinType.PinCategory))
		{
			// selected paths
			if (SelectedPaths.Contains(OutputPin) || SelectedPaths.Contains(InputPin))
			{
				Params.WireColor = SelectedColor;
				Params.WireThickness = SelectedWireThickness;
				Params.bDrawBubbles = false;
			}
			// recent paths
			else if (RecentPaths.Contains(OutputPin) && RecentPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecentColor;
				Params.WireThickness = RecentWireThickness;
				Params.bDrawBubbles = true;
			}
			// all paths, showing graph history
			else if (RecordedPaths.Contains(OutputPin) && RecordedPaths[OutputPin] == InputPin)
			{
				Params.WireColor = RecordedColor;
				Params.WireThickness = RecordedWireThickness;
				Params.bDrawBubbles = false;
			}
			// It's not followed, fade it and keep it thin
			else
			{
				Params.WireColor = InactiveColor;
				Params.WireThickness = InactiveWireThickness;
			}
		}
	}

	// If reroute node path goes backwards, we need to flip the direction to make it look nice
	// (all of the logic for this is basically same as in FKismetConnectionDrawingPolicy)
	{
		UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
		UEdGraphNode* InputNode = (InputPin != nullptr) ? InputPin->GetOwningNode() : nullptr;
		if (auto* OutputRerouteNode = Cast<UFlowGraphNode_Reroute>(OutputNode))
		{
			if (ShouldChangeTangentForReroute(OutputRerouteNode))
			{
				Params.StartDirection = EGPD_Input;
			}
		}

		if (auto* InputRerouteNode = Cast<UFlowGraphNode_Reroute>(InputNode))
		{
			if (ShouldChangeTangentForReroute(InputRerouteNode))
			{
				Params.EndDirection = EGPD_Output;
			}
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;

	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}
}

void FFlowGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	BuildPaths();

	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FFlowGraphConnectionDrawingPolicy::DrawCircuitSpline(const int32& LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) const
{
	const FVector2D StartingPoint = FVector2D(Start.X + UFlowGraphSettings::Get()->CircuitConnectionSpacing.X, Start.Y);
	const FVector2D EndPoint = FVector2D(End.X - UFlowGraphSettings::Get()->CircuitConnectionSpacing.Y, End.Y);
	const FVector2D ControlPoint = GetControlPoint(StartingPoint, EndPoint);

	const FVector2D StartDirection = (Params.StartDirection == EGPD_Output) ? FVector2D(1.0f, 0.0f) : FVector2D(-1.0f, 0.0f);
	const FVector2D EndDirection = (Params.EndDirection == EGPD_Input) ? FVector2D(1.0f, 0.0f) : FVector2D(-1.0f, 0.0f);

	DrawCircuitConnection(LayerId, Start, StartDirection, StartingPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, StartingPoint, StartDirection, ControlPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, ControlPoint, StartDirection, EndPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, EndPoint, StartDirection, End, EndDirection, Params);
}

void FFlowGraphConnectionDrawingPolicy::DrawCircuitConnection(const int32& LayerId, const FVector2D& Start, const FVector2D& StartDirection, const FVector2D& End, const FVector2D& EndDirection, const FConnectionParams& Params) const
{
	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, LayerId, Start, StartDirection, End, EndDirection, Params.WireThickness, ESlateDrawEffect::None, Params.WireColor);

	if (Params.bDrawBubbles)
	{
		// This table maps distance along curve to alpha
		FInterpCurve<float> SplineReparamTable;
		const float SplineLength = MakeSplineReparamTable(Start, StartDirection, End, EndDirection, SplineReparamTable);

		// Draw bubbles on the spline
		if (Params.bDrawBubbles)
		{
			const float BubbleSpacing = 64.f * ZoomFactor;
			const float BubbleSpeed = 192.f * ZoomFactor;
			const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.2f * Params.WireThickness;

			const float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = (static_cast<float>(i) * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
					FVector2D BubblePos = FMath::CubicInterp(Start, StartDirection, End, EndDirection, Alpha);
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(DrawElementsList, LayerId, FPaintGeometry(BubblePos, BubbleSize, ZoomFactor), BubbleImage, ESlateDrawEffect::None, Params.WireColor);
				}
			}
		}
	}
}

FVector2D FFlowGraphConnectionDrawingPolicy::GetControlPoint(const FVector2D& Source, const FVector2D& Target)
{
	const FVector2D Delta = Target - Source;
	const float Tangent = FMath::Tan(UFlowGraphSettings::Get()->CircuitConnectionAngle * (PI / 180.f));

	const float DeltaX = FMath::Abs(Delta.X);
	const float DeltaY = FMath::Abs(Delta.Y);

	const float SlopeWidth = DeltaY / Tangent;
	if (DeltaX > SlopeWidth)
	{
		return Delta.X > 0.f ? FVector2D(Target.X - SlopeWidth, Source.Y) : FVector2D(Source.X - SlopeWidth, Target.Y);
	}

	const float SlopeHeight = DeltaX * Tangent;
	if (DeltaY > SlopeHeight)
	{
		if (Delta.Y > 0.f)
		{
			return Delta.X < 0.f ? FVector2D(Source.X, Target.Y - SlopeHeight) : FVector2D(Target.X, Source.Y + SlopeHeight);
		}

		if (Delta.X < 0.f)
		{
			return FVector2D(Source.X, Target.Y + SlopeHeight);
		}
	}

	return FVector2D(Target.X, Source.Y - SlopeHeight);
}

bool FFlowGraphConnectionDrawingPolicy::ShouldChangeTangentForReroute(UFlowGraphNode_Reroute* Reroute)
{
	if (const bool* pResult = RerouteToReversedDirectionMap.Find(Reroute))
	{
		return *pResult;
	}
	else
	{
		bool bPinReversed = false;

		FVector2D AverageLeftPin;
		FVector2D AverageRightPin;
		FVector2D CenterPin = FVector2D::ZeroVector;
		const bool bCenterValid = Reroute->OutputPins.Num() == 0 ? false : FindPinCenter(Reroute->OutputPins[0], /*out*/ CenterPin);
		const bool bLeftValid = GetAverageConnectedPosition(Reroute, EGPD_Input, /*out*/ AverageLeftPin);
		const bool bRightValid = GetAverageConnectedPosition(Reroute, EGPD_Output, /*out*/ AverageRightPin);

		if (bLeftValid && bRightValid)
		{
			bPinReversed = AverageRightPin.X < AverageLeftPin.X;
		}
		else if (bCenterValid)
		{
			if (bLeftValid)
			{
				bPinReversed = CenterPin.X < AverageLeftPin.X;
			}
			else if (bRightValid)
			{
				bPinReversed = AverageRightPin.X < CenterPin.X;
			}
		}

		RerouteToReversedDirectionMap.Add(Reroute, bPinReversed);

		return bPinReversed;
	}
}

bool FFlowGraphConnectionDrawingPolicy::FindPinCenter(const UEdGraphPin* Pin, FVector2D& OutCenter) const
{
	if (const TSharedPtr<SGraphPin>* PinWidget = PinToPinWidgetMap.Find(Pin))
	{
		if (const FArrangedWidget* PinEntry = PinGeometries->Find((*PinWidget).ToSharedRef()))
		{
			OutCenter = FGeometryHelper::CenterOf(PinEntry->Geometry);
			return true;
		}
	}

	return false;
}

bool FFlowGraphConnectionDrawingPolicy::GetAverageConnectedPosition(UFlowGraphNode_Reroute* Reroute, EEdGraphPinDirection Direction, FVector2D& OutPos) const
{
	FVector2D Result = FVector2D::ZeroVector;
	int32 ResultCount = 0;

	if(Reroute->InputPins.Num() == 0 || Reroute->OutputPins.Num() == 0)
	{
		return false;
	}
	
	UEdGraphPin* Pin = (Direction == EGPD_Input) ? Reroute->InputPins[0] : Reroute->OutputPins[0];
	for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
	{
		FVector2D CenterPoint;
		if (FindPinCenter(LinkedPin, /*out*/ CenterPoint))
		{
			Result += CenterPoint;
			ResultCount++;
		}
	}

	if (ResultCount > 0)
	{
		OutPos = Result * (1.0f / ResultCount);
		return true;
	}
	else
	{
		return false;
	}
}

