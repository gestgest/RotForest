// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JobTypes.generated.h"

// 직업 종류. 이 값 하나로 직업을 식별한다.
UENUM(BlueprintType)
enum class EJobType : uint8
{
	Warrior UMETA(DisplayName = "전사"),
	Archer UMETA(DisplayName = "궁수"),
	Mage UMETA(DisplayName = "마법사"),
	Healer UMETA(DisplayName = "힐러"),
};

