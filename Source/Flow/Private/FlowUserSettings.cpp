// Fill out your copyright notice in the Description page of Project Settings.

#include "FlowUserSettings.h"

UFlowUserSettings::UFlowUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bSetFirstAssetInstanceAsInspected(true)
	, bKeepLastInspectedInstance(false)
{
}