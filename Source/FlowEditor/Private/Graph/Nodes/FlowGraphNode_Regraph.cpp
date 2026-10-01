// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "Nodes/FlowNodeBase.h"

bool UFlowGraphNode::AddSubNodeForRegraph(UFlowGraphNode* SubNode, UEdGraph* ParentGraph)
{
	if (!IsValid(SubNode) || !IsValid(ParentGraph))
	{
		return false;
	}

	SubNode->SetFlags(RF_Transactional);
	if (SubNode->GetOuter() != ParentGraph)
	{
		SubNode->Rename(nullptr, ParentGraph, REN_NonTransactional);
	}
	SubNode->SetParentNodeForSubNode(this);
	SubNode->CreateNewGuid();
	SubNode->PostPlacedNewNode();
	SubNode->AllocateDefaultPins();
	SubNodes.Add(SubNode);
	if (SubNode->NodeInstance)
	{
		SubNode->NodeInstance->OnAddOnRequestedParentReconstruction.BindUObject(
			this, &UFlowGraphNode::OnExternalChange);
	}
	OnSubNodeAdded(SubNode);
	return true;
}

void UFlowGraphNode::ReparentSubNodeForRegraph(UFlowGraphNode* SubNode)
{
	if (!IsValid(SubNode))
	{
		return;
	}

	UFlowGraphNode* const PreviousParent = SubNode->GetParentNode();
	if (IsValid(PreviousParent) && PreviousParent != this)
	{
		PreviousParent->RemoveSubNodeForRegraph(SubNode);
	}
	if (PreviousParent != this)
	{
		SubNode->SetParentNodeForSubNode(this);
		SubNodes.AddUnique(SubNode);
		OnSubNodeAdded(SubNode);
	}
	if (SubNode->NodeInstance)
	{
		SubNode->SubscribeToExternalChanges();
		SubNode->NodeInstance->OnAddOnRequestedParentReconstruction.BindUObject(
			this, &UFlowGraphNode::OnExternalChange);
	}
}

void UFlowGraphNode::RemoveSubNodeForRegraph(UFlowGraphNode* SubNode)
{
	if (!IsValid(SubNode))
	{
		return;
	}

	if (SubNode->NodeInstance)
	{
		if (SubNode->NodeInstance->OnAddOnRequestedParentReconstruction.IsBoundToObject(this))
		{
			SubNode->NodeInstance->OnAddOnRequestedParentReconstruction.Unbind();
		}
		if (SubNode->NodeInstance->OnReconstructionRequested.IsBoundToObject(SubNode))
		{
			SubNode->NodeInstance->OnReconstructionRequested.Unbind();
		}
	}
	const int32 RemovedCount = SubNodes.RemoveSingle(SubNode);
	if (SubNode->GetParentNode() == this)
	{
		SubNode->SetParentNodeForSubNode(nullptr);
	}
	if (RemovedCount > 0)
	{
		OnSubNodeRemoved(SubNode);
	}
}
