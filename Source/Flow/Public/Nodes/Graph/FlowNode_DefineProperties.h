// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Interfaces/FlowNamedPropertiesSupplierInterface.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowNamedDataPinProperty.h"

#include "FlowNode_DefineProperties.generated.h"

/**
 * FlowNode to define data pin property literals for use connecting to data pin inputs in a flow graph.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Define Properties"))
class FLOW_API UFlowNode_DefineProperties 
	: public UFlowNode
	, public IFlowNamedPropertiesSupplierInterface
{
	GENERATED_BODY()
	
public:
	UFlowNode_DefineProperties();

protected:
	/* Instance-defined properties.
	 * These will auto-generate a matching pin that is bound to its property as its data source. */
	UPROPERTY(EditAnywhere, Category = "Configuration", DisplayName = Properties)
	TArray<FFlowNamedDataPinProperty> NamedProperties;

public:
	virtual void PostLoad() override;

#if WITH_EDITOR
	// IFlowContextPinSupplierInterface
	virtual bool SupportsContextPins() const override;
	// --

	// UObject
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	// --
#endif

	// IFlowNamedPropertiesSupplierInterface
	virtual TArray<FFlowNamedDataPinProperty>& GetMutableNamedProperties() override { return NamedProperties; }
	// --

	bool TryFormatTextWithNamedPropertiesAsParameters(const FText& FormatText, FText& OutFormattedText) const;

protected:
#if WITH_EDITOR
	/* Utility function for subclasses, if they want to force a named property to be Input or Output.
	 * Unused in this class. */
	void OnPostEditEnsureAllNamedPropertiesPinDirection(const FProperty& Property, bool bIsInput);
#endif
};
