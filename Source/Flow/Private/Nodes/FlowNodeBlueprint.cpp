#include "Nodes/FlowNodeBlueprint.h"

UFlowNodeBlueprint::UFlowNodeBlueprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
UFlowNodeBlueprint* UFlowNodeBlueprint::FindRootFlowNodeBlueprint(UFlowNodeBlueprint* DerivedBlueprint)
{
	UFlowNodeBlueprint* ParentBP = nullptr;

	// Determine if there is a Flow Node blueprint in the ancestry of this class
	for (UClass* ParentClass = DerivedBlueprint->ParentClass; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
	{
		if (UFlowNodeBlueprint* TestBP = Cast<UFlowNodeBlueprint>(ParentClass->ClassGeneratedBy))
		{
			ParentBP = TestBP;
		}
	}

	return ParentBP;
}
#endif
