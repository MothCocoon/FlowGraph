#include "Nodes/FlowNodeBlueprintFactory.h"

#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBlueprint.h"

#include "Kismet2/KismetEditorUtilities.h"

UFlowNodeBlueprintFactory::UFlowNodeBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UFlowNodeBlueprint::StaticClass();
	ParentClass = UFlowNode::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UFlowNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UFlowNodeBlueprint* NewBP = CastChecked<UFlowNodeBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BPTYPE_Normal, UFlowNodeBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));
	return NewBP;
}

UObject* UFlowNodeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}
