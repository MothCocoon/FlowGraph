// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_ExecutionMultiGate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_ExecutionMultiGate)

UFlowNode_ExecutionMultiGate::UFlowNode_ExecutionMultiGate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StartIndex(INDEX_NONE)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif

	FString ResetPinTooltip = TEXT("Finish work of this node.");
	ResetPinTooltip += LINE_TERMINATOR;
	ResetPinTooltip += TEXT("Calling In input will start triggering output pins once again.");

	InputPins.Add(FFlowPin(TEXT("Reset"), ResetPinTooltip));
	SetNumberedOutputPins(0, 1);
	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_ExecutionMultiGate::ExecuteInput(const FName& PinName)
{
	if (PinName == DefaultInputPin.PinName)
	{
		if (Completed.Num() == 0)
		{
			Completed.Init(false, OutputPins.Num());
		}

		if (!Completed.Contains(false))
		{
			return;
		}

		const bool bUseStartIndex = !Completed.Contains(true) && Completed.IsValidIndex(StartIndex);

		if (bRandom)
		{
			int32 Index;
			if (bUseStartIndex)
			{
				Index = StartIndex;
			}
			else
			{
				TArray<int32> AvailableIndexes;
				AvailableIndexes.Reserve(Completed.Num()); // todo

				for (int32 i = 0; i < Completed.Num(); i++)
				{
					if (Completed[i] == false)
					{
						AvailableIndexes.Emplace(i);
					}
				}

				const int32 Random = FMath::RandRange(0, AvailableIndexes.Num() - 1);
				Index = AvailableIndexes[Random];
			}

			Completed[Index] = true;
			TriggerOutput(OutputPins[Index].PinName, false);
		}
		else
		{
			if (bUseStartIndex)
			{
				NextOutput = StartIndex;
			}

			const int32 CurrentOutput = NextOutput;
			// We have to calculate NextOutput before TriggerOutput(..)
			// TriggerOutput may call Reset and Cleanup
			NextOutput = ++NextOutput % OutputPins.Num();

			Completed[CurrentOutput] = true;
			TriggerOutput(OutputPins[CurrentOutput].PinName, false);
		}

		if (!Completed.Contains(false) && bLoop)
		{
			Finish();
		}
	}
	else if (PinName == TEXT("Reset"))
	{
		Finish();
	}
}

void UFlowNode_ExecutionMultiGate::Cleanup()
{
	NextOutput = 0;
	Completed.Reset();
}

#if WITH_EDITOR
FString UFlowNode_ExecutionMultiGate::GetNodeDescription() const
{
	FString Result;
	Result.Reserve(128);

	if (bRandom)
	{
		Result.Append(TEXT("Random"));
	}

	if (bRandom && bLoop)
	{
		Result.Append(TEXT(", "));
	}

	if (bLoop)
	{
		Result.Append(TEXT("Loop"));
	}

	if (StartIndex != INDEX_NONE)
	{
		if (bRandom || bLoop)
		{
			Result.Append(TEXT(", "));
		}

		if (OutputPins.IsValidIndex(StartIndex))
		{
			Result.Appendf(TEXT("Start Index: %d"), StartIndex);
		}
		else
		{
			Result.Append(TEXT("StartIndex: Invalid"));
		}
	}

	return Result;
}
#endif
