# 답안지 — 적 HP 바 디자인 정리

## 목표
- 형광 핑크 선 → 어두운 트랙 + 피색 바 (남은 비율이 읽히게)
- 피가 꽉 찬 적은 바 숨김, 맞은 적만 표시
- 동료(아군)는 지금처럼 살아있으면 항상 표시
- `UHPBar`(잔상/등장 애니메이션)는 안 씀. 적은 `UEnemyHPBarWidget` 그대로 (Tick 없음, 가벼움)

순서: 1(에디터) → 2(코드 한 곳) → 3(선택: 몇 초 뒤 숨김)

---

## 1. 위젯 디자인 — WBP_EnemyHPBarWidget (에디터, 코드 수정 없음)
`Content/UI/BP/WBP_EnemyHPBarWidget` 계층 구조:
```
Canvas Panel
└─ SizeBox        Width 50, Height 7, 앵커 정중앙, Alignment (0.5, 0.5)
   └─ Border      Brush Color #0C0C0C (A 1.0), Padding 1   ← 검은 테두리
      └─ HPBar    (ProgressBar)   ← 이름 그대로 유지 (BindWidget)
```
HPBar → Details → Style
- Background Image → Tint: `#080808`, 알파 `0.7`
- Fill Image → Tint: 흰색 그대로 두고 아래 Fill Color로 색 지정
- Fill Color and Opacity: `#8E1B1B`
- Bar Fill Type: Left to Right

컴파일 → 저장.

참고: 컴포넌트 Draw Size가 100x12 (`CombatCharacter.cpp:313`) 라 SizeBox 50x7은 그 안에 들어간다.

---

## 2. 맞은 적만 보이게 — CombatCharacter.cpp:317 UpdateHPBar
```cpp
// Before
	// 죽으면 시체 위에 바가 남지 않게 숨긴다. 풀 재사용/부활(SetHP>0) 시 다시 보인다.
	HPBarComponent->SetVisibility(!IsDead);
```
```cpp
void ACombatCharacter::UpdateHPBar()
{
	if (!HPBarComponent)
	{
		return; // 플레이어 등 바 없는 캐릭터
	}

	if (UEnemyHPBarWidget* Bar = Cast<UEnemyHPBarWidget>(HPBarComponent->GetWidget()))
	{
		Bar->SetHPPercent(MaxHP > 0 ? (float)HP / (float)MaxHP : 0.0f);
	}

	// 아군: 살아있으면 항상 / 적: 맞았을 때만
	const bool bDamaged = HP < MaxHP;
	const bool bShow = !IsDead && (TeamType == ETeam::Ally || bDamaged);
	HPBarComponent->SetVisibility(bShow);
}
```
- 풀에서 다시 꺼낸 적은 `SetHP(MaxHP)`를 거치므로 자동으로 숨겨진 상태로 시작한다.

---

## 3. (선택) 몇 초 안 맞으면 다시 숨기기
2만으로도 화면이 지저분하면 그때 추가. 위젯 Tick 대신 액터 타이머 사용.

### 3-1. 선언 — CombatCharacter.h:75 (UpdateHPBar 선언 아래)
```cpp
	void UpdateHPBar();
	void HideHPBar();
```

### 3-2. 멤버 — CombatCharacter.h:210 (HPBarComponent 아래)
```cpp
	UWidgetComponent* HPBarComponent = nullptr;

	// 마지막 피격 후 적 HP 바 유지 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "HPBar")
	float HPBarShowTime = 3.0f;

	FTimerHandle HPBarHideTimer;
```

### 3-3. include — CombatCharacter.cpp 상단
```cpp
#include "TimerManager.h"
```

### 3-4. UpdateHPBar 교체 + HideHPBar 추가 — CombatCharacter.cpp:317
```cpp
void ACombatCharacter::UpdateHPBar()
{
	if (!HPBarComponent)
	{
		return; // 플레이어 등 바 없는 캐릭터
	}

	if (UEnemyHPBarWidget* Bar = Cast<UEnemyHPBarWidget>(HPBarComponent->GetWidget()))
	{
		Bar->SetHPPercent(MaxHP > 0 ? (float)HP / (float)MaxHP : 0.0f);
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(HPBarHideTimer);
	}

	// 아군: 살아있으면 항상
	if (TeamType == ETeam::Ally)
	{
		HPBarComponent->SetVisibility(!IsDead);
		return;
	}

	// 적: 맞았을 때만, HPBarShowTime 뒤 숨김
	const bool bShow = !IsDead && HP < MaxHP;
	HPBarComponent->SetVisibility(bShow);

	if (bShow && World)
	{
		World->GetTimerManager().SetTimer(HPBarHideTimer, this, &ACombatCharacter::HideHPBar, HPBarShowTime, false);
	}
}

void ACombatCharacter::HideHPBar()
{
	if (HPBarComponent)
	{
		HPBarComponent->SetVisibility(false);
	}
}
```
- 헤더에 멤버가 추가되므로 Live Coding 말고 풀 리빌드.

---

## 4. QA 확인
- [ ] 처음 스폰된 적은 바가 안 보임
- [ ] 때리면 바가 뜨고 남은 비율이 어두운 트랙 위에서 읽힘
- [ ] 죽으면 바 사라짐
- [ ] 동료는 피가 꽉 차도 바가 보임
- [ ] 풀에서 재사용된 적이 이전 바 상태를 달고 나오지 않음
- [ ] (3을 했다면) 3초 안 때리면 사라지고, 다시 때리면 다시 뜸
