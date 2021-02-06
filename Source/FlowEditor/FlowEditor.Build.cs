using UnrealBuildTool;

public class FlowEditor : ModuleRules
{
    public FlowEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Flow"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ApplicationCore",
            "AssetTools",
            "BlueprintGraph",
            "ClassViewer",
            "ContentBrowser",
            "Core",
            "CoreUObject",
            "DetailCustomizations",
            "EditorStyle",
            "Engine",
            "GraphEditor",
            "InputCore",
            "KismetWidgets",
            "LevelEditor",
            "MovieScene",
            "MovieSceneTracks",
            "MovieSceneTools",
            "Projects",
            "PropertyEditor",
            "RenderCore",
            "Sequencer",
            "Slate",
            "SlateCore",
            "UnrealEd"
        });
    }
}
