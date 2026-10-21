// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "FlowNode_Log.generated.h"

/**
 * Variant of ELogVerbosity.
 */
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
 * Adds message to developer log
 * Optionally shows message on screen
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Log", Keywords = "print"))
class FLOW_API UFlowNode_Log : public UFlowNode_DefineProperties
{
	GENERATED_BODY()

public:
	UFlowNode_Log();

private:
	/* The message to write to the log.
	 * If the Message input pin is not connected to another source. */
	UPROPERTY(EditAnywhere, Category = "Flow", meta = (DefaultForInputFlowPin, FlowPinType = String))
	FString Message;

	UPROPERTY(EditAnywhere, Category = "Flow")
	EFlowLogVerbosity Verbosity = EFlowLogVerbosity::Warning;

	UPROPERTY(EditAnywhere, Category = "Flow")
	bool bPrintToScreen = true;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen", EditConditionHides))
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (EditCondition = "bPrintToScreen", EditConditionHides))
	FColor TextColor = FColor::Yellow;

	/* Prints the message also on clients.
	 * Clients print the message only if it differs from the previous one. */
	UPROPERTY(EditAnywhere, Category = "Flow")
	bool bPrintOnClients = false;

	/* Formatted message sent to clients. */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedMessage)
	FString ReplicatedMessage;

	void PrintMessage(const FString& InMessage) const;

	UFUNCTION()
	void OnRep_ReplicatedMessage();

public:
	// UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// --

	// IFlowCoreExecutableInterface
	virtual void ExecuteInput(const FName& PinName) override;
	// --

protected:
	// UFlowNode
	virtual bool ShouldReplicateProperties() const override { return bPrintOnClients; }
	// --

public:
#if WITH_EDITOR
	// UObject
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	// --

	// UFlowNodeBase
	virtual void OnEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes) override;
	// --

	virtual void UpdateNodeConfigText_Implementation() override;
#endif

	EFlowLogVerbosity GetVerbosity() const { return Verbosity; }

#if WITH_EDITOR
public:
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
