# 답안지 — 판매 발판 게이지식 전환

즉발 전량 판매 → **발판 위에 있는 동안 한 개씩** 판매로 변경.
기존 발판(`AMoneyPadZone`)들과 조작감을 맞추고, 중간에 벗어나면 멈춰서 실수를 되돌릴 수 있게 한다.

작업 순서: 1 → 2 (가방에 꺼내기 API) → 3 → 4 (발판) → 리빌드

---

## 1. 가방에서 한 개 꺼내기 — Source/ZombieHunter1/Public/Component/InventoryComponent.h:36

`ClearAll()` 선언 아래에 추가.

```cpp
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAll();

	// 가방에서 한 개 꺼낸다. 비어 있으면 nullptr.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItemDataAsset* PopItem();
```

---

## 2. 꺼내기 구현 — Source/ZombieHunter1/Private/Component/InventoryComponent.cpp:65

`ClearAll()` 아래에 추가.

```cpp
UItemDataAsset* UInventoryComponent::PopItem()
{
	while (Items.Num() > 0)
	{
		UItemDataAsset* Item = Items.Pop(EAllowShrinking::No);
		if (Item)
		{
			return Item;
		}
	}

	return nullptr;
}
```

- `TArray::Pop()`은 마지막 원소를 **반환하고 제거**한다. 뒤에서 꺼내므로 스택(LIFO).
- 빈 배열에서 부르면 내부 `RangeCheck(0)`에 걸려 크래시다. `Num() > 0` 검사가 그걸 막는다.
- `EAllowShrinking::No` — 기본값 `Yes`는 꺼낼 때마다 버퍼를 재할당할 수 있다.
  `ClearAll()`의 `Empty()`가 어차피 해제하므로 중간에 줄일 이유가 없다.
- 먼저 주운 것부터 팔고 싶으면(FIFO) 이 줄만 바꾼다:
  `UItemDataAsset* Item = Items[0]; Items.RemoveAt(0, 1, EAllowShrinking::No);`

---

## 3. 발판 헤더 — Source/ZombieHunter1/Public/Items/ItemSellZone.h

- [ ] 그전에 저거 발판 GagueZone 구현하자
파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSellZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMyPlayer;


// 밟고 있는 동안 가방을 한 개씩 파는 발판.
UCLASS()
class ZOMBIEHUNTER1_API AItemSellZone : public AActor
{
	GENERATED_BODY()

public:
	AItemSellZone();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// [컴포넌트]
	// 밟는 영역(트리거)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SellZone")
	UBoxComponent* TriggerBox;

	// 발판 바닥 메시(선택). BP에서 지정.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SellZone")
	UStaticMeshComponent* PadMesh;

	// [설정]
	// 아이템 한 개를 파는 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SellZone", meta = (ClampMin = "0.05"))
	float SellInterval = 0.3f;

	// [이벤트]
	// BP에서 동전 이펙트/사운드를 붙인다.
	UFUNCTION(BlueprintImplementableEvent, Category = "SellZone")
	void OnItemSold(int32 Price, int32 RemainingCount);

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// 한 개 판다. 가방이 비었으면 false.
	bool SellOneItem();

	// 이번에 발판에 올라와서 판 것들을 합쳐 한 줄로 알린다.
	void FlushSoldSummary();

	UPROPERTY()
	AMyPlayer* CurrentPlayer = nullptr;

	float SellTimer = 0.0f;

	int32 SoldCount = 0;
	int32 SoldTotal = 0;
};
```

---

## 4. 발판 구현 — Source/ZombieHunter1/Private/Items/ItemSellZone.cpp

파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemSellZone.h"
#include "Component/InventoryComponent.h"
#include "Items/ItemDataAsset.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"


AItemSellZone::AItemSellZone()
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


void AItemSellZone::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AItemSellZone::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AItemSellZone::OnTriggerEndOverlap);
	}
}


void AItemSellZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(CurrentPlayer))
	{
		CurrentPlayer = nullptr;
		return;
	}

	SellTimer += DeltaTime;
	if (SellTimer < SellInterval)
	{
		return;
	}

	SellTimer = 0.0f;

	if (!SellOneItem())
	{
		FlushSoldSummary();
	}
}


bool AItemSellZone::SellOneItem()
{
	UInventoryComponent* Bag = CurrentPlayer ? CurrentPlayer->GetBag() : nullptr;
	if (!Bag)
	{
		return false;
	}

	UItemDataAsset* Item = Bag->PopItem();
	if (!Item)
	{
		return false;
	}

	CurrentPlayer->GainMoney(Item->Price);

	SoldCount++;
	SoldTotal += Item->Price;

	OnItemSold(Item->Price, Bag->GetItems().Num());
	return true;
}


void AItemSellZone::FlushSoldSummary()
{
	if (SoldCount <= 0 || !IsValid(CurrentPlayer))
	{
		SoldCount = 0;
		SoldTotal = 0;
		return;
	}

	FText Msg = FText::Format(FText::FromString(TEXT("아이템 {0}개를 팔아 {1}원을 받았습니다.")), SoldCount, SoldTotal);
	CurrentPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);

	SoldCount = 0;
	SoldTotal = 0;
}


void AItemSellZone::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	AMyPlayer* MyPlayer = Cast<AMyPlayer>(OtherActor);
	if (!MyPlayer)
	{
		return;
	}

	CurrentPlayer = MyPlayer;
	SellTimer = SellInterval;  // 밟자마자 첫 한 개가 팔리게
}


void AItemSellZone::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (OtherActor != CurrentPlayer)
	{
		return;
	}

	FlushSoldSummary();
	CurrentPlayer = nullptr;
	SellTimer = 0.0f;
}
```

---

## 설계 메모

- **알림은 합쳐서 한 번.** 한 개 팔 때마다 `ShowOnItemText`를 부르면 10개 팔 때 알림이 10줄 쌓인다.
  개별 연출은 `OnItemSold`(BP 이벤트)로 넘기고, 텍스트는 다 팔렸을 때나 발판을 벗어날 때 한 줄로 낸다.
- **`SellTimer = SellInterval`** 로 시작해서 밟자마자 첫 개가 나간다. 0으로 두면 0.3초 멍하니 기다리게 된다.
- `AMoneyPadZone`을 상속하지 않는다. 그쪽은 돈을 **쓰는** 게이지고 여기는 **받는** 쪽이라 완성/쿨다운 개념이 없다.
- 가방은 `TArray` 하나로 충분하다. `TQueue`는 순회가 안 돼서 `GetCurrentWeight` / `GetTotalSellPrice`를
  만들 수 없고, `UPROPERTY`가 안 붙어 GC가 안에 든 UObject 포인터를 추적하지 못한다.
  스택이냐 큐냐는 컨테이너가 아니라 꺼내는 쪽 끝을 고르는 문제다.

## 다른 방식으로 가고 싶다면

- **즉발 전량** — Tick/타이머를 빼고 `OnTriggerBeginOverlap`에서 `while (SellOneItem()) {}` 후 `FlushSoldSummary()`
- **상시 판매** — `SellInterval`을 0에 가깝게 두면 사실상 같은 동작
