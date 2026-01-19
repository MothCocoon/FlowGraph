// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IDetailCustomNodeBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h"
#include "SResetToDefaultMenu.h"

DECLARE_DELEGATE_FourParams(FOnGenerateArrayElementWidgetVisible,
	TSharedRef<IPropertyHandle>,      // ElementHandle
	int32,                            // Index
	IDetailChildrenBuilder&,          // ChildrenBuilder
	const TAttribute<EVisibility>&);  // RowVisibility

/*
* FVisibilityArrayBuilder
*
* A fork of UE's FDetailArrayBuilder that:
*  - Adds a live visibility getter via SetVisibilityGetter(TFunction<EVisibility()>)
*  - Uses a 4‑parameter element generation delegate:
*        (ElementHandle, Index, ChildrenBuilder, RowVisibility)
*  - Works around engine behavior where setting NodeRow.Visibility() on a custom node
*    header does not always live‑update by wrapping the header name & value widgets
*    in SBoxes whose Visibility attributes are dynamic.
*
* Usage:
*    TSharedRef<FVisibilityArrayBuilder> ArrayBuilder =
*        MakeShareable(new FVisibilityArrayBuilder(ValuesHandle.ToSharedRef(), true, true, true));
*
*    ArrayBuilder->SetVisibilityGetter([this]()
*    {
*        uint8 Raw = 0;
*        if (MultiTypeHandle.IsValid() &&
*            MultiTypeHandle->GetValue(Raw) == FPropertyAccess::Success &&
*            (EFlowDataMultiType)Raw == EFlowDataMultiType::Array)
*        {
*            return EVisibility::Visible;
*        }
*        return EVisibility::Collapsed;
*    });
*
*    ArrayBuilder->OnGenerateArrayElementWidget(
*        FOnGenerateArrayElementWidgetVisible::CreateSP(
*            this, &FFlowDataPinValueCustomization_Enum::GenerateArrayElementVisible));
*
*    StructBuilder.AddCustomBuilder(ArrayBuilder);
*
* Notes:
*  - Structural changes (Add / Remove / Reorder) still require RequestRefresh() to rebuild rows.
*  - The dynamic visibility lambda MUST read the property handle each time (no cached enum mode).
*  - If you want to supply your own header (e.g. to insert custom buttons), construct with
*      bInGenerateHeader = false
*    and add a separate AddCustomRow() above the builder with its own .Visibility binding.
*/

class FVisibilityArrayBuilder : public IDetailCustomNodeBuilder
{
public:
	FVisibilityArrayBuilder(TSharedRef<IPropertyHandle> InBaseProperty,
		bool bInGenerateHeader = true,
		bool bInDisplayResetToDefault = true,
		bool bInDisplayElementNum = true)
		: ArrayProperty(InBaseProperty->AsArray())
		, BaseProperty(InBaseProperty)
		, bGenerateHeader(bInGenerateHeader)
		, bDisplayResetToDefault(bInDisplayResetToDefault)
		, bDisplayElementNum(bInDisplayElementNum)
	{
		check(ArrayProperty.IsValid());

		FSimpleDelegate OnNumChildrenChanged =
			FSimpleDelegate::CreateRaw(this, &FVisibilityArrayBuilder::OnNumChildrenChanged);
		OnNumElementsChangedHandle = ArrayProperty->SetOnNumElementsChanged(OnNumChildrenChanged);

		// Hide original property presentation so only our custom builder is shown.
		BaseProperty->MarkHiddenByCustomization();
	}

	~FVisibilityArrayBuilder()
	{
		if (ArrayProperty.IsValid())
		{
			ArrayProperty->UnregisterOnNumElementsChanged(OnNumElementsChangedHandle);
		}
	}

	// Non-copyable / non-movable: avoid duplicate delegate registrations
	FVisibilityArrayBuilder(const FVisibilityArrayBuilder&) = delete;
	FVisibilityArrayBuilder& operator=(const FVisibilityArrayBuilder&) = delete;
	FVisibilityArrayBuilder(FVisibilityArrayBuilder&&) = delete;
	FVisibilityArrayBuilder& operator=(FVisibilityArrayBuilder&&) = delete;

	// Assign a visibility callback (evaluated whenever Slate queries the attribute).
	FVisibilityArrayBuilder& SetVisibilityGetter(TFunction<EVisibility()>&& InGetter)
	{
		VisibilityGetter = MoveTemp(InGetter);
		return *this;
	}

	void SetDisplayName(const FText& InDisplayName)
	{
		DisplayName = InDisplayName;
	}

	void OnGenerateArrayElementWidget(FOnGenerateArrayElementWidgetVisible InDelegate)
	{
		OnGenerateArrayElementWidgetDelegate = InDelegate;
	}

	// IDetailCustomNodeBuilder interface
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick(float /*DeltaTime*/) override {}

	virtual FName GetName() const override
	{
		return BaseProperty->GetProperty()->GetFName();
	}

	virtual bool InitiallyCollapsed() const override { return false; }

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
	{
		if (!bGenerateHeader)
		{
			return;
		}

		// Dynamic visibility attribute for header contents (NOT on NodeRow itself).
		TAttribute<EVisibility> DynamicVis = MakeDynamicVisibilityAttribute();

		// Horizontal box for value content (mirrors stock array builder's layout).
		TSharedPtr<SHorizontalBox> ContentHorizontalBox;
		SAssignNew(ContentHorizontalBox, SHorizontalBox);

		if (bDisplayElementNum)
		{
			ContentHorizontalBox->AddSlot()
				[
					BaseProperty->CreatePropertyValueWidget()
				];
		}

		FUIAction CopyAction;
		FUIAction PasteAction;
		BaseProperty->CreateDefaultPropertyCopyPasteActions(CopyAction, PasteAction);

		NodeRow
			// Leave NodeRow itself always present; hide internal widgets via SBox visibility.
			.FilterString(!DisplayName.IsEmpty() ? DisplayName : BaseProperty->GetPropertyDisplayName())
			.NameContent()
			[
				SNew(SBox)
					.Visibility(DynamicVis)
					[
						BaseProperty->CreatePropertyNameWidget(DisplayName, FText::GetEmpty())
					]
			]
		.ValueContent()
			[
				SNew(SBox)
					.Visibility(DynamicVis)
					[
						ContentHorizontalBox.ToSharedRef()
					]
			]
		.CopyAction(CopyAction)
			.PasteAction(PasteAction);

		if (bDisplayResetToDefault)
		{
			TSharedPtr<SResetToDefaultMenu> ResetToDefaultMenu;
			ContentHorizontalBox->AddSlot()
				.AutoWidth()
				.Padding(FMargin(2.f, 0.f, 0.f, 0.f))
				[
					SAssignNew(ResetToDefaultMenu, SResetToDefaultMenu)
				];
			ResetToDefaultMenu->AddProperty(BaseProperty);
		}
	}

	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
	{
		uint32 NumChildren = 0;
		ArrayProperty->GetNumElements(NumChildren);

		TAttribute<EVisibility> DynamicVis = MakeDynamicVisibilityAttribute();

		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
		{
			TSharedRef<IPropertyHandle> ElementHandle = ArrayProperty->GetElement(ChildIndex);

			if (OnGenerateArrayElementWidgetDelegate.IsBound())
			{
				OnGenerateArrayElementWidgetDelegate.Execute(
					ElementHandle,
					static_cast<int32>(ChildIndex),
					ChildrenBuilder,
					DynamicVis);
			}
			else
			{
				IDetailPropertyRow& Row = ChildrenBuilder.AddProperty(ElementHandle);
				Row.Visibility(DynamicVis);
			}
		}
	}

	// Manual refresh (not virtual in some engine versions)
	void RefreshChildren()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const
	{
		return BaseProperty;
	}

protected:
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override
	{
		OnRebuildChildren = InOnRebuildChildren;
	}

	void OnNumChildrenChanged()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

private:
	TAttribute<EVisibility> MakeDynamicVisibilityAttribute() const
	{
		// Optimization: if no custom getter, return a simple constant attribute (no lambda capture)
		if (!VisibilityGetter)
		{
			return TAttribute<EVisibility>(EVisibility::Visible);
		}

		// Capture 'this' only when needed
		return TAttribute<EVisibility>::CreateLambda([this]()
			{
				return VisibilityGetter();
			});
	}

private:
	// Display name override
	FText DisplayName;

	// Element generator
	FOnGenerateArrayElementWidgetVisible OnGenerateArrayElementWidgetDelegate;

	// Array + base property handles
	TSharedPtr<IPropertyHandleArray> ArrayProperty;
	TSharedRef<IPropertyHandle>      BaseProperty;

	// Rebuild delegate
	FSimpleDelegate OnRebuildChildren;

	// Visibility getter (live)
	TFunction<EVisibility()> VisibilityGetter;

	// Config
	bool bGenerateHeader;
	bool bDisplayResetToDefault;
	bool bDisplayElementNum;

	// Delegate handle for array size changes
	FDelegateHandle OnNumElementsChangedHandle;
};