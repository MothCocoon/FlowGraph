// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSettings.h"

#include "FlowAsset.h"
#include "FlowTags.h"
#include "Graph/FlowGraphSchema.h"
#include "Types/FlowGameplayTagMapUtils.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphSettings)

#define LOCTEXT_NAMESPACE "FlowGraphSettings"

UFlowGraphSettings::UFlowGraphSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bExposeFlowAssetCreation(true)
	, bExposeFlowNodeCreation(true)
	, bShowAssetToolbarAboveLevelEditor(true)
	, FlowAssetCategoryName(LOCTEXT("FlowAssetCategory", "Flow"))
	, DefaultFlowAssetClass(UFlowAsset::StaticClass())
	, WorldAssetClass(UFlowAsset::StaticClass())
	, bShowDefaultPinNames(false)
	, ExecPinColorModifier(0.75f, 0.75f, 0.75f, 1.0f)
	, NodeDescriptionBackground(FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f))
	, NodeStatusBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, NodePreloadedBackground(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f))
	, ConnectionDrawType(EFlowConnectionDrawType::Default)
	, CircuitConnectionAngle(45.f)
	, CircuitConnectionSpacing(FVector2D(30.f))
	, InactiveWireColor(FLinearColor(0.364f, 0.364f, 0.364f, 1.0f))
	, InactiveWireThickness(1.5f)
	, RecentWireDuration(3.0f)
	, RecentWireColor(FLinearColor(1.0f, 0.05f, 0.0f, 1.0f))
	, RecentWireThickness(6.0f)
	, RecordedWireColor(FLinearColor(0.432f, 0.258f, 0.096f, 1.0f))
	, RecordedWireThickness(3.5f)
	, SelectedWireColor(FLinearColor(0.984f, 0.482f, 0.010f, 1.0f))
	, SelectedWireThickness(1.5f)
{
	NodePrefixesToRemove.Emplace("FN");
	NodePrefixesToRemove.Emplace("FlowNode");
	NodePrefixesToRemove.Emplace("FlowNodeAddOn");
}

void UFlowGraphSettings::PostInitProperties()
{
	Super::PostInitProperties();

	NodePrefixesToRemove.Sort(TGreater{});
}

#if WITH_EDITOR

void UFlowGraphSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED( UFlowGraphSettings, NodePrefixesToRemove ))
	{
		//
		// We need to sort items in array, because unsorted array can cause only partial prefix removal.
		// For example, we have a NodeName = "UFlowNode_Custom" and these elements in the array:
		//		NodePrefixesToRemove = {"FN", "Flow", "FlowNode"};
		// Note: Prefix "U" is removed by Unreal
		// 
		//	First prefix does not match start of NodeName, so nothing changes.
		//	Second prefix match start of the name and is removed, NodeName becomes "Node_Custom"
		//  Third prefix does not match start of NodeName, so nothing changes.
		//  After complete process NodeName == "Node_Custom", but expected result is "Custom"
		//
		// If NodePrefixesToRemove = {"FN", "FlowNode", "Flow"} instead, everything will be removed as expected.
		//
		
		if (FlowArray::TrySortAndRemoveDuplicatesFromArrayInPlace(NodePrefixesToRemove))
		{
			// error notification
			FNotificationInfo Info(LOCTEXT("FlowGraphSettings_DuplicatePrefixError", "Added prefix already exists in array."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		}
		else
		{
			UFlowGraphSchema::UpdateGeneratedDisplayNames();
		}
	}
	else if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowGraphSettings, NodeDisplayStyles))
	{
		if (FlowArray::TrySortAndRemoveDuplicatesFromArrayInPlace(NodeDisplayStyles))
		{
			// error notification
			FNotificationInfo Info(LOCTEXT("FlowGraphSettings_DuplicateNodeDisplayStyleError", "Added NodeDisplayStyle already exists in array."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
}

const TMap<FGameplayTag, FFlowNodeDisplayStyleConfig>& UFlowGraphSettings::EnsureNodeDisplayStylesMap()
{
	if (NodeDisplayStylesAuthoredTags.Num() != NodeDisplayStyles.Num())
	{
		NodeDisplayStylesAuthoredTags.Reset();

		// Create an expanded GameplayTag map that will allow the settings to be looked up by subtag
		TMap<FGameplayTag, FFlowNodeDisplayStyleConfig> UnexpandedMap;
		UnexpandedMap.Reserve(NodeDisplayStyles.Num());

		for (const FFlowNodeDisplayStyleConfig& Config : NodeDisplayStyles)
		{
			UnexpandedMap.Add(Config.Tag, Config);
			NodeDisplayStylesAuthoredTags.AddTag(Config.Tag);
		}

		// Expand the map
		NodeDisplayStylesMap.Empty();
		FlowMap::PatchGameplayTagMap<EFlowGameplayTagMapExpandPolicy::AllSubtags>(UnexpandedMap, NodeDisplayStylesMap);
	}

	return NodeDisplayStylesMap;
}

bool UFlowGraphSettings::TryAddDefaultNodeDisplayStyle(const FFlowNodeDisplayStyleConfig& StyleConfig)
{
	const int32 FoundIndex = 
		NodeDisplayStyles.FindLastByPredicate(
			[&StyleConfig](const FFlowNodeDisplayStyleConfig& CurConfig)
			{
				if (CurConfig.Tag == StyleConfig.Tag)
				{
					return true;
				}

				return false;
			});

	if (FoundIndex != INDEX_NONE)
	{
		// Keep the existing config

		return false;
	}

	NodeDisplayStyles.Add(StyleConfig);

	return true;
}

const FLinearColor* UFlowGraphSettings::LookupNodeTitleColorForNode(const UFlowNodeBase& FlowNodeBase)
{
	if (const FLinearColor* NodeSpecificColor = NodeSpecificColors.Find(FlowNodeBase.GetClass()))
	{
		return NodeSpecificColor;
	}

	const FGameplayTag& StyleTag = FlowNodeBase.GetNodeDisplayStyle();
	const TMap<FGameplayTag, FFlowNodeDisplayStyleConfig>& StyleMap = EnsureNodeDisplayStylesMap();

	if (const FFlowNodeDisplayStyleConfig* Config = FlowMap::TryLookupGameplayTagKey(StyleTag, StyleMap, FlowNodeStyle::CategoryName))
	{
		return &Config->TitleColor;
	}

	return nullptr;
}

#endif

#undef LOCTEXT_NAMESPACE
