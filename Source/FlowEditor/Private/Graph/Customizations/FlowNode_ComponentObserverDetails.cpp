#include "Graph/Customizations/FlowNode_ComponentObserverDetails.h"
#include "Nodes/World/FlowNode_ComponentObserver.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FFlowNode_ComponentObserverDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("ObservedComponent");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_ComponentObserver, IdentityTag));
}
