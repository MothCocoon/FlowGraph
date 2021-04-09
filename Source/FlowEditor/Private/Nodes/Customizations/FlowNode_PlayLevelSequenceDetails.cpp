#include "FlowNode_PlayLevelSequenceDetails.h"
#include "Nodes/World/FlowNode_PlayLevelSequence.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FFlowNode_PlayLevelSequenceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("Sequence");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, Sequence));
}
