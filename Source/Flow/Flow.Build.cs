// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
		[
			"LevelSequence"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"GameplayAbilities", // for FGameplayTagRequirements
			"GameplayTags",
			"MovieScene",
			"MovieSceneTracks",
			"NetCore",
			"Slate",
			"SlateCore"
		]);

		if (target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(
			[
				"GraphEditor",
				"MessageLog",
				"PropertyEditor",
				"SourceControl",
				"UnrealEd"
			]);
		}
	}
}