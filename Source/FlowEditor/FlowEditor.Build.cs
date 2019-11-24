using UnrealBuildTool;

public class FlowEditor : ModuleRules
{
    public FlowEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ApplicationCore",
                "ClassViewer",
                "ContentBrowser",
                "Core",
                "CoreUObject",
                "DetailCustomizations",
                "EditorStyle",
                "Engine",
                "Flow",
                "GraphEditor",
                "InputCore",
                "LevelEditor",
                "PropertyEditor",
                "RenderCore",
                "Slate",
                "SlateCore",
                "UnrealEd",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "ToolMenus",
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "AssetTools"
            }
        );

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}
