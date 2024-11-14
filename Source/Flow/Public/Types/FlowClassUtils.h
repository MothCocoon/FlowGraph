// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"

class FString;
class UClass;

#if WITH_EDITOR
namespace FlowClassUtils
{
	TArray<UClass*> GetClassesFromMetadataString(const FString& MetadataString);
}
#endif