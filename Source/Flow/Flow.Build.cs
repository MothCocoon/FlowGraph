using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules Target) : base(Target)
	{		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
			}
		);

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("FlowEditor");
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}
