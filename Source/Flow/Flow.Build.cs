using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
                "Slate",
                "SlateCore"
            }
        );

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("FlowEditor");
        }
    }
}