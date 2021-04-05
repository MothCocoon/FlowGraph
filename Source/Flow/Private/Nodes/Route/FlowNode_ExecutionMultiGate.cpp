#include "Nodes/Route/FlowNode_ExecutionMultiGate.h"

#include "Misc/StringBuilder.h"

const FName UFlowNode_ExecutionMultiGate::ResetInputName = TEXT("Reset");

UFlowNode_ExecutionMultiGate::UFlowNode_ExecutionMultiGate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	InputNames.Empty();
	InputNames.Add(DefaultInputName);
	InputNames.Add(ResetInputName);
	
	SetNumericalOutputs(0, 1);
}

void UFlowNode_ExecutionMultiGate::ExecuteInput(const FName& PinName)
{
	if (PinName == ResetInputName)
	{
		Finish();
		return;
	}

	if (Completed.Num() == 0)
		Completed.Init(false, OutputNames.Num());

	if (!Completed.Contains(false))
		return;

	const bool bUseStartIndex = !Completed.Contains(true) && Completed.IsValidIndex(StartIndex);
	
	if (bRandom)
	{
		int32 Index = INDEX_NONE;
		if (bUseStartIndex)
			Index = StartIndex;
		else
		{
			TArray<int32> AvailableIndexes;
			AvailableIndexes.Reserve(Completed.Num());
			
			for (auto It = Completed.CreateConstIterator(); It; ++It)
				if (*It == false)
					AvailableIndexes.Add(It.GetIndex());

			const int32 Random = FMath::RandRange(0, AvailableIndexes.Num() - 1);
			Index = AvailableIndexes[Random];
		}

		Completed[Index] = true;
		TriggerOutput(OutputNames[Index], false);
	}
	else
	{
		if (bUseStartIndex)
			NextOutput = StartIndex;

		const int32 CurrentOutput = NextOutput;
		//We have to calculate NextOutput before TriggerOutput(..)
		//TriggerOutput may call Reset and Cleanup
		NextOutput = ++NextOutput % OutputNames.Num();

		Completed[CurrentOutput] = true;
		TriggerOutput(OutputNames[CurrentOutput], false);
	}

	if (!Completed.Contains(false) && bLoop)
		Finish();
}

void UFlowNode_ExecutionMultiGate::Cleanup()
{
	NextOutput = 0;
	Completed.Reset();
}

#if WITH_EDITOR
FString UFlowNode_ExecutionMultiGate::GetNodeDescription() const
{
	TStringBuilder<128> Builder;

	if (bRandom)
		Builder += TEXT("Random");

	if (bRandom && bLoop)
		Builder += TEXT(", ");
	
	if (bLoop)
		Builder += TEXT("Loop");

	if (StartIndex != INDEX_NONE)
	{
		if (bRandom || bLoop)
			Builder += TEXT(", ");
		
		if (OutputNames.IsValidIndex(StartIndex))
			Builder += TEXT("StartIndex: ") + FString::FromInt(StartIndex);
		else
			Builder += TEXT("StartIndex: Invalid");
	}
	
	return Builder.ToString();
}
#endif
