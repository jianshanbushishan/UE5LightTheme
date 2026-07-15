using UnrealBuildTool;

public class LightThemeFixEditor : ModuleRules
{
	public LightThemeFixEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"GraphEditor",
			"Projects",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}
