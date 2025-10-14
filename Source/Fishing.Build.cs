
using UnrealBuildTool;


public class Fishing : ModuleRules
{
	public Fishing(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Niagara",
			"Water"
			
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicIncludePaths.AddRange(new string[] {
			"Fishing",
			"Fishing/Variant_Platforming",
			"Fishing/Variant_Combat",
			"Fishing/Variant_Combat/AI",
		});
		
	}
}

