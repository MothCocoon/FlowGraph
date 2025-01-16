// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Log.generated.h"

// Variant of ELogVerbosity
UENUM(BlueprintType)
enum class EFlowLogVerbosity : uint8
{
	Error		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Warning		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Display		UMETA(ToolTip = "Prints a message to console (and log file)"),
	Log			UMETA(ToolTip = "Prints a message to a log file (does not print to console)"),
	Verbose		UMETA(ToolTip = "Prints a verbose message to a log file (if Verbose logging is enabled for the given category, usually used for detailed logging)"),
	VeryVerbose	UMETA(ToolTip = "Prints a verbose message to a log file (if VeryVerbose logging is enabled, usually used for detailed logging that would otherwise spam output)"),
};

/**
 * Adds message to log
 * Optionally shows message on screen
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Log", Keywords = "print"))
class FLOW_API UFlowNode_Log : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	// The message to write to the log
	// (if the Message input pin is not connected to another source)
	UPROPERTY(EditAnywhere, Category = "Flow", meta = (DefaultForInputFlowPin, FlowPinType = String))
	FString Message;

	UPROPERTY(EditAnywhere, Category = "Flow")
	EFlowLogVerbosity Verbosity;

	UPROPERTY(EditAnywhere, Category = "Flow")
	bool bPrintToScreen;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen", EditConditionHides))
	float Duration;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen", EditConditionHides))
	FColor TextColor;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual void UpdateNodeConfigText_Implementation() override;
#endif
};
