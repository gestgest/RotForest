// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossStatusWidget.generated.h"

class ACombatCharacter;
class UTextBlock;
class UProgressBar;

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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HPBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DelayBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

private:
	// [변수]
	// 노란바가 버티는 시간
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Bar")
	float HoldDelayBarTime = 0.4f;

	// 보스가 죽은 뒤 잠시 붙잡는 용도
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Bar")
	float DeathHoldTime = 2.0f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Bar")
	float BarFadeTime = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Bar")
	float HealFillSpeed = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Bar")
	float DelayDrainSpeed = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Boss|Damage")
	float DamageShowTime = 2.0f;


	float TargetPercent = 0.0f;
	float CurrentPercent = 0.0f; // 빨간 바
	float DelayPercent = 0.0f; // 노란바 
	int32 AccumDamage = 0;

	// tick에서 HP 값 목표치
	float IntroCap = 0.0f;
	float DamageRemainTime = 0.0f;
	float HoldDelayBarRemainTime = 0.0f;



	TWeakObjectPtr<ACombatCharacter> Boss;
	FDelegateHandle HPChangedHandle;

	void UnbindBoss();
	void BindBoss(ACombatCharacter * InBoss);

	void TickBar(float DeltaTime);

	void HandleHPChanged(int32 NewHP, int32 Delta);
	void ResetDamage();
	float GetBossPercent() const;
};
