

#include "UI/BossStatusWidget.h"
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

	TargetPercent = GetBossPercent();
	HPBar->SetPercent(0.0f);
	DelayBar->SetPercent(0.0f);
	CurrentPercent = 0.0f;
	DelayPercent = 0.0f;
	IntroCap = 0.0f;

	ResetDamage();
	HoldDelayBarRemainTime = 0.0f;
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
	TickBar(InDeltaTime);
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

void UBossStatusWidget::TickBar(float DeltaTime)
{
	// 페이드는 안 넣었음

	// HP 차는 애니메이션
	// 줄어드는 퍼센트 선형 애니메이션
	IntroCap = FMath::FInterpConstantTo(IntroCap, 1.0f, DeltaTime, 1.0f / FMath::Max(BarFadeTime, 0.01f));
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

	HPBar->SetPercent(CurrentPercent);
	DelayBar->SetPercent(DelayPercent);


	// [데미지 텍스트] 데미지를 받은 경우
	if (DamageRemainTime > 0.0f)
	{
		DamageRemainTime -= DeltaTime;
		if (DamageRemainTime <= 0.0f)
		{
			ResetDamage();
		}
	}
}

void UBossStatusWidget::HandleHPChanged(int32 NewHP, int32 Delta)
{
	//퍼센트
	TargetPercent = GetBossPercent();

	// 만약 데미지를 받았다면
	if (Delta < 0)
	{
		AccumDamage -= Delta;
		DamageRemainTime = DamageShowTime;
		HoldDelayBarRemainTime = HoldDelayBarTime;
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
