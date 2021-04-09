#include "Nodes/Route/FlowNode_ExecutionMultiGate.h"

UFlowNode_ExecutionMultiGate::UFlowNode_ExecutionMultiGate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StartIndex(INDEX_NONE)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	InputNames.Add(TEXT("Reset"));
	SetNumericalOutputs(0, 1);
}

void UFlowNode_ExecutionMultiGate::ExecuteInput(const FName& PinName)
{
	if (PinName == DefaultInputName)
	{
		if (Completed.Num() == 0)
		{
			Completed.Init(false, OutputNames.Num());
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
			TriggerOutput(OutputNames[Index], false);
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
			NextOutput = ++NextOutput % OutputNames.Num();

			Completed[CurrentOutput] = true;
			TriggerOutput(OutputNames[CurrentOutput], false);
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

		if (OutputNames.IsValidIndex(StartIndex))
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
