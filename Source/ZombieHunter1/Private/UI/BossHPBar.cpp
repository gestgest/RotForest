

#include "UI/BossHPBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/CombatCharacter.h"


void UBossHPBar::StartEncounter(ACombatCharacter* InBoss, const FText& InBossName)
{
	// 보스가 죽었다면
	if (!InBoss || InBoss->GetIsDead())
	{
		return;
	}

	
	// 보스가 바뀌었다면 => 체인지
	const bool bSameBoss = (Boss.Get() == InBoss);

	// 바가 진행중이라면 
	if (bSameBoss)
	{
		return;
	}
	
	//!bSameBoss
	UnbindBoss();
	BindBoss(InBoss);

	TargetPercent = GetBossPercent();
	HPBar->SetPercent(TargetPercent);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBossHPBar::EndEncounter(ACombatCharacter* InBoss)
{
}

void UBossHPBar::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	DamageText->SetVisibility(ESlateVisibility::Collapsed);
}

void UBossHPBar::NativeDestruct()
{
	UnbindBoss();
	Super::NativeDestruct();
}

void UBossHPBar::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

// -- 
// [Start]

void UBossHPBar::BindBoss(ACombatCharacter* InBoss)
{
	Boss = InBoss;
	HPChangedHandle = InBoss->OnHPChanged.AddUObject(this, &UBossHPBar::HandleHPChanged);
}


void UBossHPBar::UnbindBoss()
{
	if (ACombatCharacter* B = Boss.Get())
	{
		B->OnHPChanged.Remove(HPChangedHandle);
	}
	HPChangedHandle.Reset();
	Boss.Reset();
}

void UBossHPBar::TickBar(float DeltaTime)
{
	// todo 페이드

	// todo HP 차는 애니메이션

}

void UBossHPBar::HandleHPChanged(int32 NewHP, int32 Delta)
{
	//퍼센트
	TargetPercent = GetBossPercent();
	HPBar->SetPercent(TargetPercent);

	// 만약 데미지를 받았다면
	if (Delta < 0)
	{
		AccumDamage -= Delta;
		//DamageRemain = DamgeShowTime;
		//DelayHoldRemain = DelayHoldTime;
		DamageText->SetText(FText::AsNumber(AccumDamage));
		DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}


//Percent Bar는 float단위라 변환기가 필요함
float UBossHPBar::GetBossPercent() const
{
	ACombatCharacter* TargetBoss = Boss.Get();
	if (!TargetBoss || TargetBoss->GetMaxHP() <= 0)
	{
		return 0.0f;
	}
	// 범위 제한?
	return FMath::Clamp((float)TargetBoss->GetHP() / (float)TargetBoss->GetMaxHP(), 0.0f, 1.0f);
}
