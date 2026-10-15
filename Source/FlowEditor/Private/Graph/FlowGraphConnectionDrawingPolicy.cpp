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

#include "Layout/ArrangedChildren.h"
#include "Misc/App.h"
#include "SGraphNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphConnectionDrawingPolicy)

namespace FlowGraphConnectionDrawing
{
	/* A connection is only re-routed below its nodes once the input pin sits at least this far to the left of
	 * the output pin, so near-vertical or slightly-backwards connections still use the default spline. */
	constexpr float BackwardsThresholdX = 1.0f;

	/* Vertical gap left between the endpoint nodes and the straight run of a re-routed backwards connection,
	 * scaled by zoom. */
	constexpr float RouteClearance = 24.0f;

	/* How much further the route is pushed out (down when routing below the nodes, up when routing above) for
	 * every pixel the output pin sits toward the node edge the route runs alongside - the bottom edge when
	 * routing below (bCurveUpward false), the top edge when routing above (bCurveUpward true). Multiple backward
	 * connections leaving different transitions on the same node would otherwise all overlap on the straight run. */
	constexpr float FanOutFactor = 0.5f;

	/* Bounds on the tangent magnitude used at the straight run's own ends, scaled by zoom. Kept small and capped
	 * to the horizontal run distance (see FFlowGraphConnectionDrawingPolicy::DrawDefaultConnection) because both
	 * of the straight run's tangents point the same way, so an oversized value here can make the run's
	 * parametrization double back on itself. */
	constexpr float MinStraightTangentSize = 16.0f;
	constexpr float MaxStraightTangentSize = 48.0f;

	/* Bounds on how far the drop/rise elbow visually bows away from its pin's column before curving into the
	 * straight run, scaled by zoom. A vertical run's Y is linear regardless of tangent magnitude (only its X bow
	 * changes), so there is no self-crossing risk here and this can be sized purely for visibility. */
	constexpr float MinElbowOutset = 24.0f;
	constexpr float MaxElbowOutset = 64.0f;

	/* A cubic Hermite's peak lateral excursion on a vertical run is only 1/4 of its tangent magnitude (see the
	 * derivation in FFlowGraphConnectionDrawingPolicy::DrawDefaultConnection), so the tangent used for the
	 * elbows is scaled up by this factor to make the bow actually reach ElbowOutset, instead of tucking in under
	 * the node and getting hidden behind it. */
	constexpr float ElbowExcursionCompensation = 4.0f;
}

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
	const UFlowGraphSettings* GraphSettings = GetDefault<UFlowGraphSettings>();
	
	// Cache off the editor options
	RecentWireDuration = GraphSettings->RecentWireDuration;

	InactiveColor = GraphSettings->InactiveWireColor;
	RecentColor = GraphSettings->RecentWireColor;
	RecordedColor = GraphSettings->RecordedWireColor;
	SelectedColor = GraphSettings->SelectedWireColor;

	InactiveWireThickness = GraphSettings->InactiveWireThickness;
	RecentWireThickness = GraphSettings->RecentWireThickness;
	RecordedWireThickness = GraphSettings->RecordedWireThickness;
	SelectedWireThickness = GraphSettings->SelectedWireThickness;

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

	const UFlowGraphEditorSettings* GraphEditorSettings = GetDefault<UFlowGraphEditorSettings>();
	if (GraphObj && (GraphEditorSettings->bHighlightInputWiresOfSelectedNodes || GraphEditorSettings->bHighlightOutputWiresOfSelectedNodes))
	{
		const TSharedPtr<SFlowGraphEditor> FlowGraphEditor = FFlowGraphUtils::GetFlowGraphEditor(GraphObj);
		if (FlowGraphEditor.IsValid())
		{
			for (UFlowGraphNode* SelectedNode : FlowGraphEditor->GetSelectedFlowNodes())
			{
				for (UEdGraphPin* Pin : SelectedNode->Pins)
				{
					if ((Pin->Direction == EGPD_Input && GraphEditorSettings->bHighlightInputWiresOfSelectedNodes)
						|| (Pin->Direction == EGPD_Output && GraphEditorSettings->bHighlightOutputWiresOfSelectedNodes))
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

void FFlowGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	switch (GetDefault<UFlowGraphSettings>()->ConnectionDrawType)
	{
		case EFlowConnectionDrawType::Default:
			DrawDefaultConnection(LayerId, Start, End, Params);
			break;
		case EFlowConnectionDrawType::Circuit:
			DrawCircuitSpline(LayerId, Start, End, Params);	
			break;
		default: ;
	}
}

void FFlowGraphConnectionDrawingPolicy::DrawDefaultConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	using namespace FlowGraphConnectionDrawing;

	const bool bIsBackwards = End.X < Start.X - BackwardsThresholdX;

	const bool bTouchesReroute =
		(Params.AssociatedPin1 != nullptr && IsValid(Cast<UFlowGraphNode_Reroute>(Params.AssociatedPin1->GetOwningNode()))) ||
		(Params.AssociatedPin2 != nullptr && IsValid(Cast<UFlowGraphNode_Reroute>(Params.AssociatedPin2->GetOwningNode())));

	FNodeVerticalExtent OutputNodeExtent;
	FNodeVerticalExtent InputNodeExtent;

	if (!bIsBackwards || bTouchesReroute
		|| !TryGetOwningNodeVerticalExtent(Params.AssociatedPin1, OutputNodeExtent)
		|| !TryGetOwningNodeVerticalExtent(Params.AssociatedPin2, InputNodeExtent))
	{
		FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
		return;
	}

	// Decided by node position rather than the specific pins being connected, so every backward connection
	// between the same pair of nodes curves the same way regardless of which pins on them it links. If the
	// output node sits lower than the input node, routing below both (as usual) would send the wire the long way
	// around; route above them instead. Ties (equal centers) fall back to curving downward.
	const bool bCurveUpward = OutputNodeExtent.Top > InputNodeExtent.Top;

	// Fan out multiple backward connections leaving different transitions on the same node: the closer the
	// output pin sits to the node edge the route runs alongside, the further out the route is pushed, so their
	// straight runs land at different heights instead of overlapping. That edge is the bottom when curving
	// downward and the top when curving upward, so which edge the depth is measured from has to flip with bCurveUpward.
	const float OutputPinDepth = bCurveUpward
		? FMath::Max(OutputNodeExtent.Bottom - Start.Y, 0.0f)
		: FMath::Max(Start.Y - OutputNodeExtent.Top, 0.0f);
	const float FanOutOffset = OutputPinDepth * FanOutFactor;

	float RouteY;
	float RouteVerticalTravel;

	if (bCurveUpward)
	{
		RouteY = FMath::Min(OutputNodeExtent.Top, InputNodeExtent.Top) - RouteClearance * ZoomFactor - FanOutOffset;
		RouteVerticalTravel = FMath::Max(FMath::Max(Start.Y, End.Y) - RouteY, 0.0f);
	}
	else
	{
		RouteY = FMath::Max(OutputNodeExtent.Bottom, InputNodeExtent.Bottom) + RouteClearance * ZoomFactor + FanOutOffset;
		RouteVerticalTravel = FMath::Max(RouteY - FMath::Min(Start.Y, End.Y), 0.0f);
	}

	const float HorizontalDistance = FMath::Abs(Start.X - End.X);

	// Tangent magnitude for the straight run's own ends: capped by the horizontal distance between the pins,
	// since both of its tangents point the same way (see StraightParams below) and an oversized value here can
	// make the run's parametrization double back on itself over a short distance.
	const float StraightTangentSize = FMath::Max(FMath::Min(FMath::Clamp(RouteVerticalTravel, MinStraightTangentSize * ZoomFactor, MaxStraightTangentSize * ZoomFactor), HorizontalDistance), 1.0f);

	// Tangent magnitude for the two elbows. See ElbowExcursionCompensation above for why this needs to be several
	// times larger than the visual bow it produces. A vertical run's Y is linear regardless of tangent magnitude
	// (see the derivation there too), so this works the same whether the elbow curves down out of the output pin
	// (bCurveUpward false) or up out of it (bCurveUpward true).
	const float ElbowTangentSize = FMath::Clamp(RouteVerticalTravel, MinElbowOutset * ZoomFactor, MaxElbowOutset * ZoomFactor) * ElbowExcursionCompensation;

	const FVector2f OutputElbowPoint(Start.X, RouteY);
	const FVector2f InputElbowPoint(End.X, RouteY);

	const FVector2f ElbowRightward(ElbowTangentSize, 0.0f);
	const FVector2f ElbowLeftward(-ElbowTangentSize, 0.0f);
	const FVector2f StraightLeftward(-StraightTangentSize, 0.0f);

	// Only the straight run in the middle draws bubbles, so a backwards connection does not restart its bubble
	// animation three times per wire.
	FConnectionParams OutputElbowParams = Params;
	OutputElbowParams.bDrawBubbles = false;
	OutputElbowParams.StartTangent = ElbowRightward;
	OutputElbowParams.EndTangent = ElbowLeftward;

	FConnectionParams StraightParams = Params;
	StraightParams.StartTangent = StraightLeftward;
	StraightParams.EndTangent = StraightLeftward;

	FConnectionParams InputElbowParams = Params;
	InputElbowParams.bDrawBubbles = false;
	InputElbowParams.StartTangent = ElbowLeftward;
	InputElbowParams.EndTangent = ElbowRightward;

	// Call the base FConnectionDrawingPolicy explicitly for each segment, deliberately bypassing both this
	// class's own Default/Circuit switch and its own backwards-routing (segments are not themselves backwards),
	// so relinking, slice-line cutting and hover deemphasis keep working per segment.
	FConnectionDrawingPolicy::DrawConnection(LayerId, Start, OutputElbowPoint, OutputElbowParams);
	FConnectionDrawingPolicy::DrawConnection(LayerId, OutputElbowPoint, InputElbowPoint, StraightParams);
	FConnectionDrawingPolicy::DrawConnection(LayerId, InputElbowPoint, End, InputElbowParams);
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

	// Cache node geometry for this paint so DrawDefaultConnection() can look up how far down a backwards
	// connection's endpoint nodes extend. Every paint gets a fresh policy instance (see
	// FFlowGraphConnectionDrawingPolicyFactory::CreateConnectionPolicy), so this cannot go stale between paints.
	NodeToArrangedIndexMap.Reset();
	NodeToArrangedIndexMap.Reserve(ArrangedNodes.Num());

	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		const TSharedRef<SGraphNode> GraphNodeWidget = StaticCastSharedRef<SGraphNode>(ArrangedNodes[NodeIndex].Widget);
		if (const UEdGraphNode* NodeObj = GraphNodeWidget->GetNodeObj())
		{
			NodeToArrangedIndexMap.Add(NodeObj, NodeIndex);
		}
	}

	CurrentArrangedNodes = &ArrangedNodes;
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
	CurrentArrangedNodes = nullptr;
}

bool FFlowGraphConnectionDrawingPolicy::TryGetOwningNodeVerticalExtent(const UEdGraphPin* Pin, FNodeVerticalExtent& OutExtent) const
{
	if (Pin == nullptr || CurrentArrangedNodes == nullptr)
	{
		return false;
	}

	const int32* NodeIndex = NodeToArrangedIndexMap.Find(Pin->GetOwningNode());
	if (NodeIndex == nullptr || !CurrentArrangedNodes->IsValidIndex(*NodeIndex))
	{
		return false;
	}

	const FArrangedWidget& ArrangedNode = (*CurrentArrangedNodes)[*NodeIndex];

	// Nodes culled off-screen are arranged with zero-size synthesized geometry (see SGraphPanel::OnPaint), which
	// would otherwise collapse the node's bottom edge to its top and make a route jump as the far node scrolls
	// out of view. Fall back to the widget's (zoom-scaled) desired size in that case; the top edge's position is
	// unaffected by this, since only the synthesized size (not position) is wrong for a culled node.
	const float DrawHeight = ArrangedNode.Geometry.GetDrawSize().Y;
	const float DesiredHeight = ArrangedNode.Widget->GetDesiredSize().Y * ZoomFactor;
	const float NodeHeight = FMath::Max(DrawHeight, DesiredHeight);

	OutExtent.Top = ArrangedNode.Geometry.AbsolutePosition.Y;
	OutExtent.Bottom = OutExtent.Top + NodeHeight;

	return true;
}

void FFlowGraphConnectionDrawingPolicy::DrawCircuitSpline(const int32& LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params) const
{
	const FVector2f StartingPoint = FVector2f(Start.X + GetDefault<UFlowGraphSettings>()->CircuitConnectionSpacing.X, Start.Y);
	const FVector2f EndPoint = FVector2f(End.X - GetDefault<UFlowGraphSettings>()->CircuitConnectionSpacing.Y, End.Y);
	const FVector2f ControlPoint = GetControlPoint(StartingPoint, EndPoint);

	const FVector2f StartDirection = (Params.StartDirection == EGPD_Output) ? FVector2f(1.0f, 0.0f) : FVector2f(-1.0f, 0.0f);
	const FVector2f EndDirection = (Params.EndDirection == EGPD_Input) ? FVector2f(1.0f, 0.0f) : FVector2f(-1.0f, 0.0f);

	DrawCircuitConnection(LayerId, Start, StartDirection, StartingPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, StartingPoint, StartDirection, ControlPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, ControlPoint, StartDirection, EndPoint, EndDirection, Params);
	DrawCircuitConnection(LayerId, EndPoint, StartDirection, End, EndDirection, Params);
}

void FFlowGraphConnectionDrawingPolicy::DrawCircuitConnection(const int32& LayerId, const FVector2f& Start, const FVector2f& StartDirection, const FVector2f& End, const FVector2f& EndDirection, const FConnectionParams& Params) const
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
			const FVector2f BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.2f * Params.WireThickness;

			const float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = (static_cast<float>(i) * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
					FVector2f BubblePos = FMath::CubicInterp(Start, StartDirection, End, EndDirection, Alpha);
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(DrawElementsList, LayerId, FPaintGeometry(BubblePos, BubbleSize, ZoomFactor), BubbleImage, ESlateDrawEffect::None, Params.WireColor);
				}
			}
		}
	}
}

FVector2f FFlowGraphConnectionDrawingPolicy::GetControlPoint(const FVector2f& Source, const FVector2f& Target)
{
	const FVector2f Delta = Target - Source;
	const float Tangent = FMath::Tan(GetDefault<UFlowGraphSettings>()->CircuitConnectionAngle * (PI / 180.f));

	const float DeltaX = FMath::Abs(Delta.X);
	const float DeltaY = FMath::Abs(Delta.Y);

	const float SlopeWidth = DeltaY / Tangent;
	if (DeltaX > SlopeWidth)
	{
		return Delta.X > 0.f ? FVector2f(Target.X - SlopeWidth, Source.Y) : FVector2f(Source.X - SlopeWidth, Target.Y);
	}

	const float SlopeHeight = DeltaX * Tangent;
	if (DeltaY > SlopeHeight)
	{
		if (Delta.Y > 0.f)
		{
			return Delta.X < 0.f ? FVector2f(Source.X, Target.Y - SlopeHeight) : FVector2f(Target.X, Source.Y + SlopeHeight);
		}

		if (Delta.X < 0.f)
		{
			return FVector2f(Source.X, Target.Y + SlopeHeight);
		}
	}

	return FVector2f(Target.X, Source.Y - SlopeHeight);
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

