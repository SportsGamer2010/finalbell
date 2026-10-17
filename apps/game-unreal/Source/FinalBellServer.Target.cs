// Copyright Epic Games, Inc. All Rights Reserved.
// Final Bell — dedicated server target (official online fights; P4 packaging gate)

using UnrealBuildTool;
using System.Collections.Generic;

public class FinalBellServerTarget : TargetRules
{
	public FinalBellServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("FinalBell");
	}
}
