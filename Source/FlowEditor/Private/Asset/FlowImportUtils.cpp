// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowImportUtils.h"

#include "Asset/FlowAssetFactory.h"
#include "FlowEditorModule.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraph.h"

#include "FlowAsset.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Route/FlowNode_Start.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraphNode_Comment.h"
#include "EditorAssetLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "FlowImportUtils"

TMap<FName, TSubclassOf<UFlowNode>> UFlowImportUtils::FunctionsToFlowNodes = TMap<FName, TSubclassOf<UFlowNode>>();

UFlowAsset* UFlowImportUtils::ImportBlueprintGraph(UObject* BlueprintAsset, TSubclassOf<UFlowAsset> FlowAssetClass, FString FlowAssetName, TMap<FName, TSubclassOf<UFlowNode>> BlueprintFunctionsToFlowNodes, const FName StartEventName)
{
	if (BlueprintAsset == nullptr || FlowAssetClass == nullptr || FlowAssetName.IsEmpty() || StartEventName.IsNone())
	{
		return nullptr;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintAsset);
	UFlowAsset* FlowAsset = nullptr;

	// we assume that users want to have a converted asset in the same folder as the legacy blueprint
	const FString PackageFolder = FPaths::GetPath(Blueprint->GetOuter()->GetPathName());

	if (!FPackageName::DoesPackageExist(PackageFolder / FlowAssetName, nullptr)) // create a new asset
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UFactory* Factory = Cast<UFactory>(UFlowAssetFactory::StaticClass()->GetDefaultObject());

		if (UObject* NewAsset = AssetTools.CreateAsset(FlowAssetName, PackageFolder, FlowAssetClass, Factory))
		{
			FlowAsset = Cast<UFlowAsset>(NewAsset);
		}
	}
	else // load existing asset
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

		const FString PackageName = PackageFolder / (FlowAssetName + TEXT(".") + FlowAssetName);
		const FAssetData& FoundAssetData = AssetRegistryModule.GetRegistry().GetAssetByObjectPath(FSoftObjectPath(PackageName));

		FlowAsset = Cast<UFlowAsset>(FoundAssetData.GetAsset());
	}

	// import graph
	if (FlowAsset)
	{
		FunctionsToFlowNodes = BlueprintFunctionsToFlowNodes;
		ImportBlueprintGraph(Blueprint, FlowAsset, StartEventName);
		FunctionsToFlowNodes.Empty();

		Cast<UFlowGraph>(FlowAsset->GetGraph())->RefreshGraph();
		UEditorAssetLibrary::SaveLoadedAsset(FlowAsset->GetPackage());
	}

	return FlowAsset;
}

void UFlowImportUtils::ImportBlueprintGraph(UBlueprint* Blueprint, UFlowAsset* FlowAsset, const FName StartEventName)
{
	ensureAlways(Blueprint && FlowAsset);

	UEdGraph* BlueprintGraph = Blueprint->UbergraphPages.IsValidIndex(0) ? Blueprint->UbergraphPages[0] : nullptr;
	if (BlueprintGraph == nullptr)
	{
		return;
	}

	FScopedSlowTask ExecuteAssetTask(BlueprintGraph->Nodes.Num(), FText::Format(LOCTEXT("FFlowGraphUtils::ImportBlueprintGraph", "Reading {0}"), FText::FromString(Blueprint->GetFriendlyName())));
	ExecuteAssetTask.MakeDialog();

	TMap<FGuid, FImportedGraphNode> SourceNodes;
	UEdGraphNode* StartNode = nullptr;

	for (UEdGraphNode* ThisNode : BlueprintGraph->Nodes)
	{
		ExecuteAssetTask.EnterProgressFrame(1, FText::Format(LOCTEXT("FFlowGraphUtils::ImportBlueprintGraph", "Processing blueprint node: {0}"), ThisNode->GetNodeTitle(ENodeTitleType::ListView)));

		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(ThisNode))
		{
			// special case: recreate Comment node
			FFlowGraphSchemaAction_NewComment CommentAction;
			UEdGraphNode* NewNode = CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, FVector2D(CommentNode->NodePosX, CommentNode->NodePosY), false);
			if (UEdGraphNode_Comment* CommentCopy = Cast<UEdGraphNode_Comment>(NewNode))
			{
				CommentCopy->NodeComment = CommentNode->NodeComment;

				CommentCopy->CommentColor = CommentNode->CommentColor;
				CommentCopy->FontSize = CommentNode->FontSize;
				CommentCopy->bCommentBubbleVisible_InDetailsPanel = CommentNode->bCommentBubbleVisible_InDetailsPanel;
				CommentCopy->bColorCommentBubble = CommentNode->bColorCommentBubble;
				CommentCopy->MoveMode = CommentNode->MoveMode;
			}
		}
		else // non-pure K2Nodes
		{
			const UK2Node* K2Node = Cast<UK2Node>(ThisNode);
			if (K2Node && !K2Node->IsNodePure())
			{
				FImportedGraphNode& NodeImport = SourceNodes.FindOrAdd(ThisNode->NodeGuid);
				NodeImport.SourceGraphNode = ThisNode;

				// create map of all non-pure blueprint nodes with theirs pin connections
				for (const UEdGraphPin* ThisPin : ThisNode->Pins)
				{
					for (const UEdGraphPin* LinkedPin : ThisPin->LinkedTo)
					{
						if (LinkedPin && LinkedPin->GetOwningNode())
						{
							const FConnectedPin ConnectedPin(LinkedPin->GetOwningNode()->NodeGuid, LinkedPin->PinName);

							if (ThisPin->Direction == EGPD_Input)
							{
								NodeImport.Incoming.Add(ThisPin->PinName, ConnectedPin);
							}
							else
							{
								NodeImport.Outgoing.Add(ThisPin->PinName, ConnectedPin);
							}
						}
					}
				}

				// we need to know the default entry point of blueprint graph
				const UK2Node_Event* EventNode = Cast<UK2Node_Event>(ThisNode);
				if (EventNode && (EventNode->EventReference.GetMemberName() == StartEventName || EventNode->CustomFunctionName == StartEventName))
				{
					StartNode = ThisNode;
				}
			}
		}
	}

	// can't start import if provided graph doesn't have required start node
	if (StartNode == nullptr)
	{
		return;
	}

	// clear existing graph
	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	FlowGraph->Nodes.Empty();

	TMap<FGuid, UFlowGraphNode*> TargetNodes;
	
	// recreated UFlowNode_Start, assign it a blueprint node FGuid
	UFlowGraphNode* StartGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, nullptr, UFlowNode_Start::StaticClass(), FVector2D::ZeroVector);
	FlowGraph->GetSchema()->SetNodeMetaData(StartGraphNode, FNodeMetadata::DefaultGraphNode);
	StartGraphNode->NodeGuid = StartNode->NodeGuid;
	StartGraphNode->GetFlowNode()->SetGuid(StartNode->NodeGuid);
	TargetNodes.Add(StartGraphNode->NodeGuid, StartGraphNode);

	// execute graph import
	// iterate all nodes separately, ensures we import all possible nodes and connect them together
	for (const TPair<FGuid, FImportedGraphNode>& SourceNode : SourceNodes)
	{
		ImportBlueprintFunction(FlowAsset, SourceNode.Value, SourceNodes, TargetNodes);
	}
}

void UFlowImportUtils::ImportBlueprintFunction(UFlowAsset* FlowAsset, const FImportedGraphNode& NodeImport, const TMap<FGuid, FImportedGraphNode>& SourceNodes, TMap<FGuid, UFlowGraphNode*>& TargetNodes)
{
	ensureAlways(NodeImport.SourceGraphNode);
	TSubclassOf<UFlowNode> MatchingFlowNodeClass = nullptr;

	// find FlowNode class matching provided UFunction name
	FName FunctionName = NAME_None;
	if (const UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(NodeImport.SourceGraphNode))
	{
		FunctionName = FunctionNode->GetFunctionName();
	}

	if (!FunctionName.IsNone())
	{
		// find FlowNode class matching provided UFunction name
		MatchingFlowNodeClass = FunctionsToFlowNodes.FindRef(FunctionName);
	}
	
	if (MatchingFlowNodeClass == nullptr)
	{
		UE_LOG(LogFlowEditor, Error, TEXT("Can't find Flow Node class for K2Node, function name %s"), *FunctionName.ToString());
		return;
	}

	const FGuid& NodeGuid = NodeImport.SourceGraphNode->NodeGuid;

	// create a new Flow Graph node
	const FVector2d Location = FVector2D(NodeImport.SourceGraphNode->NodePosX, NodeImport.SourceGraphNode->NodePosY);
	UFlowGraphNode* FlowGraphNode = FFlowGraphSchemaAction_NewNode::ImportNode(FlowAsset->GetGraph(), nullptr, MatchingFlowNodeClass, NodeGuid, Location);

	if (FlowGraphNode == nullptr)
	{
		return;
	}
	TargetNodes.Add(NodeGuid, FlowGraphNode);

	// transfer properties from UFunction input parameters to Flow Node properties
	{
		TMap<FName, UEdGraphPin*> InputPins;
		for (UEdGraphPin* Pin : NodeImport.SourceGraphNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && !Pin->bHidden && !Pin->bOrphanedPin)
			{
				InputPins.Add(Pin->PinName, Pin);
			}
		}

		for (TFieldIterator<FProperty> PropIt(FlowGraphNode->GetFlowNode()->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt && (PropIt->PropertyFlags & CPF_Edit); ++PropIt)
		{
			const FProperty* Param = *PropIt;
			const bool bIsEditable = !Param->HasAnyPropertyFlags(CPF_Deprecated);
			if (bIsEditable)
			{
				if (const UEdGraphPin* InputPin = InputPins.FindRef(*Param->GetAuthoredName()))
				{
					FString const PinValue = InputPin->GetDefaultAsString();
					uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(FlowGraphNode->GetFlowNode());
					Param->ImportText_Direct(*PinValue, Offset, FlowGraphNode->GetFlowNode(), PPF_Copy, GLog);
				}
			}
		}
	}

	// Flow Nodes with Context Pins needs to update related data and call OnReconstructionRequested.ExecuteIfBound() in order to fully construct a graph node
	FlowGraphNode->GetFlowNode()->PostImport();

	// connect new node to all already recreated nodes
	for (const TPair<FName, FConnectedPin>& Connection : NodeImport.Incoming)
	{
		UEdGraphPin* ThisPin = nullptr;
		for (UEdGraphPin* Pin : FlowGraphNode->InputPins)
		{
			if (Pin->PinName == Connection.Key || FlowGraphNode->InputPins.Num() == 1)
			{
				ThisPin = Pin;
				break;
			}
		}
		if (ThisPin == nullptr)
		{
			continue;
		}
		
		UEdGraphPin* ConnectedPin = nullptr;
		if (UFlowGraphNode* ConnectedNode = TargetNodes.FindRef(Connection.Value.NodeGuid))
		{
			for (UEdGraphPin* Pin : ConnectedNode->OutputPins)
			{
				if (ConnectedNode->OutputPins.Num() == 1 || Pin->PinName == Connection.Value.PinName)
				{
					ConnectedPin = Pin;
					break;
				}
			}
		}

		// link the pin to existing node
		if (ConnectedPin)
		{
			FlowAsset->GetGraph()->GetSchema()->TryCreateConnection(ThisPin, ConnectedPin);
		}
	}
	for (const TPair<FName, FConnectedPin>& Connection : NodeImport.Outgoing)
	{
		UEdGraphPin* ThisPin = nullptr;
		for (UEdGraphPin* Pin : FlowGraphNode->OutputPins)
		{
			if (Pin->PinName == Connection.Key || FlowGraphNode->OutputPins.Num() == 1)
			{
				ThisPin = Pin;
				break;
			}
		}
		if (ThisPin == nullptr)
		{
			continue;
		}
		
		UEdGraphPin* ConnectedPin = nullptr;
		if (UFlowGraphNode* ConnectedNode = TargetNodes.FindRef(Connection.Value.NodeGuid))
		{
			for (UEdGraphPin* Pin : ConnectedNode->InputPins)
			{
				if (ConnectedNode->InputPins.Num() == 1 || Pin->PinName == Connection.Value.PinName)
				{
					ConnectedPin = Pin;
					break;
				}
			}
		}

		// link the pin to existing node
		if (ConnectedPin)
		{
			FlowAsset->GetGraph()->GetSchema()->TryCreateConnection(ThisPin, ConnectedPin);
		}
	}
}

#undef LOCTEXT_NAMESPACE
