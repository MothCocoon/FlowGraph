// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

using UnrealBuildTool;

public class FlowEditor : ModuleRules
{
	public FlowEditor(ReadOnlyTargetRules target) : base(target)
	{
		if (CppStandard is null || CppStandard != CppStandardVersion.Cpp20)
		{
			CppStandard = CppStandardVersion.Cpp20;
		}

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"AssetSearch",
			"EditorSubsystem",
			"Flow",
			"MessageLog",
			"AIModule", // For BlueprintNodeHelpers::DescribeProperty (could be copy/pasted out to remove editor-only dependency)
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"ApplicationCore",
			"AssetDefinition",
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
			"Engine",
			"GraphEditor",
			"GameplayTags",
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
			"StructUtils",
			"ToolMenus",
			"UnrealEd"
		});
	}
}