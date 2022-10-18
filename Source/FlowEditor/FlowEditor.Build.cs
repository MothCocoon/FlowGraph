// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

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
            "AssetSearch",
            "AssetTools",
            "BlueprintGraph",
            "ClassViewer",
            "ContentBrowser",
            "Core",
            "CoreUObject",
            "DetailCustomizations",
            "DeveloperSettings",
            "EditorStyle",
            "Engine",
            "GraphEditor",
            "InputCore",
            "Json",
            "JsonUtilities",
            "KismetWidgets",
            "LevelEditor",
            "LevelSequence",
            "MovieScene",
            "MovieSceneTracks",
            "MovieSceneTools",
            "Projects",
            "PropertyEditor",
            "RenderCore",
            "Sequencer",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd"
        });
    }
}
