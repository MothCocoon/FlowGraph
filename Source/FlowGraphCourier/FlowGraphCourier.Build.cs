using UnrealBuildTool;

public class FlowGraphCourier : ModuleRules
{
	public FlowGraphCourier(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Flow",
				"FlowEditor",
				"ToolsetRegistry"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AssetRegistry",
				"Json",
				"JsonUtilities",
				"Projects",
				"SourceControl"
			}
		);
	}
}
