// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossStatusWidget.generated.h"

class ACombatCharacter;
class UTextBlock;
class UHPBar;

UCLASS()
class ZOMBIEHUNTER1_API UBossStatusWidget : public UUserWidget
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
	UTextBlock* BossNameText;

	// 보스가 죽은 뒤 잠시 붙잡는 용도
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "HPBar")
	float DeathHoldTime = 2.0f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Damage")
	float DamageShowTime = 2.0f;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

	UPROPERTY(meta = (BindWidget))
	UHPBar* HPBar;

private:

	int32 AccumDamage = 0;

	float DamageRemainTime = 0.0f;


	TWeakObjectPtr<ACombatCharacter> Boss;

	void UnbindBoss();
	void BindBoss(ACombatCharacter * InBoss);

	FDelegateHandle HPChangedHandle;
	void HandleHPChanged(int32 NewHP, int32 Delta);

	void ResetDamage();
	float GetBossPercent() const;
};
