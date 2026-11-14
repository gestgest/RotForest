// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillDataAsset.generated.h"

class UTexture2D;
class USoundBase;

UCLASS()
class ZOMBIEHUNTER1_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta= (MultiLine = "true"))
	FText Description; // 설명

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	USoundBase* Sound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	int32 UnlockLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float Cooldown;
};
