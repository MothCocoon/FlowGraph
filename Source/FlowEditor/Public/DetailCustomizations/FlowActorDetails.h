// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "ActorDetailsDelegates.h"

/**
 * 
 */
class FLOWEDITOR_API FFlowActorDetails : public TSharedFromThis<FFlowActorDetails>
{
public:	
	virtual ~FFlowActorDetails();
	virtual void Register();
	virtual void AddFlowCategory(class IDetailLayoutBuilder& Details, const FGetSelectedActors& GetSelectedActors);
	
	static TArray<class UFlowComponent*> GetSelectedFlowComponents(const FGetSelectedActors& GetSelectedActors);
};
