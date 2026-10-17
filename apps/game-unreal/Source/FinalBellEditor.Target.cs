// Copyright Epic Games, Inc. All Rights Reserved.
// Final Bell — editor target

using UnrealBuildTool;
using System.Collections.Generic;

public class FinalBellEditorTarget : TargetRules
{
	public FinalBellEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("FinalBell");
	}
}
