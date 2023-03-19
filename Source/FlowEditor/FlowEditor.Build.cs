// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

using UnrealBuildTool;

public class FlowEditor : ModuleRules
{
	public FlowEditor(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Flow",
			"MessageLog"
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
			"EditorFramework",
			"EditorScriptingUtilities",
			"EditorStyle",
			"EditorSubsystem",
			"Engine",
			"GraphEditor",
			"InputCore",
			"Json",
			"JsonUtilities",
			"Kismet",
			"KismetWidgets",
			"LevelEditor",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTools",
			"MovieSceneTracks",
			"Projects",
			"PropertyEditor",
			"PropertyPath",
			"RenderCore",
			"Sequencer",
			"Slate",
			"SlateCore",
			"SourceControl",
			"ToolMenus",
			"UnrealEd"
		});
	}
}