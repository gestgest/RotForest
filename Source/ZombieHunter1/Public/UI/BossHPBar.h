// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHPBar.generated.h"

class ACombatCharacter;
class UTextBlock;
class UProgressBar;

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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HPBar;


	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

private:
	float TargetPercent = 0.0f;

	int32 AccumDamage = 0;

	TWeakObjectPtr<ACombatCharacter> Boss;
	FDelegateHandle HPChangedHandle;

	void UnbindBoss();
	void BindBoss(ACombatCharacter * InBoss);

	void TickBar(float DeltaTime);

	void HandleHPChanged(int32 NewHP, int32 Delta);
	float GetBossPercent() const;
};
