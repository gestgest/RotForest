// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HPBar.generated.h"

class UProgressBar;

/**
 * 
 */
UCLASS()
class ZOMBIEHUNTER1_API UHPBar : public UUserWidget
{
	GENERATED_BODY()


protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* FillHPBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DelayBar;

protected:
	// [변수]
	// 노란바가 버티는 시간
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Bar")
	float HoldDelayBarTime = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Bar")
	float BarIntroTime = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Bar")
	float HealFillSpeed = 0.4f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Bar")
	float DelayDrainSpeed = 0.4f;


private:
	void TickBar(float DeltaTime);

	float TargetPercent = 0.0f;
	float CurrentPercent = 0.0f; // 빨간 바
	float DelayPercent = 0.0f; // 노란바 

	float IntroCap = 0.0f;

	float HoldDelayBarRemainTime = 0.0f;

public:
	void SetPercent(float Percent);
	void ResetIntro();
};
