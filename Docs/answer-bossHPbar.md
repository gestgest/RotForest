# 답안지 — 보스 HP바 (엘든링식)

기획: 프로젝트 문서 `Docs/기획-보스HP바.md`

## 흐름
```
보스 Tick ─ 플레이어가 EngageRadius 안 ─▶ MyCanvas::StartBossEncounter
                                          └▶ BossHPBar: 컷인 → 바 페이드인 + 0→100% 차오름
보스 SetHP ─ OnHPChanged.Broadcast(NewHP, Delta) ─▶ BossHPBar: 현재 바 즉시 / 잔상 바 지연 / 누적 데미지
보스 HP 0 ─▶ BossHPBar: 1초 정지 → 페이드아웃
플레이어 사망 / DisengageRadius 밖 / 청크 언로드 ─▶ MyCanvas::EndBossEncounter ─▶ 페이드아웃
```

순서: 1~3(HP 이벤트) → 4~5(위젯 클래스) → 6~8(캔버스/플레이어 연결) → 9~10(보스) → 11(에디터 작업)
**새 C++ 클래스가 추가되므로 Live Coding 금지, 풀 리빌드.**

---

## 1. HP 변경 델리게이트 선언 — CombatCharacter.h:20 (LogWeapon 선언 바로 아래)
```cpp
// 인자: 새 HP, 변화량(피해면 음수)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, int32, int32);
```

## 2. 델리게이트 멤버 — CombatCharacter.h:232 (SetHP 선언 위)
```cpp
	FOnHPChanged OnHPChanged;

	// 죽음 상태, HP바 갱신 
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetHP(int32 new_hp);
```

## 3. SetHP에서 방송 — CombatCharacter.cpp:290
```cpp
// Before
void ACombatCharacter::SetHP(int32 new_hp)
{
	HP = new_hp;
	SetDead(HP <= 0);
	UpdateHPBar(); // 머리 위 바가 있는 캐릭터(적/동료)만 실제로 갱신된다
}
```
```cpp
// After
void ACombatCharacter::SetHP(int32 new_hp)
{
	const int32 OldHP = HP;
	HP = new_hp;
	SetDead(HP <= 0);
	UpdateHPBar(); // 머리 위 바가 있는 캐릭터(적/동료)만 실제로 갱신된다

	// 오버킬 제외 => HP 30에 50 피해면 Delta = -30
	const int32 Delta = FMath::Max(HP, 0) - FMath::Max(OldHP, 0);
	if (Delta != 0)
	{
		OnHPChanged.Broadcast(HP, Delta);
	}
}
```

---

## 4. 새 클래스 헤더 — Public/UI/BossHPBarWidget.h (새 파일)
에디터 Tools → New C++ Class → UserWidget 부모, 이름 `BossHPBarWidget`, 경로 `Public/UI` 로 만든 뒤 내용 교체.
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHPBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class ACombatCharacter;

UENUM()
enum class EBossBarState : uint8
{
	Hidden,
	Cutin,
	Active,
	Dying,
	FadingOut,
};

UCLASS()
class ZOMBIEHUNTER1_API UBossHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartEncounter(ACombatCharacter* InBoss, const FText& InBossName);
	void EndEncounter(ACombatCharacter* InBoss);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// [바]
	// 이름 + 데미지 + 바 묶음. 페이드 대상
	UPROPERTY(meta = (BindWidget))
	UWidget* BarRoot;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BossNameText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HPBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DelayBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

	// [컷인]
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CutinText;

	// [수치]
	UPROPERTY(EditAnywhere, Category = "Boss|Cutin")
	float CutinFadeTime = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Boss|Cutin")
	float CutinHoldTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float BarFadeTime = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float IntroFillTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float DelayHoldTime = 0.4f;

	// 초당 비율 => 0.5면 가득 찬 잔상이 2초에 빠짐
	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float DelayDrainSpeed = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float HealFillSpeed = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Bar")
	float DeathHoldTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Damage")
	float DamageShowTime = 2.0f;

private:
	void SetState(EBossBarState NewState);
	void TickCutin();
	void TickBar(float DeltaTime);

	void BindBoss(ACombatCharacter* InBoss);
	void UnbindBoss();
	void HandleHPChanged(int32 NewHP, int32 Delta);
	void ResetDamage();
	float GetBossPercent() const;

	TWeakObjectPtr<ACombatCharacter> Boss;
	TWeakObjectPtr<ACombatCharacter> CutinShownBoss;
	FDelegateHandle HPChangedHandle;

	EBossBarState State = EBossBarState::Hidden;
	float StateTime = 0.0f;

	float TargetPercent = 0.0f;
	float ShownPercent = 0.0f;
	float DelayPercent = 0.0f;
	float IntroCap = 0.0f;
	float DelayHoldRemain = 0.0f;
	float BarOpacity = 0.0f;

	int32 AccumDamage = 0;
	float DamageRemain = 0.0f;
};
```

## 5. 새 클래스 구현 — Private/UI/BossHPBarWidget.cpp (새 파일)
```cpp
#include "UI/BossHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/CombatCharacter.h"

// [외부 호출]
void UBossHPBarWidget::StartEncounter(ACombatCharacter* InBoss, const FText& InBossName)
{
	if (!InBoss || InBoss->GetIsDead())
	{
		return;
	}

	const bool bSameBoss = (Boss.Get() == InBoss);
	if (bSameBoss && (State == EBossBarState::Cutin || State == EBossBarState::Active))
	{
		return;
	}

	if (!bSameBoss)
	{
		UnbindBoss();
		BindBoss(InBoss);
	}
	ResetDamage();

	BossNameText->SetText(InBossName);
	CutinText->SetText(InBossName);

	// 컷인은 보스당 1번
	SetState(CutinShownBoss.Get() == InBoss ? EBossBarState::Active : EBossBarState::Cutin);
}

void UBossHPBarWidget::EndEncounter(ACombatCharacter* InBoss)
{
	if (Boss.Get() != InBoss)
	{
		return;
	}

	switch (State)
	{
	case EBossBarState::Cutin:
		SetState(EBossBarState::Hidden);
		break;
	case EBossBarState::Active:
		ResetDamage();
		SetState(EBossBarState::FadingOut);
		break;
	default:
		break;
	}
}

// [생명주기]
void UBossHPBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetState(EBossBarState::Hidden);
}

void UBossHPBarWidget::NativeDestruct()
{
	UnbindBoss();
	Super::NativeDestruct();
}

void UBossHPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	StateTime += InDeltaTime;

	switch (State)
	{
	case EBossBarState::Cutin:
		TickCutin();
		break;

	case EBossBarState::Active:
		TickBar(InDeltaTime);
		break;

	case EBossBarState::Dying:
		TickBar(InDeltaTime);
		if (StateTime >= DeathHoldTime)
		{
			SetState(EBossBarState::FadingOut);
		}
		break;

	case EBossBarState::FadingOut:
		TickBar(InDeltaTime);
		if (BarOpacity <= 0.0f)
		{
			SetState(EBossBarState::Hidden);
		}
		break;

	default:
		break;
	}
}

// [상태]
void UBossHPBarWidget::SetState(EBossBarState NewState)
{
	State = NewState;
	StateTime = 0.0f;

	switch (NewState)
	{
	case EBossBarState::Hidden:
		SetVisibility(ESlateVisibility::Collapsed);
		UnbindBoss();
		ResetDamage();
		break;

	case EBossBarState::Cutin:
		SetVisibility(ESlateVisibility::HitTestInvisible);
		BarRoot->SetVisibility(ESlateVisibility::Collapsed);
		CutinText->SetVisibility(ESlateVisibility::HitTestInvisible);
		CutinText->SetRenderOpacity(0.0f);
		break;

	case EBossBarState::Active:
		SetVisibility(ESlateVisibility::HitTestInvisible);
		CutinText->SetVisibility(ESlateVisibility::Collapsed);
		BarRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		BarOpacity = 0.0f;
		BarRoot->SetRenderOpacity(0.0f);
		IntroCap = 0.0f;
		ShownPercent = 0.0f;
		DelayPercent = 0.0f;
		TargetPercent = GetBossPercent();
		break;

	default:
		break;
	}
}

// 페이드인 → 유지 → 페이드아웃
void UBossHPBarWidget::TickCutin()
{
	const float Fade = FMath::Max(CutinFadeTime, 0.01f);
	const float T = StateTime;

	float Alpha = 0.0f;
	if (T < Fade)
	{
		Alpha = T / Fade;
	}
	else if (T < Fade + CutinHoldTime)
	{
		Alpha = 1.0f;
	}
	else if (T < Fade * 2.0f + CutinHoldTime)
	{
		Alpha = 1.0f - (T - Fade - CutinHoldTime) / Fade;
	}
	else
	{
		CutinShownBoss = Boss;
		SetState(EBossBarState::Active);
		return;
	}

	CutinText->SetRenderOpacity(Alpha);
}

void UBossHPBarWidget::TickBar(float DeltaTime)
{
	// [페이드]
	const float TargetOpacity = (State == EBossBarState::FadingOut) ? 0.0f : 1.0f;
	BarOpacity = FMath::FInterpConstantTo(BarOpacity, TargetOpacity, DeltaTime, 1.0f / FMath::Max(BarFadeTime, 0.01f));
	BarRoot->SetRenderOpacity(BarOpacity);

	// [등장 차오름]
	IntroCap = FMath::FInterpConstantTo(IntroCap, 1.0f, DeltaTime, 1.0f / FMath::Max(IntroFillTime, 0.01f));
	const float Goal = FMath::Min(TargetPercent, IntroCap);

	// [현재 바] 감소는 즉시, 회복은 서서히
	if (Goal < ShownPercent || IntroCap < 1.0f)
	{
		ShownPercent = Goal;
	}
	else
	{
		ShownPercent = FMath::FInterpConstantTo(ShownPercent, Goal, DeltaTime, HealFillSpeed);
	}

	// [잔상 바] 마지막 타격 후 DelayHoldTime 대기 → 따라 내려감
	if (DelayPercent <= ShownPercent)
	{
		DelayPercent = ShownPercent;
	}
	else if (DelayHoldRemain > 0.0f)
	{
		DelayHoldRemain -= DeltaTime;
	}
	else
	{
		DelayPercent = FMath::FInterpConstantTo(DelayPercent, ShownPercent, DeltaTime, DelayDrainSpeed);
	}

	HPBar->SetPercent(ShownPercent);
	DelayBar->SetPercent(DelayPercent);

	// [누적 데미지]
	if (DamageRemain > 0.0f)
	{
		DamageRemain -= DeltaTime;
		if (DamageRemain <= 0.0f)
		{
			ResetDamage();
		}
	}
}

// [보스 연결]
void UBossHPBarWidget::BindBoss(ACombatCharacter* InBoss)
{
	Boss = InBoss;
	HPChangedHandle = InBoss->OnHPChanged.AddUObject(this, &UBossHPBarWidget::HandleHPChanged);
}

void UBossHPBarWidget::UnbindBoss()
{
	if (ACombatCharacter* B = Boss.Get())
	{
		B->OnHPChanged.Remove(HPChangedHandle);
	}
	HPChangedHandle.Reset();
	Boss.Reset();
}

void UBossHPBarWidget::HandleHPChanged(int32 NewHP, int32 Delta)
{
	TargetPercent = GetBossPercent();

	if (State == EBossBarState::Cutin && NewHP <= 0)
	{
		SetState(EBossBarState::Hidden);
		return;
	}

	if (State != EBossBarState::Active)
	{
		return;
	}

	if (Delta < 0)
	{
		AccumDamage += -Delta;
		DamageRemain = DamageShowTime;
		DelayHoldRemain = DelayHoldTime;
		DamageText->SetText(FText::AsNumber(AccumDamage));
		DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (NewHP <= 0)
	{
		SetState(EBossBarState::Dying);
	}
}

void UBossHPBarWidget::ResetDamage()
{
	AccumDamage = 0;
	DamageRemain = 0.0f;
	if (DamageText)
	{
		DamageText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

float UBossHPBarWidget::GetBossPercent() const
{
	ACombatCharacter* B = Boss.Get();
	if (!B || B->GetMaxHP() <= 0)
	{
		return 0.0f;
	}
	return FMath::Clamp((float)B->GetHP() / (float)B->GetMaxHP(), 0.0f, 1.0f);
}
```

---

## 6. 캔버스에 보스바 슬롯 — MyCanvas.h:18 (전방 선언)
```cpp
class UVirtualJoystick;
class UDeathPanelWidget;
class UExitPanelWidget;
class UBossHPBarWidget;
class ACombatCharacter;
```

## 7. 캔버스 멤버/함수 — MyCanvas.h:97 (RemoveItemNotification 아래)
```cpp
    void AddItemNotification(const FText& Text, EItemNotifyType Type = EItemNotifyType::Gain);
    void RemoveItemNotification();

    // [보스]
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBossHPBarWidget* BossHPBar;

    void StartBossEncounter(ACombatCharacter* Boss, const FText& BossName);
    void EndBossEncounter(ACombatCharacter* Boss);
};
```

## 8-1. 캔버스 구현 — MyCanvas.cpp:9 (include) + 파일 맨 아래
```cpp
#include "UI/BossHPBarWidget.h"
```
```cpp
void UMyCanvas::StartBossEncounter(ACombatCharacter* Boss, const FText& BossName)
{
    if (BossHPBar)
    {
        BossHPBar->StartEncounter(Boss, BossName);
    }
}

void UMyCanvas::EndBossEncounter(ACombatCharacter* Boss)
{
    if (BossHPBar)
    {
        BossHPBar->EndEncounter(Boss);
    }
}
```

## 8-2. 캔버스 게터 — MyPlayer.h:317 (`public: //Property Function` 바로 아래)
```cpp
public: //Property Function

	UMyCanvas* GetCanvasWidget() const { return CanvasWidget; }
```

---

## 9. 보스 헤더 — Boss.h (전체 교체)
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemy.h"


#include "Boss.generated.h"

class AInfiniteMapGenerator;
class AWeaponPickup;
class UWeaponDataAsset;
class UMyCanvas;

UCLASS()
class ZOMBIEHUNTER1_API ABoss : public AEnemy
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

	virtual void OnDeath() override;

	/** 소속 좀비마을을 알려준다 — 스폰 직후 생성기가 자기 자신과 중심 청크 좌표를 넣어준다.
	 *  이걸 알아야 죽을 때 "어느 마을을 클리어했는지"를 기록할 수 있다. */
	void SetHome(AInfiniteMapGenerator* InGenerator, const FIntPoint& InCenterChunk);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 이 보스가 떨굴 수 있는 무기들. 무기 에셋(DA_*)을 직접 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TArray<UWeaponDataAsset*> DropTable;

	// 바닥에 떨굴 픽업 액터 (BP_WeaponPickup)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
	TSubclassOf<AWeaponPickup> DropPickupClass;

	// [보스바]
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss")
	FText BossName = FText::FromString(TEXT("Boss"));

	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float EngageRadius = 1500.0f;

	// EngageRadius보다 커야 경계에서 바가 깜빡이지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float DisengageRadius = 3000.0f;

private:
	// 상태를 기록할 생성기. 약참조 — 레벨 종료 중이면 이미 사라졌을 수 있다. 
	TWeakObjectPtr<AInfiniteMapGenerator> HomeGenerator;

	FIntPoint HomeChunk = FIntPoint::ZeroValue;

	// (0,0)도 유효한 청크 좌표라 좌표값만으로는 "설정됨"을 구분할 수 없다 — 별도 플래그가 필요.
	bool bHomeSet = false;

	void SpawnDropPickup();

	// [보스바]
	bool bEngaged = false;

	void UpdateEncounter();
	UMyCanvas* GetPlayerCanvas() const;
};
```

## 10. 보스 구현 — Boss.cpp (include 추가 + 함수 4개 추가)
```cpp
#include "Characters/Boss.h"
#include "InfiniteMapGenerator.h" // 클리어 기록을 남길 곳 (POIStates)
#include "Items/WeaponPickup.h" // 바닥에 떨굴 전리품
#include "Items/ItemDataAsset.h"
#include "Kismet/GameplayStatics.h" // FinishSpawningActor
#include "Characters/MyPlayer.h"
#include "UI/MyCanvas.h"
```
```cpp
void ABoss::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateEncounter();
}

void ABoss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bEngaged)
    {
        bEngaged = false;
        if (UMyCanvas* Canvas = GetPlayerCanvas())
        {
            Canvas->EndBossEncounter(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ABoss::UpdateEncounter()
{
    // 죽음 연출은 위젯이 HP 0을 받아서 처리
    if (IsDead)
    {
        bEngaged = false;
        return;
    }

    AMyPlayer* Player = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(this, 0));

    bool bShouldEngage = false;
    if (Player && !Player->GetIsDead())
    {
        const float Radius = bEngaged ? DisengageRadius : EngageRadius;
        bShouldEngage = FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(Radius);
    }

    if (bShouldEngage == bEngaged)
    {
        return;
    }
    bEngaged = bShouldEngage;

    UMyCanvas* Canvas = GetPlayerCanvas();
    if (!Canvas)
    {
        return;
    }

    if (bEngaged)
    {
        Canvas->StartBossEncounter(this, BossName);
    }
    else
    {
        Canvas->EndBossEncounter(this);
    }
}

UMyCanvas* ABoss::GetPlayerCanvas() const
{
    AMyPlayer* Player = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(this, 0));
    return Player ? Player->GetCanvasWidget() : nullptr;
}
```

---

## 11. 에디터 작업 (풀 리빌드 후)

### 11-1. WBP_BossHPBar 만들기 — Content/UI/BP/
위젯 블루프린트 생성 → Class Settings → Parent Class = `BossHPBarWidget`.
이름이 **정확히** 아래와 같아야 BindWidget이 붙는다 (틀리면 WBP 컴파일 에러).
```
Canvas Panel
├─ CutinText        (TextBlock)  앵커: 정중앙, 폰트 48~64, 정렬 Center
└─ BarRoot          (VerticalBox) 앵커: 하단 중앙, Size 900 x 60, Position Y -100, Alignment (0.5, 1)
   ├─ HorizontalBox
   │  ├─ BossNameText (TextBlock)  Fill, 왼쪽 정렬
   │  └─ DamageText   (TextBlock)  Auto, 오른쪽 정렬
   └─ Overlay        높이 14~18
      ├─ Image        (프레임/배경, 어두운 색)
      ├─ DelayBar     (ProgressBar) Fill Color 노랑/흰색, Background 투명 X (어두운 배경 역할)
      └─ HPBar        (ProgressBar) Fill Color 짙은 빨강, Background Image Tint 알파 0  ← 중요
```
- HPBar가 DelayBar **위**(Overlay에서 아래쪽 순서)에 있어야 잔상이 뒤에 보인다.
- HPBar 배경 알파를 0으로 안 하면 잔상 바가 가려진다.

### 11-2. BP_Canvas에 배치 — Content/UI/BP/Panel/BP_Canvas
- WBP_BossHPBar를 드래그해서 넣고 이름을 **BossHPBar**로.
- 앵커: 전체 화면(우하단 꽉 찬 앵커), Offset 전부 0.

### 11-3. 보스 BP
- Details → Boss → `Boss Name` 입력.
- 컴포넌트 `HPBar`(머리 위 바) → Widget Class = **None** (머리 위 바 안 보이게).

---

## 12. QA 확인
- [ ] 보스에게 1500 안으로 접근 → 이름 컷인 → 하단 바 차오름
- [ ] 연타 시 숫자가 누적되고, 2초 손 떼면 사라짐
- [ ] 연타 중엔 노란 잔상이 멈춰 있다가 끝나면 내려감
- [ ] 한 방 킬(오버킬) 시 숫자가 남은 HP만큼만 더해짐
- [ ] 3000 밖으로 도망 → 바 페이드아웃 → 다시 접근 시 컷인 없이 바만 등장
- [ ] 플레이어 사망 시 바 사라짐
- [ ] 보스 처치 → 0에서 1초 정지 → 페이드아웃
- [ ] 컷인 도중 때려도 바가 나오면 현재 HP까지만 차오름
- [ ] 보스 청크 언로드 시 바가 남지 않음

---
---

# 부록 — 최소 테스트: "때리면 바가 줄어드는지"만 확인
컷인/잔상/페이드 없이, 접근하면 바가 뜨고 때리면 줄어드는 것까지만. (클래스명은 현재 코드 기준 `UBossHPBar`)

## A1. 전방 선언 + HPBar 바인딩 + 함수 추가 — BossHPBar.h
```cpp
class ACombatCharacter;
class UProgressBar;
class UTextBlock;
```
```cpp
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HPBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

private:
	float TargetPercent = 0.0f;

	int32 AccumDamage = 0;

	TWeakObjectPtr<ACombatCharacter> Boss;
	FDelegateHandle HPChangedHandle;

	void UnbindBoss();
	void BindBoss(ACombatCharacter* InBoss);
	void HandleHPChanged(int32 NewHP, int32 Delta);
	float GetBossPercent() const;
```

## A2. 시작 시 바 채우고 보이기 — BossHPBar.cpp StartEncounter 끝
```cpp
	UnbindBoss();
	BindBoss(InBoss);

	TargetPercent = GetBossPercent();
	HPBar->SetPercent(TargetPercent);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
```

## A3. Native 함수에 Super 호출 — BossHPBar.cpp
```cpp
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

void UBossHPBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}
```

## A4. HP 변경 시 바 갱신 — BossHPBar.cpp HandleHPChanged
```cpp
void UBossHPBar::HandleHPChanged(int32 NewHP, int32 Delta)
{
	TargetPercent = GetBossPercent();
	HPBar->SetPercent(TargetPercent);

	if (Delta < 0)
	{
		AccumDamage -= Delta;
		DamageText->SetText(FText::AsNumber(AccumDamage));
		DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

float UBossHPBar::GetBossPercent() const
{
	ACombatCharacter* B = Boss.Get();
	if (!B || B->GetMaxHP() <= 0)
	{
		return 0.0f;
	}
	return FMath::Clamp((float)B->GetHP() / (float)B->GetMaxHP(), 0.0f, 1.0f);
}
```

## A5. 캔버스 — MyCanvas.h (전방 선언 + RemoveItemNotification 아래)
```cpp
class UBossHPBar;
class ACombatCharacter;
```
```cpp
    // [보스]
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBossHPBar* BossHPBar;

    void StartBossEncounter(ACombatCharacter* Boss, const FText& BossName);
```

## A6. 캔버스 — MyCanvas.cpp (include + 맨 아래)
```cpp
#include "UI/BossHPBar.h"
```
```cpp
void UMyCanvas::StartBossEncounter(ACombatCharacter* Boss, const FText& BossName)
{
    if (BossHPBar)
    {
        BossHPBar->StartEncounter(Boss, BossName);
    }
}
```

## A7. 캔버스 게터 — MyPlayer.h:317
```cpp
public: //Property Function

	UMyCanvas* GetCanvasWidget() const { return CanvasWidget; }
```

## A8. 보스 — Boss.h (public / protected / private 에 각각 추가)
```cpp
public:
	virtual void Tick(float DeltaTime) override;
```
```cpp
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float EngageRadius = 1500.0f;
```
```cpp
private:
	bool bEngaged = false;
```

## A9. 보스 — Boss.cpp (include + Tick)
```cpp
#include "Characters/MyPlayer.h"
#include "UI/MyCanvas.h"
```
```cpp
void ABoss::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bEngaged || IsDead)
    {
        return;
    }

    AMyPlayer* Player = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Player)
    {
        return;
    }

    if (FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(EngageRadius))
    {
        return;
    }

    if (UMyCanvas* Canvas = Player->GetCanvasWidget())
    {
        Canvas->StartBossEncounter(this, FText::FromString(TEXT("Boss")));
        bEngaged = true;
    }
}
```

## A10. 에디터 (풀 리빌드 후)
- WBP_BossHPBar: 부모 `BossHPBar`, 안에 **HPBar**(ProgressBar), **DamageText**(TextBlock) 두 개만 있으면 됨
- BP_Canvas: WBP_BossHPBar를 넣고 이름 **BossHPBar**, 앵커 전체 화면
- 확인: 보스에게 1500 안으로 접근 → 바 등장 → 때리면 줄어들고 숫자 누적
