# 답안지 — 플레이어 HP바 차오름 애니메이션 (MyCanvas)

참고: `Docs/answer-bossHPbar.md` (BossStatusWidget 구현). 구조를 그대로 가져온다.

## 지금 상태 vs 목표
```
지금 : AMyPlayer::UpdateHPUI ─▶ MyCanvas::SetProgressUISize(FVector2D(HP*500/MaxHP, 50))
                                 └▶ UImage 슬롯 Size 즉시 대입. 애니메이션 없음

목표 : AMyPlayer::UpdateHPUI ─▶ MyCanvas::SetHPPercent(HP/MaxHP)  = 목표값만 저장
       MyCanvas::NativeTick  ─▶ TickHPBar(DeltaTime)
                                 ├ IntroCap  : HUD 뜰 때 0 → 현재 HP까지 차오름
                                 ├ HPBar      (빨강) : 피해는 즉시, 회복은 천천히 차오름
                                 └ HPDelayBar (노랑) : 피해 후 0.4초 버티고 따라 내려옴
```

`hp_bar`(UImage)를 **UProgressBar로 교체**한다. BossStatusWidget과 같은 방식이라 폭/높이 계산이 필요 없다.

순서: 1~3(헤더) → 4~7(cpp) → 8(플레이어 연결) → 9(에디터 작업)
**UPROPERTY 타입이 바뀌므로 Live Coding 금지, 풀 리빌드.**

---

## 1. 바 바인딩 교체 — MyCanvas.h:46
```cpp
	// Before
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* hp_bar;
```
```cpp
	// After
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* HPBar;

	// 잔상(노란) 바
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* HPDelayBar;
```

## 2. 튜닝값 — MyCanvas.h:84 (BossArrowEdgeMargin 아래)
```cpp
	// [플레이어 HP바]
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

## 3. 함수/상태 변수 — MyCanvas.h:98 / 112
public:
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
```

---

## 4. NativeConstruct — 바를 0부터 — MyCanvas.cpp:40 (조이스틱 처리 아래)
```cpp
	HPIntroCap = 0.0f;
	HPCurrentPercent = 0.0f;
	HPDelayPercent = 0.0f;
	HPBar->SetPercent(0.0f);
	HPDelayBar->SetPercent(0.0f);
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
BossStatusWidget::TickBar와 같은 로직. 마지막 SetPercent 두 줄만 다르다.
```cpp
void UMyCanvas::TickHPBar(float DeltaTime)
{
    // 처음 뜰 때 0 => 현재 HP까지 차오름
    HPIntroCap = FMath::FInterpConstantTo(HPIntroCap, 1.0f, DeltaTime, 1.0f / FMath::Max(HPIntroTime, 0.01f));
    const float Goal = FMath::Min(HPTargetPercent, HPIntroCap);

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

    HPBar->SetPercent(HPCurrentPercent);
    HPDelayBar->SetPercent(HPDelayPercent);
}
```

---

## 8. 플레이어 쪽 호출 교체 — MyPlayer.cpp:620 (UpdateHPUI)
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

---

## 9. 에디터 작업 — WBP_Canvas
기준: HUD 시안 캔버스 `모바일 · 추천 — 위쪽 한 줄` (1280 시안 × 1.5 = 1920×1080 기준 값)

### 9-1. 계층 구조
기존 `hp_bar`(Image)는 삭제. `hp_background`는 테두리로 재사용.
```
[Canvas Panel] (루트)
 └ SafeZone_Top          ← 노치/펀치홀 회피
    └ PlayerStatusPanel  (Canvas Panel)
       ├ Portrait        (Image)
       ├ hp_background   (Image)        ← 테두리
       ├ HPDelayBar      (Progress Bar) ← 노란 잔상
       ├ HPBar           (Progress Bar) ← 빨강
       ├ ExpBar          (Progress Bar)
       └ ExpText         (Text)
```
Hierarchy는 **위가 먼저 그려짐(뒤)**. `hp_background → HPDelayBar → HPBar` 순서를 지킨다.

### 9-2. 배치 (전부 Anchor: 왼쪽 위, Alignment 0,0)
| 위젯 | Position | Size | 비고 |
|---|---|---|---|
| SafeZone_Top | Anchor 전체 채움, Offset 0 | — | Pad Left/Top만 체크 |
| PlayerStatusPanel | (90, 36) | 600 × 150 | Size To Content 끔 |
| Portrait | (0, 0) | 96 × 96 | 초상화 텍스처 없으면 비워둠 |
| hp_background | (87, 24) | 375 × 33 | 테두리 |
| HPDelayBar | (91, 28) | 367 × 25 | hp_background 안쪽 4px |
| HPBar | (91, 28) | 367 × 25 | HPDelayBar와 **완전히 동일** |
| ExpBar | (100, 66) | 300 × 9 | HP 아래 얇게 |
| ExpText | (410, 58) | Auto | 글자 크기 14 |

### 9-3. 스타일 (색은 Hex sRGB로 입력)
| 위젯 | 항목 | 값 |
|---|---|---|
| hp_background | Brush Tint | `3A3129` |
| HPDelayBar | Background Image > Tint | `0D0B09` (빈 칸 배경) |
| HPDelayBar | Fill Color and Opacity | `D9A441` (노랑) |
| HPBar | Background Image > Tint | 알파 **0** => 배경 투명 |
| HPBar | Fill Color and Opacity | `B8321F` (빨강) |
| ExpBar | Background Image > Tint | `0D0B09` |
| ExpBar | Fill Color and Opacity | `5A8FB8` (파랑) |
| ExpText | Color | `A89A85`, 그림자 켜기 |

공통: Bar Fill Type `Left to Right`, Percent `0`, Fill Image의 Draw As `Box` 또는 `Image`.

### 9-4. 같이 확인
- 화면 위 가운데(약 X 600~1320)는 보스바 자리. PlayerStatusPanel 오른쪽 끝(약 X 690)을 넘기지 않는다.
- 위쪽 가장자리는 보스 화살표가 지나가는 자리라 `BossArrowEdgeMargin`(60)보다 바가 아래에 있으면 겹치지 않는다.

## 확인 사항
- HUD가 뜰 때 바가 0에서 차오르는지
- 피해를 받으면 빨간바는 즉시 줄고 노란바가 0.4초 뒤 따라오는지
- 회복하면 천천히 차오르는지
- 사망 후 ReStart에서 바가 다시 꽉 차는지
