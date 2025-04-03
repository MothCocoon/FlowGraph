// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_ComponentObserverDetails.h"
#include "Nodes/Actor/FlowNode_ComponentObserver.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FFlowNode_ComponentObserverDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("ObservedComponent");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_ComponentObserver, IdentityTags));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_ComponentObserver, IdentityMatchType));
}
