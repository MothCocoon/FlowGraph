using UnrealBuildTool;

public class Flow : ModuleRules
{
	public Flow(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
                "GameplayTags"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new[] 
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