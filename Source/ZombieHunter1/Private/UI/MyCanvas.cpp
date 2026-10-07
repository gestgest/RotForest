// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MyCanvas.h"
#include "UI/DeathPanelWidget.h" 
#include "UI/ExitPanelWidget.h"
#include "UI/VirtualJoystick.h"
#include "UI/BossStatusWidget.h"

#include "Characters/Boss.h"

#include "kismet/GameplayStatics.h"
#include "Engine/Engine.h" //GEngine 화면 디버그

void UMyCanvas::NativeConstruct()
{
    Super::NativeConstruct();

    // Deathpanel, ExitPanel 미리 숨기기
    if (DeathPanel)
    {
        DeathPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (ExitPanel)
    {
        ExitPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (BossArrow)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 모바일(안드로이드/iOS)에서만 터치 조이스틱 표시, PC에선 숨김.
    // 위젯이 만들어질 때 자동 실행되므로 BP가 SetCanvasWidget을 호출하든 말든 항상 적용됨.
#if PLATFORM_ANDROID || PLATFORM_IOS
    const ESlateVisibility JoystickVis = ESlateVisibility::Visible;
#else
    const ESlateVisibility JoystickVis = ESlateVisibility::Collapsed;
#endif
    if (MoveJoystick) { MoveJoystick->SetVisibility(JoystickVis); }
    if (AimJoystick)  { AimJoystick->SetVisibility(JoystickVis); }

}

void UMyCanvas::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateBossArrow();
}



void UMyCanvas::UpdateCoinText(int32 Money)
{
    if (CoinText)
    {
        FText MoneyText = FText::Format(
            FText::FromString(TEXT("{0} 원")),
            FText::AsNumber(Money)
        );
        CoinText->SetText(MoneyText);
    }
}

void UMyCanvas::UpdateExp(int32 Level, int32 Exp, int32 ExpToNext)
{
    // 위젯은 옵셔널(BindWidgetOptional) — BP_Canvas에 아직 안 배치했으면 조용히 넘어간다.
    if (ExpText)
    {
        ExpText->SetText(FText::FromString(
            FString::Printf(TEXT("Lv.%d  %d / %d"), Level, Exp, ExpToNext)));
    }
    if (ExpBar)
    {
        ExpBar->SetPercent(ExpToNext > 0 ? (float)Exp / (float)ExpToNext : 0.0f);
    }
}

void UMyCanvas::SetProgressUISize(FVector2D size)
{
    if (HPBar)
    {
        UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(HPBar);
        if (CanvasSlot)
        {
            CanvasSlot->SetSize(size);
            //UE_LOG(LogTemp, Log, TEXT("hp_bar size set to: %s"), *size.ToString());
        }
    }
}

// 사망 패널 표시/숨김 — 패널 위젯 하나만 토글하면 안의 텍스트/버튼이 전부 따라간다.
void UMyCanvas::ShowDeathPanel(bool bShow)
{
    if (DeathPanel)
    {
        DeathPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UMyCanvas::ToggleExitPanel()
{
    if (!ExitPanel)
    {
        return;
    }
    const bool bOpen = ExitPanel->GetVisibility() == ESlateVisibility::Collapsed; // 비활성화라면
    ExitPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    UGameplayStatics::SetGamePaused(this, bOpen); //열었다면 멈췃?
}



namespace
{
	// 종류별 글자색. 여기만 고치면 전체가 바뀐다.
	FLinearColor GetItemNotifyColor(EItemNotifyType Type)
	{
		switch (Type)
		{
		case EItemNotifyType::Blocked:   return FLinearColor(1.0f, 0.0f, 0.0f);	// 빨간 - 뭐 안된다는 내용
		case EItemNotifyType::Companion: return FLinearColor(0.45f, 0.8f, 1.0f);	// 하늘 - 주체가 동료
		default:                         return FLinearColor::White;				// 획득
		}
	}
}

void UMyCanvas::AddItemNotification(const FText& Text, EItemNotifyType Type)
{
    if (!Vertical_ItemTextBox)
    {
        return;
    }
    

    UTextBlock* NewText = NewObject<UTextBlock>(this);
    NewText->SetText(Text);
    NewText->SetColorAndOpacity(FSlateColor(GetItemNotifyColor(Type)));
    NewText->SetRenderTransformAngle(180.0f);   // 부모 박스가 뒤집혀 있어서 되돌리기
    Vertical_ItemTextBox->AddChildToVerticalBox(NewText);

    //5초후에 제거 느낌
    FTimerHandle TempHandle;
    FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UMyCanvas::RemoveItemNotification);
    GetWorld()->GetTimerManager().SetTimer(TempHandle, Delegate, 2.0f, false);
}

void UMyCanvas::RemoveItemNotification()
{
    if (Vertical_ItemTextBox && Vertical_ItemTextBox->GetChildrenCount() > 0)
    {
        Vertical_ItemTextBox->RemoveChildAt(0);
    }
}

void UMyCanvas::StartBossEncounter(ACombatCharacter* Boss, const FText& BossName)
{
    TargetBoss = Boss;

    if (BossHPBar)
    {
        BossHPBar->StartEncounter(Boss, BossName);
    }
}

void UMyCanvas::EndBossEncounter(ACombatCharacter* Boss)
{
    if (TargetBoss.Get() == Boss)
    {
        TargetBoss.Reset();
        if (BossArrow)
        {
            BossArrow->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (BossHPBar)
    {
        BossHPBar->EndEncounter(Boss);
    }
}

// 화살표 tick 함수
void UMyCanvas::UpdateBossArrow()
{
    if (!BossArrow)
    {
        return;
    }

    ACombatCharacter* Boss = TargetBoss.Get();
    APlayerController* PC = GetOwningPlayer(); // 2d 스크린 정보 가져오는 용도

    if (!Boss || !PC)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 화면 관련 코드
    const float Scale = UWidgetLayoutLibrary::GetViewportScale(this); // DPI => 대충 1920 x 1080이면 1.0 반환 (1080p 기준)
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
    const FVector2D Center = Viewport * 0.5f;

    FVector2D Screen = FVector2D::ZeroVector;;
    const bool bInFront = UGameplayStatics::ProjectWorldToScreen(PC, Boss->GetActorLocation(), Screen);

    if (!bInFront)
    {
        return;
    }
    Screen /= Scale;

    const bool bOnScreen = bInFront
        && Screen.X >= 0.0f && Screen.X <= Viewport.X
        && Screen.Y >= 0.0f && Screen.Y <= Viewport.Y;

    // 화면안에 보스가 있다면
    if (bOnScreen)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 방향
    FVector2D Dir = Screen - Center; // 나눗셈 제로 오류 방지
    if (Dir.IsNearlyZero())
    {
        return;
    }

    // 보스를 향한 화살표 벡터
    const FVector2D Half = Center - FVector2D(BossArrowEdgeMargin, BossArrowEdgeMargin);

    // 위, 오른쪽 벽면중에 가장 가까운 벽을 찾기
    // KINDA_SMALL_NUMBER => 0.000000000
    const float T = FMath::Min(
        Half.X / FMath::Max(FMath::Abs(Dir.X), KINDA_SMALL_NUMBER),
        Half.Y / FMath::Max(FMath::Abs(Dir.Y), KINDA_SMALL_NUMBER)
    );

    if (UCanvasPanelSlot* ArrowSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(BossArrow))
    {
        ArrowSlot->SetPosition(Center + Dir * T);
    }


    // 방향 표시
    BossArrow->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)));
    BossArrow->SetVisibility(ESlateVisibility::HitTestInvisible);

}
