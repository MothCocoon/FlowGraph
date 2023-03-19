// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EditorSubsystem.h"
#include "FlowDebuggerSubsystem.generated.h"

class UFlowAsset;
class FFlowMessageLog;

/**
** Persistent subsystem supporting Flow Graph debugging
 */
UCLASS()
class FLOWEDITOR_API UFlowDebuggerSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
	
public:
	UFlowDebuggerSubsystem();

protected:	
	TMap<TWeakObjectPtr<UFlowAsset>, TSharedPtr<class IMessageLogListing>> RuntimeLogs;

	void OnInstancedTemplateAdded(UFlowAsset* FlowAsset);
	void OnInstancedTemplateRemoved(UFlowAsset* FlowAsset);
	
	void OnRuntimeMessageAdded(UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const;
	
	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

public:	
	static void PausePlaySession();
	static bool IsPlaySessionPaused();
};
