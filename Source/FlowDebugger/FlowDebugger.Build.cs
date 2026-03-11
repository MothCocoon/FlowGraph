// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
using UnrealBuildTool;

public class FlowDebugger : ModuleRules
{
	public FlowDebugger(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
		[
			"Flow"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"Slate",
			"SlateCore",
		]);
	}
}