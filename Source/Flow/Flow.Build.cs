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
			"NetCore",
			"Slate",
			"SlateCore"
		});

		if (target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new[]
			{
				"GraphEditor",
				"MessageLog",
				"PropertyEditor",
				"SourceControl",
				"UnrealEd"
			});
		}
	}
}