// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

using UnrealBuildTool;

public class FlowDebugger : ModuleRules
{
	public FlowDebugger(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Flow"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"Slate",
			"SlateCore",
		});
	}
}