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
			"ApplicationCore",
			"GameInputBase",
			"UMG",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"NetCore",
			"Sockets",
			"AIModule",
			"AnimationCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"EngineSettings"
		});
	}
}
