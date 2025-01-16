// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowImportUtils.h"

#include "Asset/FlowAssetFactory.h"
#include "FlowEditorDefines.h"
#include "FlowEditorLogChannels.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraph.h"

#include "FlowAsset.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Graph/FlowNode_Start.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraphSchema_K2.h"
#include "EditorAssetLibrary.h"
#include "Misc/ScopedSlowTask.h"

#if ENABLE_ASYNC_NODES_IMPORT
#include "K2Node_BaseAsyncTask.h"
#endif
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Knot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowImportUtils)

#define LOCTEXT_NAMESPACE "FlowImportUtils"

TMap<FName, TSubclassOf<UFlowNode>> UFlowImportUtils::FunctionsToFlowNodes = TMap<FName, TSubclassOf<UFlowNode>>();
TMap<TSubclassOf<UFlowNode>, FBlueprintToFlowPinName> UFlowImportUtils::PinMappings = TMap<TSubclassOf<UFlowNode>, FBlueprintToFlowPinName>();

UFlowAsset* UFlowImportUtils::ImportBlueprintGraph(UObject* BlueprintAsset, const TSubclassOf<UFlowAsset> FlowAssetClass, const FString FlowAssetName,
													const TMap<FName, TSubclassOf<UFlowNode>> InFunctionsToFlowNodes, const TMap<TSubclassOf<UFlowNode>, FBlueprintToFlowPinName> InPinMappings, const FName StartEventName)
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
		FunctionsToFlowNodes = InFunctionsToFlowNodes;
		PinMappings = InPinMappings;

		ImportBlueprintGraph(Blueprint, FlowAsset, StartEventName);
		FunctionsToFlowNodes.Empty();
		PinMappings.Empty();

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

		// non-pure K2Nodes or UK2Node_Knot
		const UK2Node* K2Node = Cast<UK2Node>(ThisNode);
		if (K2Node && (!K2Node->IsNodePure() || Cast<UK2Node_Knot>(K2Node)))
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

	// can't start import if provided graph doesn't have required start node
	if (StartNode == nullptr)
	{
		return;
	}

	// clear existing graph
	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node.Value->GetGraphNode()))
		{
			FlowGraph->GetSchema()->BreakNodeLinks(*FlowGraphNode);
			FlowGraphNode->DestroyNode();
		}

		FlowAsset->UnregisterNode(Node.Key);
	}

	TMap<FGuid, UFlowGraphNode*> TargetNodes;

	// recreated UFlowNode_Start, assign it a blueprint node FGuid
	UFlowGraphNode* StartGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, nullptr, UFlowNode_Start::StaticClass(), FVector2D::ZeroVector);
	FlowGraph->GetSchema()->SetNodeMetaData(StartGraphNode, FNodeMetadata::DefaultGraphNode);
	StartGraphNode->NodeGuid = StartNode->NodeGuid;
	UFlowNode* StartFlowNode = Cast<UFlowNode>(StartGraphNode->GetFlowNodeBase());
	StartFlowNode->SetGuid(StartNode->NodeGuid);
	TargetNodes.Add(StartGraphNode->NodeGuid, StartGraphNode);

	// execute graph import
	// iterate all nodes separately, ensures we import all possible nodes and connect them together
	for (const TPair<FGuid, FImportedGraphNode>& SourceNode : SourceNodes)
	{
		ImportBlueprintFunction(FlowAsset, SourceNode.Value, SourceNodes, TargetNodes);
	}
}

void UFlowImportUtils::ImportBlueprintFunction(const UFlowAsset* FlowAsset, const FImportedGraphNode& NodeImport, const TMap<FGuid, FImportedGraphNode>& SourceNodes, TMap<FGuid, class UFlowGraphNode*>& TargetNodes)
{
	ensureAlways(NodeImport.SourceGraphNode);
	TSubclassOf<UFlowNode> MatchingFlowNodeClass = nullptr;

	// find FlowNode class matching provided UFunction name
	FName FunctionName = NAME_None;
	if (const UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(NodeImport.SourceGraphNode))
	{
		FunctionName = FunctionNode->GetFunctionName();
	}
#if ENABLE_ASYNC_NODES_IMPORT
	else if (const UK2Node_BaseAsyncTask* AsyncTaskNode = Cast<UK2Node_BaseAsyncTask>(NodeImport.SourceGraphNode))
	{
		FunctionName = AsyncTaskNode->GetProxyFactoryFunctionName();
	}
#endif
	else if (Cast<UK2Node_Knot>(NodeImport.SourceGraphNode))
	{
		FunctionName = TEXT("Reroute");
	}
	else if (Cast<UK2Node_IfThenElse>(NodeImport.SourceGraphNode))
	{
		FunctionName = TEXT("Branch");
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
		TMap<const FName, const UEdGraphPin*> InputPins;
		GetValidInputPins(NodeImport.SourceGraphNode, InputPins);

		UClass* FlowNodeClass = FlowGraphNode->GetFlowNodeBase()->GetClass();
		for (TFieldIterator<FProperty> PropIt(FlowNodeClass, EFieldIteratorFlags::IncludeSuper); PropIt && (PropIt->PropertyFlags & CPF_Edit); ++PropIt)
		{
			const FProperty* Param = *PropIt;
			const bool bIsEditable = !Param->HasAnyPropertyFlags(CPF_Deprecated);
			if (bIsEditable)
			{
				if (const UEdGraphPin* MatchingInputPin = FindPinMatchingToProperty(FlowNodeClass, Param, InputPins))
				{
					if (MatchingInputPin->LinkedTo.Num() == 0) // nothing connected to pin, so user can set value directly on this pin
					{
						FString const PinValue = MatchingInputPin->GetDefaultAsString();
						uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(FlowGraphNode->GetFlowNodeBase());
						Param->ImportText_Direct(*PinValue, Offset, FlowGraphNode->GetFlowNodeBase(), PPF_Copy, GLog);
					}
				}
				else // try to find matching Pin in connected pure nodes
				{
					bool bPinFound = false;
					for (const TPair<const FName, const UEdGraphPin*> InputPin : InputPins)
					{
						for (const UEdGraphPin* LinkedPin : InputPin.Value->LinkedTo)
						{
							if (LinkedPin && LinkedPin->GetOwningNode()) // try to read value from the first pure node connected to the pin
							{
								// in theory, we could put this part in recursive loop, iterating pure nodes until we find one with matching Pin Name
								// in practice, iterating blueprint graph isn't that easy as might encounter Make/Break nodes, array builders
								// if someone is willing put work to it, you're welcome to make a pull request

								UK2Node* LinkedK2Node = Cast<UK2Node>(LinkedPin->GetOwningNode());
								if (LinkedK2Node && LinkedK2Node->IsNodePure())
								{
									TMap<const FName, const UEdGraphPin*> PureNodePins;
									GetValidInputPins(LinkedK2Node, PureNodePins);

									if (const UEdGraphPin* PureInputPin = FindPinMatchingToProperty(FlowNodeClass, Param, PureNodePins))
									{
										if (PureInputPin->LinkedTo.Num() == 0) // nothing connected to pin, so user can set value directly on this pin
										{
											FString const PinValue = PureInputPin->GetDefaultAsString();
											uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(FlowGraphNode->GetFlowNodeBase());
											Param->ImportText_Direct(*PinValue, Offset, FlowGraphNode->GetFlowNodeBase(), PPF_Copy, GLog);

											bPinFound = true;
										}
									}
								}

								// there can be only single valid connection on input parameter pin
								break;
							}
						}

						if (bPinFound)
						{
							break;
						}
					}
				}
			}
		}
	}

	// Flow Nodes with Context Pins needs to update related data and call OnReconstructionRequested.ExecuteIfBound() in order to fully construct a graph node
	FlowGraphNode->GetFlowNodeBase()->PostImport();

	// connect new node to all already recreated nodes
	for (const TPair<FName, FConnectedPin>& Connection : NodeImport.Incoming)
	{
		UEdGraphPin* ThisPin = nullptr;
		for (UEdGraphPin* FlowInputPin : FlowGraphNode->InputPins)
		{
			if (FlowGraphNode->InputPins.Num() == 1 || Connection.Key == FlowInputPin->PinName)
			{
				ThisPin = FlowInputPin;
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
			for (UEdGraphPin* FlowOutputPin : ConnectedNode->OutputPins)
			{
				if (ConnectedNode->OutputPins.Num() == 1 || Connection.Value.PinName == FlowOutputPin->PinName
					|| (Connection.Value.PinName == UEdGraphSchema_K2::PN_Then && FlowOutputPin->PinName == FName("TRUE"))
					|| (Connection.Value.PinName == UEdGraphSchema_K2::PN_Else && FlowOutputPin->PinName == FName("FALSE")))
				{
					ConnectedPin = FlowOutputPin;
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
		for (UEdGraphPin* FlowOutputPin : FlowGraphNode->OutputPins)
		{
			if (FlowGraphNode->OutputPins.Num() == 1 || Connection.Key == FlowOutputPin->PinName
				|| (Connection.Key == UEdGraphSchema_K2::PN_Then && FlowOutputPin->PinName == FName("TRUE"))
				|| (Connection.Key == UEdGraphSchema_K2::PN_Else && FlowOutputPin->PinName == FName("FALSE")))
			{
				ThisPin = FlowOutputPin;
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
			for (UEdGraphPin* FlowInputPin : ConnectedNode->InputPins)
			{
				if (ConnectedNode->InputPins.Num() == 1 || Connection.Value.PinName == FlowInputPin->PinName)
				{
					ConnectedPin = FlowInputPin;
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

void UFlowImportUtils::GetValidInputPins(const UEdGraphNode* GraphNode, TMap<const FName, const UEdGraphPin*>& Result)
{
	for (const UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && !Pin->bHidden && !Pin->bOrphanedPin)
		{
			Result.Add(Pin->PinName, Pin);
		}
	}
}

const UEdGraphPin* UFlowImportUtils::FindPinMatchingToProperty(UClass* FlowNodeClass, const FProperty* Property, const TMap<const FName, const UEdGraphPin*> Pins)
{
	const FName& PropertyAuthoredName = *Property->GetAuthoredName();

	// if Pin Name is exactly the same as Flow Node property name
	if (const UEdGraphPin* Pin = Pins.FindRef(PropertyAuthoredName))
	{
		return Pin;
	}

	// if not, check if appropriate Pin Mapping has been provided
	if (const FBlueprintToFlowPinName* PinMapping = PinMappings.Find(FlowNodeClass))
	{
		if (const FName* MappedPinName = PinMapping->NodePropertiesToFunctionPins.Find(PropertyAuthoredName))
		{
			if (const UEdGraphPin* Pin = Pins.FindRef(*MappedPinName))
			{
				return Pin;
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
