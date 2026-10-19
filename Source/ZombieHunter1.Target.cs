// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ZombieHunter1Target : TargetRules
{
	public ZombieHunter1Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game; 
		DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ZombieHunter1");
	}
}
