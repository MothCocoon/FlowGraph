// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetIndexer.h"

#include "FlowAsset.h"
#include "Nodes/FlowNodeBase.h"

#include "Graph/Nodes/FlowGraphNode_Reroute.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "Internationalization/Text.h"
#include "SearchSerializer.h"
#include "Utility/IndexerUtilities.h"

#define LOCTEXT_NAMESPACE "FFlowAssetIndexer"

enum class EFlowAssetIndexerVersion
{
	Empty,
	Initial,

	// -----<new versions can be added above this line>-------------------------------------------------
	VersionPlusOne,
	LatestVersion = VersionPlusOne - 1
};

int32 FFlowAssetIndexer::GetVersion() const
{
	return static_cast<int32>(EFlowAssetIndexerVersion::LatestVersion);
}

void FFlowAssetIndexer::IndexAsset(const UObject* InAssetObject, FSearchSerializer& Serializer) const
{
	const UFlowAsset* FlowAsset = CastChecked<UFlowAsset>(InAssetObject);

	{
		Serializer.BeginIndexingObject(FlowAsset, TEXT("$self"));

		FIndexerUtilities::IterateIndexableProperties(FlowAsset, [&Serializer](const FProperty* Property, const FString& Value)
		{
			Serializer.IndexProperty(Property, Value);
		});

		Serializer.EndIndexingObject();
	}

	IndexGraph(FlowAsset, Serializer);
}

void FFlowAssetIndexer::IndexGraph(const UFlowAsset* InFlowAsset, FSearchSerializer& Serializer) const
{
	for (UEdGraphNode* Node : InFlowAsset->GetGraph()->Nodes)
	{
		// Ignore Reroutes
		if (Cast<UFlowGraphNode_Reroute>(Node))
		{
			continue;
		}

		// Special rules for comment nodes
		if (Cast<UEdGraphNode_Comment>(Node))
		{
			Serializer.BeginIndexingObject(Node, Node->NodeComment);
			Serializer.IndexProperty(TEXT("Comment"), Node->NodeComment);
			Serializer.EndIndexingObject();
			continue;
		}

		// Indexing UEdGraphNode
		{
			const FText NodeText = Node->GetNodeTitle(ENodeTitleType::MenuTitle);
			Serializer.BeginIndexingObject(Node, NodeText);
			Serializer.IndexProperty(TEXT("Title"), NodeText);

			if (!Node->NodeComment.IsEmpty())
			{
				Serializer.IndexProperty(TEXT("Comment"), Node->NodeComment);
			}

			for (const UEdGraphPin* Pin : Node->GetAllPins())
			{
				if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 0)
				{
					const FText PinText = Pin->GetDisplayName();
					if (PinText.IsEmpty())
					{
						continue;
					}

					const FText PinValue = Pin->GetDefaultAsText();
					if (PinValue.IsEmpty())
					{
						continue;
					}

					const FString PinLabel = TEXT("[Pin] ") + *FTextInspector::GetSourceString(PinText);
					Serializer.IndexProperty(PinLabel, PinValue);
				}
			}

			// This will serialize any user exposed options for the node that are editable in the Details
			FIndexerUtilities::IterateIndexableProperties(Node, [&Serializer](const FProperty* Property, const FString& Value)
			{
				Serializer.IndexProperty(Property, Value);
			});

			Serializer.EndIndexingObject();
		}

		// Indexing Flow Node
		if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
		{
			if (const UFlowNodeBase* FlowNodeBase = FlowGraphNode->GetFlowNodeBase())
			{
				const FString NodeFriendlyName = FString::Printf(TEXT("%s: %s"), *FlowNodeBase->GetClass()->GetName(), *FlowNodeBase->GetNodeDescription());
				Serializer.BeginIndexingObject(FlowNodeBase, NodeFriendlyName);
				FIndexerUtilities::IterateIndexableProperties(FlowNodeBase, [&Serializer](const FProperty* Property, const FString& Value)
				{
					Serializer.IndexProperty(Property, Value);
				});
				FlowGraphNode->AdditionalNodeIndexing(Serializer);
				Serializer.EndIndexingObject();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
