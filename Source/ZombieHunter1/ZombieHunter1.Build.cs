// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZombieHunter1 : ModuleRules
{
    public ZombieHunter1(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",        // 슬레이트 UI (FSlateBrush 등)
            "SlateCore",    // 슬레이트 코어
            "AIModule",
            "NavigationSystem",  // 경로 찾기 기능
            "Niagara"
        });
    }
}
