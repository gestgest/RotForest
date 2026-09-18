// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/VerticalBox.h"
#include "MyCanvas.generated.h"

class UVirtualJoystick;
class UDeathPanelWidget;

// 알림 메시지 종류. 색은 UMyCanvas가 정한다 — 호출부는 "무슨 일인지"만 넘긴다.
UENUM(BlueprintType)
enum class EItemNotifyType : uint8
{
	Gain,		// 획득
	Blocked,	// 장착 불가 (직업 불일치 등)
	Companion,	// 동료가 대신 장착
};

UCLASS()
class ZOMBIEHUNTER1_API UMyCanvas : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:

    //변수들
    // CoinText와 자동으로 바인딩됨
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CoinText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* hp_bar;

    // BP_Canvas에 배치한 조이스틱 인스턴스. 이름이 MoveJoystick / AimJoystick 이어야 자동 연결됨
    UPROPERTY(meta = (BindWidgetOptional))
    UVirtualJoystick* MoveJoystick;

    UPROPERTY(meta = (BindWidgetOptional))
    UVirtualJoystick* AimJoystick;


    UPROPERTY(meta = (BindWidgetOptional))
    UVerticalBox* Vertical_ItemTextBox;
    

    // 경험치 표시 (선택) — BP_Canvas에 이 이름으로 배치하면 자동 연결, 없어도 컴파일에 지장 없음.
    // ExpText: "Lv.3  12 / 20" 형식 텍스트, ExpBar: 다음 레벨까지 진행도(0~1)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* ExpText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UProgressBar* ExpBar;

    //////////////////////////////////////////////////////////////////////////
    // 사망 패널 — 별도 위젯 WBP_DeathPanel(부모: UDeathPanelWidget)을
    // BP_Canvas에 "DeathPanel"이라는 이름으로 배치하면 자동 연결된다(없어도 컴파일/실행 지장 없음).
    // 여기(MyCanvas)는 켜고 끄기만 하고, 버튼 로직은 UDeathPanelWidget이 자체 처리한다.

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UDeathPanelWidget* DeathPanel;


    //BlueprintCallable 함수들

    /** 사망 패널 표시/숨김. 플레이어 OnDeath/OnRevive(및 SetHP 동기화)가 호출한다.
     *  DeathPanel이 BP에 아직 없으면 조용히 넘어간다. */
    UFUNCTION(BlueprintCallable)
    void ShowDeathPanel(bool bShow);


    void UpdateCoinText(int32 Money);
    void SetProgressUISize(FVector2D size);

    /** 경험치 HUD 갱신 — AMyPlayer::UpdateExpUI가 호출. 위젯이 배치돼 있을 때만 그린다. */
    void UpdateExp(int32 Level, int32 Exp, int32 ExpToNext);


    void AddItemNotification(const FText& Text, EItemNotifyType Type = EItemNotifyType::Gain);
    void RemoveItemNotification();
};
