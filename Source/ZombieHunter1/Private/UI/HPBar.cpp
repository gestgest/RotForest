// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HPBar.h"
#include "Components/ProgressBar.h"


void UHPBar::NativeConstruct()
{
	Super::NativeConstruct();
	ResetIntro();
}

void UHPBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickBar(InDeltaTime);
}

void UHPBar::TickBar(float DeltaTime)
{
	// 페이드는 안 넣었음

	// HP 차는 애니메이션
	// 줄어드는 퍼센트 선형 애니메이션
	IntroCap = FMath::FInterpConstantTo(IntroCap, 1.0f, DeltaTime, 1.0f / FMath::Max(BarIntroTime, 0.01f));
	const float Goal = FMath::Min(TargetPercent, IntroCap); // 목표 퍼센트보다 떨어지는 거 방지

	// 빨간 HP바
	// 평범한 데미지받는 상황이거나 HP가 감소되는 중일경우
	if (Goal < CurrentPercent || IntroCap < 1.0f)
	{
		CurrentPercent = Goal;
	}
	else // 회복
	{
		CurrentPercent = FMath::FInterpConstantTo(CurrentPercent, Goal, DeltaTime, HealFillSpeed);
	}

	// 노란바 (delay)
	if (DelayPercent <= CurrentPercent) // 대체로 회복
	{
		DelayPercent = CurrentPercent;
	}
	else if (HoldDelayBarRemainTime > 0.0f) // 바로 맞으면 줄지 말고 대기
	{
		HoldDelayBarRemainTime -= DeltaTime;
	}
	else // 피격 바 감소
	{
		DelayPercent = FMath::FInterpConstantTo(DelayPercent, CurrentPercent, DeltaTime, DelayDrainSpeed);
	}

	FillHPBar->SetPercent(CurrentPercent);
	DelayBar->SetPercent(DelayPercent);


}


void UHPBar::ResetIntro()
{
	IntroCap = 0.0f;
	CurrentPercent = 0.0f;
	DelayPercent = 0.0f;
	HoldDelayBarRemainTime = 0.0f;
	FillHPBar->SetPercent(0.0f);
	DelayBar->SetPercent(0.0f);
}

void UHPBar::SetPercent(float Percent)
{
	const float NewPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
	if (NewPercent < TargetPercent)
	{
		HoldDelayBarRemainTime = HoldDelayBarTime;
	}
	TargetPercent = NewPercent;
}
