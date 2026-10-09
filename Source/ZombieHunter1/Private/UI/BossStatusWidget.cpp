

#include "UI/BossStatusWidget.h"
#include "UI/HPBar.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/CombatCharacter.h"


void UBossStatusWidget::StartEncounter(ACombatCharacter* InBoss, const FText& InBossName)
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
	BossNameText->SetText(InBossName);

	HPBar->ResetIntro();
	HPBar->SetPercent(GetBossPercent());
	ResetDamage();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// 보스가 죽는 경우
void UBossStatusWidget::EndEncounter(ACombatCharacter* InBoss)
{
	// 다른 보스면 무시
	if (InBoss && Boss.Get() != InBoss)
	{
		return;
	}

	UnbindBoss();
	ResetDamage();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBossStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	DamageText->SetVisibility(ESlateVisibility::Collapsed);
}

void UBossStatusWidget::NativeDestruct()
{
	UnbindBoss();
	Super::NativeDestruct();
}

void UBossStatusWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (DamageRemainTime > 0.0f)
	{
		DamageRemainTime -= InDeltaTime;
		if (DamageRemainTime <= 0.0f)
		{
			ResetDamage();
		}
	}
}

// -- 
// [Start]

void UBossStatusWidget::BindBoss(ACombatCharacter* InBoss)
{
	Boss = InBoss;
	HPChangedHandle = InBoss->OnHPChanged.AddUObject(this, &UBossStatusWidget::HandleHPChanged);
}


void UBossStatusWidget::UnbindBoss()
{
	if (ACombatCharacter* B = Boss.Get())
	{
		B->OnHPChanged.Remove(HPChangedHandle);
	}
	HPChangedHandle.Reset();
	Boss.Reset();
}

void UBossStatusWidget::HandleHPChanged(int32 NewHP, int32 Delta)
{
	//퍼센트
	HPBar->SetPercent(GetBossPercent());
	// 만약 데미지를 받았다면
	if (Delta < 0)
	{
		AccumDamage -= Delta;
		DamageRemainTime = DamageShowTime;
		DamageText->SetText(FText::AsNumber(AccumDamage));
		DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}


// 중첩 데미지 초기화
void UBossStatusWidget::ResetDamage()
{
	AccumDamage = 0;
	DamageRemainTime = 0;

	// 데미지는 숨겨
	if (DamageText)
	{
		DamageText->SetVisibility(ESlateVisibility::Collapsed);
	}

}


//Percent Bar는 float단위라 변환기가 필요함
float UBossStatusWidget::GetBossPercent() const
{
	ACombatCharacter* TargetBoss = Boss.Get();
	if (!TargetBoss || TargetBoss->GetMaxHP() <= 0)
	{
		return 0.0f;
	}
	// 범위 제한?
	return FMath::Clamp((float)TargetBoss->GetHP() / (float)TargetBoss->GetMaxHP(), 0.0f, 1.0f);
}
