using UnrealBuildTool;

public class FlowEditor : ModuleRules
{
    public FlowEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "ApplicationCore",
                "BlueprintGraph",
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
            new[]
            {
                "KismetWidgets",
                "Projects",
                "ToolMenus"
            }
        );
    }
}