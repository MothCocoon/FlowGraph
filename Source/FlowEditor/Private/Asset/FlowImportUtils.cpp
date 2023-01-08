// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowImportUtils.h"
#include "Asset/FlowAssetFactory.h"
#include "Graph/FlowGraphSchema_Actions.h"

#include "FlowAsset.h"
#include "FlowSettings.h"
#include "Nodes/FlowPin.h"
#include "Nodes/Route/FlowNode_Start.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "Misc/ScopedSlowTask.h"
#include "PropertyPathHelpers.h"

#define LOCTEXT_NAMESPACE "FlowImportUtils"

UFlowAsset* UFlowImportUtils::ImportBlueprintGraph(UObject* BlueprintAsset, TSubclassOf<UFlowAsset> FlowAssetClass, FString FlowAssetName, const FName StartEventName)
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
			UEditorAssetLibrary::SaveLoadedAsset(NewAsset->GetPackage());
			FlowAsset = Cast<UFlowAsset>(NewAsset);
		}
	}
	else // load existing asset
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

		const FString PackageName = PackageFolder / (FlowAssetName + TEXT(".") + FlowAssetName);
		const FAssetData& FoundAssetData = AssetRegistryModule.GetRegistry().GetAssetByObjectPath(*PackageName);

		FlowAsset = Cast<UFlowAsset>(FoundAssetData.GetAsset());
	}

	// import graph
	if (FlowAsset)
	{
		ImportBlueprintGraph(Blueprint, FlowAsset, StartEventName);
	}

	return FlowAsset;
}

void UFlowImportUtils::ImportBlueprintGraph(UBlueprint* Blueprint, const UFlowAsset* FlowAsset, const FName StartEventName)
{
	ensureAlways(Blueprint && FlowAsset);

	UEdGraph* BlueprintGraph = Blueprint->UbergraphPages.IsValidIndex(0) ? Blueprint->UbergraphPages[0] : nullptr;
	if (BlueprintGraph == nullptr)
	{
		return;
	}

	FScopedSlowTask ExecuteAssetTask(BlueprintGraph->Nodes.Num(), FText::Format(LOCTEXT("FFlowGraphUtils::ImportBlueprintGraph", "Reading {0}"), FText::FromString(Blueprint->GetFriendlyName())));
	ExecuteAssetTask.MakeDialog();

	TMap<FGuid, FGraphNodeImport> ImportedNodes;
	UEdGraphNode* StartNode = nullptr;

	for (UEdGraphNode* ThisNode : BlueprintGraph->Nodes)
	{
		ExecuteAssetTask.EnterProgressFrame(1, FText::Format(LOCTEXT("FFlowGraphUtils::ImportBlueprintGraph", "Processing blueprint node: {0}"), ThisNode->GetNodeTitle(ENodeTitleType::ListView)));

		const UK2Node* K2Node = Cast<UK2Node>(ThisNode);
		if (K2Node == nullptr || K2Node->IsNodePure())
		{
			continue;
		}

		// create map of all non-pure blueprint nodes with theirs pin connections
		for (const UEdGraphPin* ThisPin : ThisNode->Pins)
		{
			if (ThisPin->Direction == EGPD_Output && ThisPin->LinkedTo.Num() > 0)
			{
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();

					// we assume all imported nodes except the entry point node represent functions
					// only the first node from the left in the blueprint uber-graph has to be the Event node (UK2Node_Event)
					const UK2Node_CallFunction* LinkedFunctionNode = Cast<UK2Node_CallFunction>(LinkedNode);
					if (LinkedFunctionNode && !LinkedFunctionNode->IsNodePure())
					{
						FGraphNodeImport& ThisNodeImport = ImportedNodes.FindOrAdd(ThisNode->NodeGuid);
						ThisNodeImport.Outputs.Add(FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));

						FGraphNodeImport& LinkedNodeImport = ImportedNodes.FindOrAdd(LinkedNode->NodeGuid);
						LinkedNodeImport.SourceGraphNode = LinkedNode;
						LinkedNodeImport.Inputs.Add(FConnectedPin(ThisNode->NodeGuid, ThisPin->PinName));
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

	// can't start import if provided graph doesn't have required start node
	// todo: do we really needs this?
	if (StartNode == nullptr)
	{
		return;
	}

	// clear existing graph
	UEdGraph* FlowGraph = FlowAsset->GetGraph();
	FlowGraph->Nodes.Empty();

	// recreated UFlowNode_Start, assign it a blueprint node FGuid
	UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, nullptr, UFlowNode_Start::StaticClass(), FVector2D::ZeroVector);
	FlowGraph->GetSchema()->SetNodeMetaData(NewGraphNode, FNodeMetadata::DefaultGraphNode);
	NewGraphNode->NodeGuid = StartNode->NodeGuid;
	NewGraphNode->GetFlowNode()->SetGuid(StartNode->NodeGuid);

	// execute graph import
	ImportBlueprintFunction_Recursive(FlowGraph->Nodes[0], ImportedNodes);
}

void UFlowImportUtils::ImportBlueprintFunction_Recursive(UEdGraphNode* PrecedingGraphNode, const TMap<FGuid, FGraphNodeImport> SourceNodes)
{
	UEdGraph* Graph = PrecedingGraphNode->GetGraph();
	const FGraphNodeImport& PrecedingNode = SourceNodes.FindRef(PrecedingGraphNode->NodeGuid);

	for (const FConnectedPin& Output : PrecedingNode.Outputs)
	{
		// todo: support multiple output pins
		//UFlowNode* LinkedNodeDefaults = MatchingFlowNodeClass.GetDefaultObject();
		//const FName PinName = LinkedNodeDefaults->OutputPins[0].PinName;
		UEdGraphPin* OutputPin = Cast<UFlowGraphNode>(PrecedingGraphNode)->OutputPins[0];

		const FGuid& LinkedNodeGuid = Output.NodeGuid;
		const FGraphNodeImport& LinkedNodeImport = SourceNodes.FindRef(LinkedNodeGuid);

		// we only accept here blueprint nodes representing functions
		const UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(LinkedNodeImport.SourceGraphNode);
		if (FunctionNode == nullptr)
		{
			continue;
		}

		// find FlowNode class matching provided UFunction name
		const TSubclassOf<UFlowNode> MatchingFlowNodeClass = UFlowSettings::Get()->BlueprintFunctionsToFlowNodes.FindRef(FunctionNode->GetFunctionName());
		if (MatchingFlowNodeClass == nullptr)
		{
			continue;
		}

		// check if already imported connected node
		// todo: optimize?
		UFlowGraphNode* LinkedGraphNode = nullptr;
		for (const TObjectPtr<UEdGraphNode> ExistingGraphNode : Graph->Nodes)
		{
			if (ExistingGraphNode->NodeGuid == LinkedNodeGuid)
			{
				LinkedGraphNode = Cast<UFlowGraphNode>(ExistingGraphNode);
				break;
			}
		}

		if (LinkedGraphNode == nullptr)
		{
			// create a new Flow Graph node
			const FVector2d Location = FVector2D(LinkedNodeImport.SourceGraphNode->NodePosX, LinkedNodeImport.SourceGraphNode->NodePosY);
			LinkedGraphNode = FFlowGraphSchemaAction_NewNode::ImportNode(PrecedingGraphNode->GetGraph(), OutputPin, MatchingFlowNodeClass, LinkedNodeGuid, Location);
		}
		else
		{
			UEdGraphPin* LinkedPin = nullptr;
			for (UEdGraphPin* InputPin : LinkedGraphNode->InputPins)
			{
				if (InputPin->PinName == Output.PinName)
				{
					LinkedPin = InputPin;
					break;
				}
			}

			// just link the pin to existing node
			Graph->GetSchema()->TryCreateConnection(OutputPin, LinkedPin);
		}

		if (LinkedGraphNode)
		{
			// transfer properties from UFunction input parameters to Flow Node properties
			{
				TMap<FName, UEdGraphPin*> InputPins;
				for (UEdGraphPin* Pin : FunctionNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && !Pin->bHidden && !Pin->bOrphanedPin)
					{
						InputPins.Add(Pin->PinName, Pin);
					}
				}

				for (TFieldIterator<FProperty> PropIt(LinkedGraphNode->GetFlowNode()->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
				{
					const FProperty* Param = *PropIt;
					const bool bIsEditable = !Param->HasAnyPropertyFlags(CPF_Edit | CPF_Deprecated);
					if (bIsEditable)
					{
						if (const UEdGraphPin* InputPin = InputPins.FindRef(*Param->GetAuthoredName()))
						{
							FString const PinValue = InputPin->GetDefaultAsString();
							uint8* Offset = Param->ContainerPtrToValuePtr<uint8>(LinkedGraphNode->GetFlowNode());
							Param->ImportText(*PinValue, Offset, PPF_Copy, nullptr, GLog);
						}
					}
				}
			}

			// iterate next nodes
			ImportBlueprintFunction_Recursive(LinkedGraphNode, SourceNodes);
		}
	}
}

#undef LOCTEXT_NAMESPACE
