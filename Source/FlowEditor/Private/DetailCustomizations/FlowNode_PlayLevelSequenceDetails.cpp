// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_PlayLevelSequenceDetails.h"
#include "Nodes/Actor/FlowNode_PlayLevelSequence.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FFlowNode_PlayLevelSequenceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& SequenceCategory = DetailBuilder.EditCategory("Sequence");
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, Sequence));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, PlaybackSettings));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, bPlayReverse));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, CameraSettings));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, bUseGraphOwnerAsTransformOrigin));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, bReplicates));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, bAlwaysRelevant));
	SequenceCategory.AddProperty(GET_MEMBER_NAME_CHECKED(UFlowNode_PlayLevelSequence, bApplyOwnerTimeDilation));
}
