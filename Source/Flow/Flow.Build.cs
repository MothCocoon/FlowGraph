// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[] 
		{
			"LevelSequence"
		});
		
		PrivateDependencyModuleNames.AddRange(new[] 
		{
            "Core",
			"CoreUObject",
            "DeveloperSettings",
			"Engine",
            "GameplayTags",
			"MovieScene",
			"MovieSceneTracks",
            "Slate",
            "SlateCore"
        });

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("UnrealEd");
        }
    }
}
