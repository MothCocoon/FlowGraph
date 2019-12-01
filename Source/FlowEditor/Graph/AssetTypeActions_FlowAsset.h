#pragma once

#include "AssetTypeActions_Base.h"
#include "Toolkits/IToolkitHost.h"

class FAssetTypeActions_FlowAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_FlowAsset", "Flow Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(129, 196, 115); }
	virtual bool CanFilter() override { return true; }

	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
};
