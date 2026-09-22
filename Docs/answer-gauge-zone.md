# 답안지 — GaugeZone 베이스 추출

`AMoneyPadZone`이 들고 있던 "밟으면 게이지가 차는 발판" 로직을 `AGaugeZone`으로 올린다.
무엇으로 채우는지만 서브클래스가 정한다.

```
AGaugeZone (Abstract)          게이지, 트리거, 간격 타이머, 완성/쿨다운, 상태 영속
 ├─ AMoneyPadZone (Abstract)   돈으로 채움
 │   ├─ AWeaponUpgradeZone     (HandleZoneFilled만 오버라이드)
 │   └─ ACompanionSpawnZone    (HandleZoneFilled만 오버라이드)
 └─ AItemSellZone              가방 아이템으로 채움
```

작업 순서: 1 → 2 → **빌드** → 3 → 4 → 5 → **빌드** → 6 → 7 → **풀 리빌드** → 에디터 확인

---

## 사전 확인 — CoreRedirects 필요 없음

Zone 관련 에셋 6개(BP 4개 + 맵 배치 인스턴스 2개)를 전부 확인한 결과,
BP가 참조하는 프로퍼티는 `TriggerBox` / `PadMesh` **둘뿐**이다.

`PaidMoney` `MaxMoney` `MoneyPerPayment` `Progress` `bConsumed` `bOneShot` `Cooldown`
`OnProgressChanged` `OnZoneCompleted` `OnInsufficientFunds` — 어느 에셋에도 없다.

따라서 이름을 바꿔도 깨질 BP 노드도, 날아갈 저장값도 없다.
`TriggerBox` / `PadMesh`는 `CreateDefaultSubobject<>(TEXT("..."))`의 문자열로 매칭되므로
부모 클래스로 올라가도 그 문자열만 유지하면 그대로 붙는다.

이름 바꿈:

| 전 | 후 |
|---|---|
| `MaxMoney` | `RequiredAmount` |
| `PaidMoney` | `FilledAmount` |
| `PaymentInterval` | `FillInterval` |
| `PaymentTimer` | `FillTimer` |
| `MoneyPerPayment` | 그대로 (`AMoneyPadZone`에 남음) |

## BlueprintImplementableEvent 정리

`OnProgressChanged` / `OnZoneCompleted` / `OnInsufficientFunds` 셋 다 구현한 BP가 없다.
연출은 프로젝트 기존 방식(`AttackSound` / `HitSound` + `PlaySoundAtLocation`)에 맞춰 사운드 변수로 바꾼다.

| 전 | 후 |
|---|---|
| `OnInsufficientFunds()` | `USoundBase* InsufficientFundsSound` (MoneyPadZone) |
| `OnItemSold(Price, Remaining)` | `USoundBase* SellSound` (ItemSellZone) |
| `OnProgressChanged(float)` | 삭제. 게이지 표시는 `DrawDebugGauge()`가 담당 |
| `OnZoneCompleted(Player)` | 삭제. 서브클래스 알림은 `HandleZoneFilled()` (C++ virtual)로 충분 |

결과적으로 `AGaugeZone`에 `BlueprintImplementableEvent`가 하나도 남지 않는다.
BP 서브클래스 없이 C++ 액터를 레벨에 그대로 배치해도 전부 작동한다.

---

## 1. 게이지 베이스 헤더 — Source/ZombieHunter1/Public/Zones/GaugeZone.h

파일 전체 교체.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GaugeZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMyPlayer;

/**
 * 밟고 있는 동안 일정 간격으로 게이지가 차는 발판의 공통 베이스.
 * 무엇으로 채우는지는 TryFillOnce()를 오버라이드해서 서브클래스가 정한다.
 */
UCLASS(Abstract)
class ZOMBIEHUNTER1_API AGaugeZone : public AActor
{
	GENERATED_BODY()

public:
	AGaugeZone();

	virtual void Tick(float DeltaTime) override;

	// [설정]
	// 한 번 채우는 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone", meta = (ClampMin = "0.02"))
	float FillInterval = 0.15f;

	// 완성에 필요한 총량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone", meta = (ClampMin = "1"))
	int32 RequiredAmount = 5;

	// 한 번 완성되면 더 이상 작동하지 않게 할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone")
	bool bOneShot = false;

	// 완성 후 다시 채울 수 있게 되기까지의 쿨다운(초). bOneShot이 false일 때만 의미 있음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone", meta = (ClampMin = "0.0"))
	float Cooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GaugeZone|Debug")
	bool bShowDebugGauge = true;

	// [상태]
	// 지금까지 채운 양
	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	int32 FilledAmount = 0;

	// 현재 게이지(0.0 ~ 1.0)
	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "GaugeZone")
	bool bPlayerInside = false;

	// 완성까지 남은 양
	FORCEINLINE int32 GetRemainingAmount() const { return FMath::Max(0, RequiredAmount - FilledAmount); }

	// [상태 영속]
	bool IsConsumed() const { return bConsumed; }

	void RestorePadState(int32 InFilledAmount, int32 InRequiredAmount, bool bInConsumed);

protected:
	virtual void BeginPlay() override;

	// 한 번 채우기를 시도한다. 성공하면 채운 양을 OutAmount에 담고 true.
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) PURE_VIRTUAL(AGaugeZone::TryFillOnce, return false;);

	// 게이지가 가득 찼을 때 서브클래스가 할 일. Payer는 항상 유효.
	virtual void HandleZoneFilled(AMyPlayer* Payer) {}

	// 플레이어가 막 올라왔을 때. RequiredAmount를 그때그때 다시 잡을 때 쓴다.
	virtual void OnPlayerEntered(AMyPlayer* Player) {}

	// 추적하던 플레이어가 발판을 떠났을 때.
	virtual void OnPlayerExited(AMyPlayer* Player) {}

	void CompleteZone();

	void DrawDebugGauge();

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// [컴포넌트]
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GaugeZone")
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GaugeZone")
	UStaticMeshComponent* PadMesh;

	UPROPERTY()
	AMyPlayer* CurrentPlayer = nullptr;

	float FillTimer = 0.0f;
	float CooldownRemaining = 0.0f;
	bool bConsumed = false;
};
```

`FilledAmount` / `RequiredAmount` / `IsConsumed()` / `RestorePadState()`가 `public`인 이유:
`InfiniteMapGenerator.cpp`가 밖에서 직접 읽고 호출한다. `private`으로 두면 컴파일 에러.

`PURE_VIRTUAL` 매크로는 헤더 안에 본문을 만들어준다. 그냥 선언만 두면 서브클래스가 구현을 빠뜨렸을 때
링크 에러가 나지만, 이 매크로를 쓰면 런타임 로그로 알려준다.

---

## 2. 게이지 베이스 구현 — Source/ZombieHunter1/Private/Zones/GaugeZone.cpp

파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Zones/GaugeZone.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

AGaugeZone::AGaugeZone()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// 판정은 TriggerBox 하나만 담당한다.
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGaugeZone::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGaugeZone::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AGaugeZone::OnTriggerEndOverlap);
	}
}

void AGaugeZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CooldownRemaining > 0.0f)
	{
		CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);
	}

	if (!IsValid(CurrentPlayer))
	{
		CurrentPlayer = nullptr;
		bPlayerInside = false;
	}

	const bool bActive = !bConsumed && CooldownRemaining <= 0.0f;

	if (bPlayerInside && bActive && IsValid(CurrentPlayer))
	{
		FillTimer += DeltaTime;
		if (FillTimer >= FillInterval)
		{
			FillTimer = 0.0f;

			int32 Amount = 0;
			if (TryFillOnce(CurrentPlayer, Amount) && Amount > 0)
			{
				FilledAmount = FMath::Min(FilledAmount + Amount, RequiredAmount);
				Progress = FMath::Clamp((float)FilledAmount / (float)RequiredAmount, 0.0f, 1.0f);

				if (FilledAmount >= RequiredAmount)
				{
					CompleteZone();
				}
			}
		}
	}
	else
	{
		FillTimer = 0.0f;
	}

	if (bShowDebugGauge)
	{
		DrawDebugGauge();
	}
}

void AGaugeZone::CompleteZone()
{
	AMyPlayer* Payer = CurrentPlayer;

	// HandleZoneFilled가 RequiredAmount를 키워도 안전하도록 먼저 리셋한다.
	FilledAmount = 0;
	Progress = 0.0f;

	if (IsValid(Payer))
	{
		HandleZoneFilled(Payer);
	}

	if (bOneShot)
	{
		bConsumed = true;
	}
	else
	{
		CooldownRemaining = Cooldown;
	}
}

void AGaugeZone::RestorePadState(int32 InFilledAmount, int32 InRequiredAmount, bool bInConsumed)
{
	RequiredAmount = FMath::Max(1, InRequiredAmount);
	FilledAmount = FMath::Clamp(InFilledAmount, 0, RequiredAmount);
	bConsumed = bInConsumed;

	Progress = (float)FilledAmount / (float)RequiredAmount;
}

void AGaugeZone::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	AMyPlayer* Player = Cast<AMyPlayer>(OtherActor);
	if (!Player)
	{
		return;
	}

	CurrentPlayer = Player;
	bPlayerInside = true;

	OnPlayerEntered(Player);
}

void AGaugeZone::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (OtherActor != CurrentPlayer)
	{
		return;
	}

	AMyPlayer* Leaver = CurrentPlayer;

	CurrentPlayer = nullptr;
	bPlayerInside = false;

	OnPlayerExited(Leaver);

	// 다른 플레이어가 아직 안에 있으면 그 사람으로 승계.
	if (TriggerBox)
	{
		TArray<AActor*> Overlapping;
		TriggerBox->GetOverlappingActors(Overlapping, AMyPlayer::StaticClass());
		for (AActor* A : Overlapping)
		{
			if (AMyPlayer* P = Cast<AMyPlayer>(A))
			{
				CurrentPlayer = P;
				bPlayerInside = true;
				OnPlayerEntered(P);
				break;
			}
		}
	}
}

// debug : 그리는 함수
void AGaugeZone::DrawDebugGauge()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Base = GetActorLocation() + FVector(0, 0, 220.0f);
	const float HalfWidth = 120.0f;
	const FVector Left = Base + FVector(-HalfWidth, 0, 0);
	const FVector Right = Base + FVector(HalfWidth, 0, 0);
	const FVector FillRight = Left + (Right - Left) * FMath::Clamp(Progress, 0.0f, 1.0f);

	DrawDebugLine(World, Left, Right, FColor(60, 60, 60), false, -1.0f, 0, 8.0f);
	DrawDebugLine(World, Left, FillRight, FColor::Green, false, -1.0f, 0, 8.0f);
}
```

여기까지 하고 **한 번 빌드**한다. `MoneyPadZone`은 아직 `AActor` 상속 그대로라 영향 없다.

---

## 3. 돈 발판 헤더 — Source/ZombieHunter1/Public/Zones/MoneyPadZone.h

파일 전체 교체. 150줄이 25줄로 줄어든다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Zones/GaugeZone.h"
#include "MoneyPadZone.generated.h"

class AMyPlayer;
class USoundBase;

/**
 * 돈으로 채우는 발판. 밟고 있으면 간격마다 MoneyPerPayment씩 빠지고 그만큼 게이지가 찬다.
 * 완성 시 무엇을 줄지는 HandleZoneFilled()를 오버라이드하는 서브클래스가 정한다.
 *  - ACompanionSpawnZone : 동료 소환
 *  - AWeaponUpgradeZone  : 플레이어 무기 강화
 */
UCLASS(Abstract)
class ZOMBIEHUNTER1_API AMoneyPadZone : public AGaugeZone
{
	GENERATED_BODY()

public:
	// 한 번 결제할 때 소비하는 돈(원)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MoneyPadZone", meta = (ClampMin = "1"))
	int32 MoneyPerPayment = 1;

	// 돈이 부족해 결제에 실패했을 때 재생
	UPROPERTY(EditAnywhere, Category = "MoneyPadZone")
	USoundBase* InsufficientFundsSound = nullptr;

protected:
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;
};
```

---

## 4. 돈 발판 구현 — Source/ZombieHunter1/Private/Zones/MoneyPadZone.cpp

파일 전체 교체. 200줄이 35줄로 줄어든다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Zones/MoneyPadZone.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

bool AMoneyPadZone::TryFillOnce(AMyPlayer* Player, int32& OutAmount)
{
	// 마지막 한 칸은 남은 금액만 받아 초과 결제를 막는다.
	const int32 Payment = FMath::Min(MoneyPerPayment, GetRemainingAmount());
	if (Payment <= 0)
	{
		return false;
	}

	if (!Player->TrySpendMoney(Payment))
	{
		UGameplayStatics::PlaySoundAtLocation(this, InsufficientFundsSound, GetActorLocation());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(7001, 1.0f, FColor::Red,
				FString::Printf(TEXT("[Zone] 돈 부족! (%d원 필요)"), MoneyPerPayment));
		}
		return false;
	}

	OutAmount = Payment;
	return true;
}
```

`PlaySoundAtLocation`은 사운드가 `nullptr`이면 아무것도 하지 않으므로 따로 검사하지 않는다.

`AWeaponUpgradeZone` / `ACompanionSpawnZone`의 `.h`는 수정하지 않는다.

---

## 5. 이름 바뀐 곳 따라가기

### 5-1. Source/ZombieHunter1/Private/Zones/WeaponUpgradeZone.cpp:12, 18

```cpp
// Before
	MaxMoney += CostGrowth;
	...
			FString::Printf(TEXT("[UpgradeZone] 무기 강화 완료! (다음 비용 %d원)"), MaxMoney));
```

```cpp
	RequiredAmount += CostGrowth;
	...
			FString::Printf(TEXT("[UpgradeZone] 무기 강화 완료! (다음 비용 %d원)"), RequiredAmount));
```

`WeaponUpgradeZone.h:20`의 주석도 `MaxMoney` → `RequiredAmount`로 고친다.

### 5-2. Source/ZombieHunter1/Private/InfiniteMapGenerator.cpp:589

```cpp
// Before
	const bool bIsDefault =
		Pad->PaidMoney == Defaults->PaidMoney &&
		Pad->MaxMoney == Defaults->MaxMoney &&
		Pad->IsConsumed() == Defaults->IsConsumed();
```

```cpp
	const bool bIsDefault =
		Pad->FilledAmount == Defaults->FilledAmount &&
		Pad->RequiredAmount == Defaults->RequiredAmount &&
		Pad->IsConsumed() == Defaults->IsConsumed();
```

### 5-3. Source/ZombieHunter1/Private/InfiniteMapGenerator.cpp:604

```cpp
// Before
	POIStateStore.SavePad(Coord, Pad->PaidMoney, Pad->MaxMoney, Pad->IsConsumed());

	UE_LOG(LogTemp, Log, TEXT("[POIState] 저장: 청크(%d, %d) Paid %d / Max %d"),
		Coord.X, Coord.Y, Pad->PaidMoney, Pad->MaxMoney);
```

```cpp
	POIStateStore.SavePad(Coord, Pad->FilledAmount, Pad->RequiredAmount, Pad->IsConsumed());

	UE_LOG(LogTemp, Log, TEXT("[POIState] 저장: 청크(%d, %d) Filled %d / Required %d"),
		Coord.X, Coord.Y, Pad->FilledAmount, Pad->RequiredAmount);
```

`FPOIState`(POIState.h)의 `PaidMoney` / `MaxMoney` 필드는 **그대로 둔다.**
저장 레코드 쪽 이름이라 액터와 무관하고, 433줄의 `Saved.PaidMoney` 같은 코드도 안 건드려도 된다.

여기까지 하고 **다시 빌드**한다. 에디터에서 배치된 `BP_CompanionSpawnZone` / `BP_WeaponUpgradeZone`을 열어
Details 패널에 `Required Amount`, `Money Per Payment`가 보이는지 확인한다.

---

## 6. 판매 발판 헤더 — Source/ZombieHunter1/Public/Zones/ItemSellZone.h

파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Zones/GaugeZone.h"
#include "ItemSellZone.generated.h"

class AMyPlayer;
class USoundBase;

// 밟고 있는 동안 가방을 한 개씩 파는 발판. 올라온 시점의 가방 개수가 게이지 총량이 된다.
UCLASS()
class ZOMBIEHUNTER1_API AItemSellZone : public AGaugeZone
{
	GENERATED_BODY()

public:
	AItemSellZone();

	// 한 개 팔릴 때마다 재생
	UPROPERTY(EditAnywhere, Category = "SellZone")
	USoundBase* SellSound = nullptr;

protected:
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;

	virtual void OnPlayerEntered(AMyPlayer* Player) override;

	virtual void OnPlayerExited(AMyPlayer* Player) override;

	virtual void HandleZoneFilled(AMyPlayer* Payer) override;

private:
	// 이번에 올라와서 판 것들을 합쳐 한 줄로 알린다.
	void FlushSoldSummary(AMyPlayer* Player);

	int32 SoldCount = 0;
	int32 SoldTotal = 0;
};
```

---

## 7. 판매 발판 구현 — Source/ZombieHunter1/Private/Zones/ItemSellZone.cpp

파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Zones/ItemSellZone.h"
#include "Component/InventoryComponent.h"
#include "Items/ItemDataAsset.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h"

AItemSellZone::AItemSellZone()
{
	FillInterval = 0.3f;
	RequiredAmount = 1;
	bOneShot = false;
	Cooldown = 0.0f;
}

// 올라온 시점의 가방 개수를 게이지 총량으로 잡는다. 기본값: 빈 가방이면 1
void AItemSellZone::OnPlayerEntered(AMyPlayer* Player)
{
	UInventoryComponent* Bag = Player ? Player->GetBag() : nullptr;

	RequiredAmount = Bag ? FMath::Max(1, Bag->GetItems().Num()) : 1;
	FilledAmount = 0;
	Progress = 0.0f;

	SoldCount = 0;
	SoldTotal = 0;

	// 밟자마자 첫 한 개가 팔리게
	FillTimer = FillInterval;
}

void AItemSellZone::OnPlayerExited(AMyPlayer* Player)
{
	FlushSoldSummary(Player);
}

bool AItemSellZone::TryFillOnce(AMyPlayer* Player, int32& OutAmount)
{
	UInventoryComponent* Bag = Player ? Player->GetBag() : nullptr;
	if (!Bag)
	{
		return false;
	}

	UItemDataAsset* Item = Bag->PopItem();
	if (!Item)
	{
		return false;
	}

	Player->GainMoney(Item->Price);

	SoldCount++;
	SoldTotal += Item->Price;

	UGameplayStatics::PlaySoundAtLocation(this, SellSound, GetActorLocation());

	OutAmount = 1;
	return true;
}

void AItemSellZone::HandleZoneFilled(AMyPlayer* Payer)
{
	FlushSoldSummary(Payer);
}

void AItemSellZone::FlushSoldSummary(AMyPlayer* Player)
{
	if (SoldCount <= 0 || !IsValid(Player))
	{
		SoldCount = 0;
		SoldTotal = 0;
		return;
	}

	FText Msg = FText::Format(FText::FromString(TEXT("아이템 {0}개를 팔아 {1}원을 받았습니다.")), SoldCount, SoldTotal);
	Player->ShowOnItemText(Msg, EItemNotifyType::Gain);

	SoldCount = 0;
	SoldTotal = 0;
}
```

`FillTimer`를 `FillInterval`로 시작해서 밟자마자 첫 개가 나간다. 0으로 두면 0.3초 멍하니 기다린다.

---

## 빌드

새 클래스를 추가한 게 아니라 상속만 바꾼 것이지만, `UCLASS` 계층이 바뀌었으므로
**Live Coding(Ctrl+Alt+F11) 금지.** 에디터 닫고 풀 리빌드한다.

## 에디터 확인

1. 배치된 `BP_CompanionSpawnZone` / `BP_WeaponUpgradeZone` Details 패널에
   `Required Amount`, `Money Per Payment`, `Fill Interval`이 보이는가
2. 발판 밟고 돈 빠지면서 디버그 게이지 바가 차는가
3. 가득 차면 동료 소환 / 무기 강화가 되는가
4. **청크 왕복** — 발판을 절반쯤 채우고 멀리 걸어가 청크를 언로드시킨 뒤 돌아와서
   게이지가 그대로 복원되는가 (`[POIState] 저장:` 로그 확인)
5. `AItemSellZone`을 레벨에 배치하고 가방을 채운 채 밟았을 때
   0.3초마다 하나씩 팔리고 게이지가 차는가, 다 팔면 알림 한 줄이 뜨는가
