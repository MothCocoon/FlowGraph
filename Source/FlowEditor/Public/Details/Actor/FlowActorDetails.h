// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "ActorDetailsDelegates.h"
#include "IDetailCustomNodeBuilder.h"

class IPropertyHandle;

/**
 * 
 */
class FLOWEDITOR_API FFlowActorDetails : public TSharedFromThis<FFlowActorDetails>
{
public:	
	virtual ~FFlowActorDetails();
	virtual void Register();
	virtual void AddFlowCategory(class IDetailLayoutBuilder& Details, const FGetSelectedActors& GetSelectedActors);
	
	static TArray<class UFlowComponent*> GetSelectedFlowComponents(const FGetSelectedActors& GetSelectedActors);
};

class FFlowActorDetailsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FFlowActorDetailsBuilder>
{
public:
	explicit FFlowActorDetailsBuilder(const FGetSelectedActors& GetSelectedActors);
	virtual ~FFlowActorDetailsBuilder() override;

	// IDetailCustomNodeBuilder
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override {}
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildBuilder) override;
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual FName GetName() const override;
	// --

private:
	static void AddSinglePropertyRow(IDetailChildrenBuilder& ChildBuilder, UObject* Component, const FText& RowName);

	static void PasteTags(TSharedPtr<IPropertyHandle> StructPropertyHandle);
	static bool CanPasteTags(TSharedPtr<IPropertyHandle> StructPropertyHandle);

	void ResolveCategoriesMeta(const TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const;

	FGetSelectedActors Getter;
	TArray<TWeakObjectPtr<UObject>> EditedComponents;
};
