# 답안지 — 플레이어 HP바 차오름 애니메이션 (MyCanvas)

참고: `Docs/answer-bossHPbar.md` (BossStatusWidget 구현). 구조를 그대로 가져온다.

## 지금 상태 vs 목표
```
지금 : AMyPlayer::UpdateHPUI ─▶ MyCanvas::SetProgressUISize(FVector2D(HP*500/MaxHP, 50))
                                 └▶ 슬롯 Size 즉시 대입. 애니메이션 없음, 즉시 끊김

목표 : AMyPlayer::UpdateHPUI ─▶ MyCanvas::SetHPPercent(HP/MaxHP)  = 목표값만 저장
       MyCanvas::NativeTick  ─▶ TickHPBar(DeltaTime)
                                 ├ IntroCap  : HUD 뜰 때 0 → 현재 HP까지 차오름
                                 ├ hp_bar       (빨강) : 피해는 즉시, 회복은 천천히 차오름
                                 └ hp_bar_delay (노랑) : 피해 후 0.4초 버티고 따라 내려옴
```

BossStatusWidget과 다른 점은 **바를 ProgressBar가 아니라 UImage 슬롯 폭으로 그린다**는 것 하나뿐.
`SetPercent(P)` 자리에 `ApplyHPBarWidth(Bar, P)`가 들어간다.

순서: 1~3(헤더) → 4~8(cpp) → 9(플레이어 연결) → 10(에디터 작업)
**UPROPERTY가 추가되므로 Live Coding 금지, 풀 리빌드.**

---

## 1. 잔상 바 바인딩 추가 — MyCanvas.h:47 (hp_bar 바로 아래)
```cpp
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* hp_bar;

	// 잔상(노란) 바. 배치 안 해도 빨간 바만으로 동작
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* hp_bar_delay;
```

## 2. 튜닝값 — MyCanvas.h:84 (BossArrowEdgeMargin 아래)
```cpp
	// [플레이어 HP바]
	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPBarFullWidth = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPBarHeight = 50.0f;

	// 0 => 현재 HP까지 차오르는 시간
	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPIntroTime = 0.4f;

	// 회복 시 차오르는 속도 (퍼센트/초)
	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPHealFillSpeed = 0.4f;

	// 노란바가 버티는 시간
	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPHoldDelayBarTime = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Player|HPBar")
	float HPDelayDrainSpeed = 0.4f;
```

## 3. 상태 변수 + 함수 선언 — MyCanvas.h:98 / 112
`SetProgressUISize`를 `SetHPPercent`로 교체한다. public 선언:
```cpp
	// Before
	void SetProgressUISize(FVector2D size);
```
```cpp
	// After
	// 목표 퍼센트만 저장. 실제 그리기는 TickHPBar
	void SetHPPercent(float Percent);
```

private 블록(`TWeakObjectPtr<ACombatCharacter> TargetBoss;` 아래):
```cpp
	// [플레이어 HP바]
	float HPTargetPercent = 0.0f;
	float HPCurrentPercent = 0.0f; // 빨간바
	float HPDelayPercent = 0.0f;   // 노란바
	float HPIntroCap = 0.0f;
	float HPHoldDelayRemainTime = 0.0f;

	void TickHPBar(float DeltaTime);
	void ApplyHPBarWidth(UImage* Bar, float Percent);
```

---

## 4. NativeConstruct — 바를 0부터 — MyCanvas.cpp:40 (조이스틱 처리 아래)
```cpp
	HPIntroCap = 0.0f;
	HPCurrentPercent = 0.0f;
	HPDelayPercent = 0.0f;
	ApplyHPBarWidth(hp_bar, 0.0f);
	ApplyHPBarWidth(hp_bar_delay, 0.0f);
```

## 5. NativeTick — MyCanvas.cpp:44
```cpp
void UMyCanvas::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateBossArrow();
    TickHPBar(InDeltaTime);
}
```

## 6. SetHPPercent — MyCanvas.cpp:78 (SetProgressUISize 전체를 대체)
```cpp
void UMyCanvas::SetHPPercent(float Percent)
{
    const float NewTarget = FMath::Clamp(Percent, 0.0f, 1.0f);

    // 피해면 노란바를 잠시 붙잡는다
    if (NewTarget < HPTargetPercent)
    {
        HPHoldDelayRemainTime = HPHoldDelayBarTime;
    }

    HPTargetPercent = NewTarget;
}
```

## 7. TickHPBar — MyCanvas.cpp (SetHPPercent 아래에 새로 추가)
```cpp
void UMyCanvas::TickHPBar(float DeltaTime)
{
    // 처음 뜰 때 0 => 현재 HP까지 차오름
    HPIntroCap = FMath::FInterpConstantTo(HPIntroCap, 1.0f, DeltaTime, 1.0f / FMath::Max(HPIntroTime, 0.01f));
    const float Goal = FMath::Min(HPTargetPercent, HPIntroCap); // 목표보다 넘어가는 거 방지

    // 빨간바 : 피해는 즉시, 회복은 천천히
    if (Goal < HPCurrentPercent || HPIntroCap < 1.0f)
    {
        HPCurrentPercent = Goal;
    }
    else
    {
        HPCurrentPercent = FMath::FInterpConstantTo(HPCurrentPercent, Goal, DeltaTime, HPHealFillSpeed);
    }

    // 노란바
    if (HPDelayPercent <= HPCurrentPercent) // 회복
    {
        HPDelayPercent = HPCurrentPercent;
    }
    else if (HPHoldDelayRemainTime > 0.0f) // 버티는 중
    {
        HPHoldDelayRemainTime -= DeltaTime;
    }
    else // 피격 바 감소
    {
        HPDelayPercent = FMath::FInterpConstantTo(HPDelayPercent, HPCurrentPercent, DeltaTime, HPDelayDrainSpeed);
    }

    ApplyHPBarWidth(hp_bar_delay, HPDelayPercent);
    ApplyHPBarWidth(hp_bar, HPCurrentPercent);
}
```

## 8. ApplyHPBarWidth — MyCanvas.cpp (TickHPBar 아래)
```cpp
// ProgressBar의 SetPercent 역할. 슬롯 폭으로 그린다
void UMyCanvas::ApplyHPBarWidth(UImage* Bar, float Percent)
{
    if (!Bar)
    {
        return;
    }

    if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Bar))
    {
        CanvasSlot->SetSize(FVector2D(HPBarFullWidth * Percent, HPBarHeight));
    }
}
```

---

## 9. 플레이어 쪽 호출 교체 — MyPlayer.cpp:620 (UpdateHPUI)
```cpp
// Before
void AMyPlayer::UpdateHPUI()
{
    if (CanvasWidget)
    {
        CanvasWidget->SetProgressUISize(FVector2D(HP * 500 / FMath::Max(1, MaxHP), 50));
    }
}
```
```cpp
// After
void AMyPlayer::UpdateHPUI()
{
    if (CanvasWidget)
    {
        CanvasWidget->SetHPPercent((float)HP / (float)FMath::Max(1, MaxHP));
    }
}
```

`SetProgressUISize`를 부르는 다른 곳이 없는지 확인 (`SetHP`, `SetCanvasWidget` 경유만 있음).

---

## 10. 에디터 작업 — WBP_Canvas
1. `hp_bar`를 복제해서 이름을 **hp_bar_delay**로 바꾼다.
2. Hierarchy에서 `hp_bar_delay`를 `hp_bar` **위쪽(= 먼저 그려짐)** 으로 옮긴다. 노란바가 뒤에 깔려야 한다.
3. 두 Image 모두 Canvas 슬롯에서:
   - Anchors: 왼쪽 (Minimum/Maximum X 동일)
   - **Alignment X = 0** => 왼쪽 고정, 오른쪽으로 줄어든다. 0.5면 가운데서 양쪽으로 줄어 보인다
   - Position 동일하게 맞춘다
4. `hp_bar_delay` 색을 노란색(예: `1, 0.8, 0.2`), `hp_bar`는 빨간색으로.
5. Size는 코드가 매 프레임 덮으므로 값은 아무래도 상관없다.

## 확인 사항
- HUD가 뜰 때 바가 0에서 차오르는지
- 피해를 받으면 빨간바는 즉시 줄고 노란바가 0.4초 뒤 따라오는지
- 회복하면 천천히 차오르는지
- 사망 후 ReStart에서 바가 다시 꽉 차는지 (`HPIntroCap`은 이미 1.0이므로 회복 보간으로 올라감)
