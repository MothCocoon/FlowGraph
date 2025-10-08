// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinValuesStandard.h"
#include "Nodes/FlowPin.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinValuesStandard)

#if WITH_EDITOR
void FFlowDataPinValue_Enum::OnEnumNameChanged()
{
	if (!EnumName.IsEmpty())
	{
		EnumClass = UClass::TryFindTypeSlow<UEnum>(EnumName, EFindFirstObjectOptions::ExactClass);

		if (EnumClass != nullptr && !FFlowPin::ValidateEnum(*EnumClass))
		{
			EnumClass = nullptr;
		}
	}
}
#endif