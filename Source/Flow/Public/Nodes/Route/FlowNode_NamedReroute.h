// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "FlowNode_NamedReroute.generated.h"

UCLASS(Abstract)
class FLOW_API UFlowNode_NamedRerouteBase : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_NamedRerouteBase(const FObjectInitializer& ObjectInitializer);
	
protected:
	template<typename NodesArrayType>
	class UFlowNode_NamedRerouteDeclaration* FindDeclarationInArray(const FGuid& VariableGuid, const NodesArrayType& Nodes) const;
	
	class UFlowNode_NamedRerouteDeclaration* FindDeclarationInGraph(const FGuid& VariableGuid) const;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "NamedReroute"))
class FLOW_API UFlowNode_NamedRerouteDeclaration final : public UFlowNode_NamedRerouteBase
{
	GENERATED_BODY()

public:
	const FName& GetVariableName() const { return VariableName; }
	const FGuid& GetVariableGuid() const { return VariableGuid; }

#if WITH_EDITOR
	// virtual bool CanRenameNode() const override;
	// virtual FString GetEditableName() const override;
	void SetEditableName(const FString& NewName);
#endif
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
	
	//~ Begin UObject Interface
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End UObject Interface

#if WITH_EDITOR
	virtual void PostPasteNode(const TArray<UFlowNode*>& PastedNodes) override;
#endif

#if WITH_EDITOR
	virtual FText GetNodeToolTip() const override;
	virtual FText GetNodeTitle() const override;
	virtual bool CanUserAddInput() const override { return false; }
	virtual bool CanUserAddOutput() const override { return false; }
#endif
	
private:
	UPROPERTY(EditAnywhere, Category = NamedRerouteDeclaration)
	FName VariableName = TEXT("Name");

	// The variable GUID, to support copy across graphs
	UPROPERTY()
	FGuid VariableGuid;

#if WITH_EDITOR
	void MakeNameUnique();
#endif
	void UpdateVariableGuid(bool bForceGeneration, bool bAllowMarkingPackageDirty);
};

// This node is not placeable manually, only created from context popup menu
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "NamedRerouteUsage"))
class FLOW_API UFlowNode_NamedRerouteUsage final : public UFlowNode_NamedRerouteBase
{
	GENERATED_BODY()

public:
	UFlowNode_NamedRerouteUsage(const FObjectInitializer& ObjectInitializer);
	
	// The declaration this node is linked to
	UPROPERTY()
	TObjectPtr<UFlowNode_NamedRerouteDeclaration> Declaration;
	
	// The variable GUID, to support copy across graphs
	UPROPERTY()
	FGuid DeclarationVariableGuid;

	// Check that the declaration isn't deleted
	bool IsDeclarationValid() const;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual void PostPasteNode(const TArray<UFlowNode*>& PastedNodes) override;
#endif

#if WITH_EDITOR
	virtual FText GetNodeToolTip() const override;
	virtual FText GetNodeTitle() const override;
	virtual bool CanUserAddInput() const override { return false; }
	virtual bool CanUserAddOutput() const override { return false; }
	virtual EDataValidationResult ValidateNode() override;
#endif
};

template<typename NodesArrayType>
inline UFlowNode_NamedRerouteDeclaration* UFlowNode_NamedRerouteBase::FindDeclarationInArray(const FGuid& VariableGuid, const NodesArrayType& Nodes) const
{
	for (UFlowNode* Node : Nodes)
	{
		auto* Declaration = Cast<UFlowNode_NamedRerouteDeclaration>(Node);
		if (Declaration && this != Declaration && Declaration->GetVariableGuid() == VariableGuid)
		{
			return Declaration;
		}
	}
	return nullptr;
}
