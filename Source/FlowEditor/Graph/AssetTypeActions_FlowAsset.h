#pragma once

#include "AssetTypeActions_Base.h"
#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"

class FMenuBuilder;
class UFlowAsset;

class FAssetTypeActions_FlowAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_FlowAsset", "Flow Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(129, 196, 115); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual bool CanFilter() override { return true; }
};
