// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlowGraphNode.h"
#include "FlowGraphNode_NamedReroute.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_NamedRerouteDeclaration : public UFlowGraphNode
{
	GENERATED_BODY()

public:
	UFlowGraphNode_NamedRerouteDeclaration(const FObjectInitializer& ObjectInitializer);
	
protected:
	// UEdGraphNode Begin
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	// UEdGraphNode End
	virtual void OnNodeDoubleClicked() const override;
	virtual void OnRenameNode(const FString& NewName) override;

private:
	void OnCreateRerouteUsageFromDeclaration() const;
	bool CanCreateRerouteUsageFromDeclaration() const;
	
	void OnSelectNamedRerouteUsages() const;
	bool CanSelectNamedRerouteUsages() const;
};

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_NamedRerouteUsage : public UFlowGraphNode
{
	GENERATED_BODY()

public:
	UFlowGraphNode_NamedRerouteUsage(const FObjectInitializer& ObjectInitializer);
	
protected:
	// UEdGraphNode Begin
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	// UEdGraphNode End
	virtual void OnNodeDoubleClicked() const override;

private:
	void OnSelectNamedRerouteDeclaration() const;
	bool CanSelectNamedRerouteDeclaration() const;
};


