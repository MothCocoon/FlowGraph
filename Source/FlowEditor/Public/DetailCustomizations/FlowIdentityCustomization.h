// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayTagContainer.h"
#include "IPropertyTypeCustomization.h"

#include "FlowComponent.h"
#include "Types/FlowIdentity.h"

#include "StructUtils/InstancedStruct.h"

/**
 * Flow Identity Tag Selector
 */
class FLOWEDITOR_API SFlowIdentityTagSelector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowIdentityTagSelector)
	{
	}

	SLATE_END_ARGS()

public:
	FSimpleDelegate OnAction;

public:
	void Construct(const FArguments& InArgs, const TSharedPtr<IPropertyHandle>& InIdentityTagsHandle);

	void SetSourceFromActor(const AActor* Actor) { SetSourceFromComponent(Actor ? Actor->FindComponentByClass<UFlowComponent>() : nullptr); }
	void SetSourceFromComponent(UFlowComponent* Component);

protected:
	TArray<FGameplayTagContainer> GetCurrentTags() const;
	FText GetSourceName() const;
	FText GetSourceTooltip() const;

	void RebuildTagOptions();

	bool IsTagSelected(const FGameplayTag GameplayTag) const { return GetTagState(GameplayTag) == ECheckBoxState::Checked; }
	ECheckBoxState GetTagState(const FGameplayTag GameplayTag) const;
	void SetTagSelected(const FGameplayTag GameplayTag, const bool bNewSelected);

	void ApplyAll();
	void AppendAll();
	void ClearTags();

	void ActionExecuted();

	static TSharedRef<SWidget> CreateSettingsMenu();

	static FString TagsToPropertyString(const FGameplayTagContainer& Tags)
	{
		FString Value;
		FGameplayTagContainer::StaticStruct()->ExportText(Value, &Tags, &Tags, nullptr, PPF_None, nullptr);
		return Value;
	}

private:
	TWeakObjectPtr<UFlowComponent> SelectionSource;
	TWeakPtr<IPropertyHandle> WeakTagsHandle;

	TSharedPtr<SVerticalBox> TagOptions;
	TSharedPtr<SVerticalBox> CurrentTagsPanel;
};

/**
 * Flow Identity Customization
 */
class FLOWEDITOR_API FFlowIdentityCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowIdentityCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:
	void UpdateCachedStructs();
	
	virtual bool IsActorAllowed(const AActor* Actor) const;	
	virtual bool IsActorMatches(const AActor* Actor) const;		

	virtual void ResolveCategoriesMeta(TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const;

	virtual TSharedRef<SWidget> CreateHeaderWidget();
	virtual TSharedRef<SWidget> CreateTagPicker();

	void OpenTagPicker_UseCurrentSelection();
	void OpenTagPicker_UseActor(AActor* Actor);

	void FocusActor(AActor* Actor);

	void UseActor_SelectedInViewport() const;
	void UseActor_Explicit(AActor* Actor) const;
	void UseActor_FromEyeDrop();

	virtual TSharedRef<SWidget> MenuContent_ActorPicker();
	virtual TSharedRef<SWidget> MenuContent_FindMatching();

	static bool CanOpenMatchingPopup();
	void FindMatchingPopup();

protected:
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> IdentityTagsHandle;	

	TArray<TInstancedStruct<FFlowIdentity>> CachedIdentities;

	TSharedPtr<IDetailsView> OwningView;

	TSharedPtr<SWidget> HeaderWidget;
	TSharedPtr<SWidget> PopUpAnchor;

	TSharedPtr<class SFlowIdentityTagSelector> TagSelector;
};
