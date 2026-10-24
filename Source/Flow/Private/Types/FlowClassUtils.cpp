// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Types/FlowClassUtils.h"

#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
TArray<UClass*> FlowClassUtils::GetClassesFromMetadataString(const FString& MetadataString)
{
	// Adapted from the inaccessible PropertyCustomizationHelpers::GetClassesFromMetadataString

	if (MetadataString.IsEmpty())
	{
		return TArray<UClass*>();
	}

	auto FindClass = [](const FString& InClassName) -> UClass*
		{
			UClass* Class = UClass::TryFindTypeSlow<UClass>(InClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
			if (!Class)
			{
				Class = LoadObject<UClass>(nullptr, *InClassName);
			}
			return Class;
		};

	TArray<FString> ClassNames;
	MetadataString.ParseIntoArrayWS(ClassNames, TEXT(","), true);

	TArray<UClass*> Classes;
	Classes.Reserve(ClassNames.Num());

	for (const FString& ClassName : ClassNames)
	{
		UClass* Class = FindClass(ClassName);
		if (!Class)
		{
			continue;
		}

		// If the class is an interface, expand it to be all classes in memory that implement the class.
		if (Class->HasAnyClassFlags(CLASS_Interface))
		{
			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* ClassWithInterface = (*ClassIt);
				if (ClassWithInterface->ImplementsInterface(Class))
				{
					Classes.Add(ClassWithInterface);
				}
			}
		}
		else
		{
			Classes.Add(Class);
		}
	}

	return Classes;
}
#endif