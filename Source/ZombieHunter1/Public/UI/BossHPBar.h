// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHPBar.generated.h"

class ACombatCharacter;

UCLASS()
class ZOMBIEHUNTER1_API UBossHPBar : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartEncounter(ACombatCharacter* InBoss, const FText& InBossName); //보스전 시작
	void EndEncounter(ACombatCharacter* InBoss); //보스전 종료
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;
};
