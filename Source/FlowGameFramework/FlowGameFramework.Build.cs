// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
using UnrealBuildTool;

public class FlowGameFramework : ModuleRules
{
	public FlowGameFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
		[
			"Core",
			"CoreUObject",
			"Engine",
			"Flow",
			"NavigationSystem",
		]);

		PrivateDependencyModuleNames.Add("DeveloperSettings");
	}
}
