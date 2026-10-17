// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FinalBell : ModuleRules
{
	public FinalBell(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"NetCore",
			"AIModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
