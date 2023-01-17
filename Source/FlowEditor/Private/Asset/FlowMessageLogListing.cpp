// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowMessageLogListing.h"

#include "MessageLogModule.h"

#define LOCTEXT_NAMESPACE "FlowMessageLogListing"

FFlowMessageLogListing::FFlowMessageLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type)
	: Log(RegisterLogListing(InFlowAsset, Type))
{
}

FFlowMessageLogListing::~FFlowMessageLogListing()
{
	// Unregister the log so it will be ref-counted to zero if it has no messages
	if(Log->NumMessages(EMessageSeverity::Info) == 0)
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(Log->GetName());
	}
}

TSharedRef<IMessageLogListing> FFlowMessageLogListing::RegisterLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type)
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

	const FName LogName = GetListingName(InFlowAsset, Type);

	// Register the log (this will return an existing log if it has been used before)
	FMessageLogInitializationOptions LogInitOptions;
	LogInitOptions.bShowInLogWindow = false;
	MessageLogModule.RegisterLogListing(LogName, LOCTEXT("FlowGraphLogLabel", "FlowGraph"), LogInitOptions);
	return MessageLogModule.GetLogListing(LogName);
}

TSharedRef<IMessageLogListing> FFlowMessageLogListing::GetLogListing(const UFlowAsset* InFlowAsset, const EFlowLogType Type)
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");

	const FName LogName = GetListingName(InFlowAsset, Type);

	// Reuse any existing log, or create a new one (that is not held onto bey the message log system)
	if(MessageLogModule.IsRegisteredLogListing(LogName))
	{
		return MessageLogModule.GetLogListing(LogName);
	}
	else
	{
		FMessageLogInitializationOptions LogInitOptions;
		LogInitOptions.bShowInLogWindow = false;
		return MessageLogModule.CreateLogListing(LogName, LogInitOptions);
	}
}

FName FFlowMessageLogListing::GetListingName(const UFlowAsset* InFlowAsset, const EFlowLogType Type)
{
	FName LogListingName;
	if (InFlowAsset)
	{
		const FString TypeAsString = StaticEnum<EFlowLogType>()->GetNameStringByIndex(static_cast<int32>(Type));
		LogListingName = *FString::Printf(TEXT("FlowLog_%s_%s_%s_"), *TypeAsString, *InFlowAsset->AssetGuid.ToString(), *InFlowAsset->GetName());
	}
	else
	{
		LogListingName = "FlowGraph";
	}
	return LogListingName;
}

#undef LOCTEXT_NAMESPACE
