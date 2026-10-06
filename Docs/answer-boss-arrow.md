# 답안지 — 보스 화면 밖 화살표 (오프스크린 인디케이터)

흐름: 보스 월드 위치 → 화면 좌표 → 화면 안이면 숨김 / 밖이면 화면 가장자리에 붙이고 보스 방향으로 회전
- 보스 전투 중(StartBossEncounter ~ EndBossEncounter)에만 동작
- 화살표 이미지는 **오른쪽(→)을 향하게** 그린 것 사용 (각도 0 = 오른쪽)

---

## 1. 선언 — MyCanvas.h

### 1-1. protected: (`BossHPBar` 아래)

```cpp
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BossArrow;

    // 화면 가장자리에서 띄울 거리
    UPROPERTY(EditAnywhere, Category = "Boss|Arrow")
    float ArrowEdgeMargin = 60.0f;
```

### 1-2. private: (클래스 맨 아래 `};` 위에 새로 추가)

```cpp
private:
    TWeakObjectPtr<ACombatCharacter> ArrowBoss;

    void UpdateBossArrow();
```

## 2. 숨기고 시작 — MyCanvas.cpp:35 (NativeConstruct, 조이스틱 처리 아래)

```cpp
    if (BossArrow)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
    }
```

## 3. 보스 기억 / 해제 — MyCanvas.cpp (Start/EndBossEncounter 교체)

```cpp
void UMyCanvas::StartBossEncounter(ACombatCharacter* Boss, const FText& BossName)
{
    ArrowBoss = Boss;

    if (BossHPBar)
    {
        BossHPBar->StartEncounter(Boss, BossName);
    }
}

void UMyCanvas::EndBossEncounter(ACombatCharacter* Boss)
{
    if (ArrowBoss.Get() == Boss)
    {
        ArrowBoss.Reset();
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
```

## 4. Tick + 화살표 계산 — MyCanvas.cpp 맨 아래에 추가

```cpp
void UMyCanvas::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateBossArrow();
}

// [보스 화살표]
void UMyCanvas::UpdateBossArrow()
{
    if (!BossArrow)
    {
        return;
    }

    ACombatCharacter* Boss = ArrowBoss.Get();
    APlayerController* PC = GetOwningPlayer();
    if (!Boss || !PC)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 픽셀 → UMG 단위
    const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
    const FVector2D Center = Viewport * 0.5f;

    FVector2D Screen;
    const bool bInFront = UGameplayStatics::ProjectWorldToScreen(PC, Boss->GetActorLocation(), Screen);
    Screen /= Scale;

    const bool bOnScreen = bInFront
        && Screen.X >= 0.0f && Screen.X <= Viewport.X
        && Screen.Y >= 0.0f && Screen.Y <= Viewport.Y;

    if (bOnScreen)
    {
        BossArrow->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    FVector2D Dir = Screen - Center;
    if (!bInFront)
    {
        Dir = -Dir;
    }
    if (Dir.IsNearlyZero())
    {
        return;
    }

    // 중앙에서 Dir 방향으로 화면 가장자리(여백 포함)까지
    const FVector2D Half = Center - FVector2D(ArrowEdgeMargin, ArrowEdgeMargin);
    const float T = FMath::Min(
        Half.X / FMath::Max(FMath::Abs(Dir.X), KINDA_SMALL_NUMBER),
        Half.Y / FMath::Max(FMath::Abs(Dir.Y), KINDA_SMALL_NUMBER));

    if (UCanvasPanelSlot* ArrowSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(BossArrow))
    {
        ArrowSlot->SetPosition(Center + Dir * T);
    }

    BossArrow->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)));
    BossArrow->SetVisibility(ESlateVisibility::HitTestInvisible);
}
```

## 5. BP_Canvas 설정

- 루트 CanvasPanel 바로 아래에 Image 추가 → 이름 **BossArrow** (정확히)
- Anchors: **왼쪽 위 (0,0)** — SetPosition이 앵커 기준이라 다른 앵커면 위치가 밀림
- Alignment: **(0.5, 0.5)** — 화살표 중심이 위치점
- Size: 48 x 48 정도, Brush에 → 방향 화살표 텍스처
- Render Transform Pivot: (0.5, 0.5) (기본값) — 제자리 회전

## 6. 빌드

- 헤더 변경 → **에디터 끄고** VS 빌드
- 테스트: 보스 전투 시작 후 멀어지거나 카메라 밖으로 보스를 빼서 확인
