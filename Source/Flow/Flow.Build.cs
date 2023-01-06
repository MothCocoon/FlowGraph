// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules target) : base(target)
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

		if (target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new[]
			{
				"MessageLog",
				"UnrealEd"
			});
		}
	}
}