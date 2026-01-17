#pragma once

#if WITH_EDITOR

#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"

#include "Interfaces/FlowDataPinValueOwnerInterface.h"

class IFlowDataPinValueOwnerInterface;

/*
* Template customization for owner types implementing IFlowDataPinValueOwnerInterface.
* Captures the layout builder pointer and installs a rebuild delegate.
*/
template <class OwnerT>
class TFlowDataPinValueOwnerCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new TFlowDataPinValueOwnerCustomization<OwnerT>());
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		CachedBuilder = &DetailBuilder;

		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder.GetObjectsBeingCustomized(Objects);

		for (TWeakObjectPtr<UObject>& Obj : Objects)
		{
			OwnerT* TypedOwner = Cast<OwnerT>(Obj.Get());
			if (!TypedOwner)
			{
				continue;
			}

			if (IFlowDataPinValueOwnerInterface* InterfacePtr = Cast<IFlowDataPinValueOwnerInterface>(TypedOwner))
			{
				InterfacePtr->SetFlowDataPinValuesRebuildDelegate(
					FSimpleDelegate::CreateSP(this, &TFlowDataPinValueOwnerCustomization<OwnerT>::RequestRebuild));
			}
		}
	}

private:
	IDetailLayoutBuilder* CachedBuilder = nullptr;

	void RequestRebuild()
	{
		if (CachedBuilder)
		{
			CachedBuilder->ForceRefreshDetails(); // Full rebuild; will recreate this customization instance
		}
	}
};

#endif // WITH_EDITOR