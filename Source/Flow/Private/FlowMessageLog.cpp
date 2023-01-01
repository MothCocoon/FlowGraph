// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowMessageLog.h"
#include "Nodes/FlowNode.h"
#include "FlowAsset.h"

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FlowMessageLog"

const FName FFlowMessageLog::LogName(TEXT("FlowGraph"));

FFlowGraphToken::FFlowGraphToken(const UFlowAsset* InFlowAsset)
{
	CachedText = FText::FromString(InFlowAsset->GetClass()->GetPathName());
}

FFlowGraphToken::FFlowGraphToken(const UFlowNode* InFlowNode)
	: GraphNode(InFlowNode->GetGraphNode())
{
	CachedText = InFlowNode->GetNodeTitle();
}

FFlowGraphToken::FFlowGraphToken(UEdGraphNode* InGraphNode, const UEdGraphPin* InPin)
	: GraphNode(InGraphNode)
	, GraphPin(InPin)
{
	if (InPin)
	{
		CachedText = InPin->GetDisplayName();
		if (CachedText.IsEmpty())
		{
			CachedText = LOCTEXT("UnnamedPin", "<Unnamed Pin>");
		}
	}
	else
	{
		CachedText = GraphNode->GetNodeTitle(ENodeTitleType::ListView);
	}
}

TSharedPtr<IMessageToken> FFlowGraphToken::Create(const UFlowAsset* InFlowAsset, FTokenizedMessage& Message)
{
	if (InFlowAsset)
	{
		Message.AddToken(MakeShareable(new FFlowGraphToken(InFlowAsset)));
		return Message.GetMessageTokens().Last();
	}

	return nullptr;
}

TSharedPtr<IMessageToken> FFlowGraphToken::Create(const UFlowNode* InFlowNode, FTokenizedMessage& Message)
{
	if (InFlowNode)
	{
		Message.AddToken(MakeShareable(new FFlowGraphToken(InFlowNode)));
		return Message.GetMessageTokens().Last();
	}
	
	return nullptr;
}

TSharedPtr<IMessageToken> FFlowGraphToken::Create(UEdGraphNode* InGraphNode, FTokenizedMessage& Message)
{
	if (InGraphNode)
	{
		Message.AddToken(MakeShareable(new FFlowGraphToken(InGraphNode, nullptr)));
		return Message.GetMessageTokens().Last();
	}

	return nullptr;
}

TSharedPtr<IMessageToken> FFlowGraphToken::Create(const UEdGraphPin* InPin, FTokenizedMessage& Message)
{
	if (InPin && InPin->GetOwningNode())
	{
		Message.AddToken(MakeShareable(new FFlowGraphToken(InPin->GetOwningNode(), InPin)));
		return Message.GetMessageTokens().Last();
	}
	
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
#endif // WITH_EDITOR
