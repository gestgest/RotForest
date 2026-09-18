// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MyCanvas.h"
#include "UI/DeathPanelWidget.h" //사망 패널 켜고 끄기 (SetVisibility에 완전한 타입 필요)
#include "UI/VirtualJoystick.h"
#include "Engine/Engine.h" //GEngine 화면 디버그

void UMyCanvas::NativeConstruct()
{
    Super::NativeConstruct();

    // 사망 패널은 기본 숨김. 버튼 배선은 패널 자신(UDeathPanelWidget)이 한다.
    // (BP_Canvas에 DeathPanel을 아직 안 배치했으면 null — 조용히 건너뛴다)
    if (DeathPanel)
    {
        DeathPanel->SetVisibility(ESlateVisibility::Collapsed);
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
    if (hp_bar)
    {
        UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(hp_bar);
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
