# 답안지 — 판매 발판 게이지식 전환

즉발 전량 판매 → **발판 위에 있는 동안 한 개씩** 판매로 변경.
기존 발판(`AMoneyPadZone`)들과 조작감을 맞추고, 중간에 벗어나면 멈춰서 실수를 되돌릴 수 있게 한다.

작업 순서: 1 → 2 (가방 정리 + 꺼내기 API) → 3 → 4 (발판) → 풀 리빌드 → 에디터 확인

> **보류 중인 선행 작업**
> - [ ] 그전에 발판 공통 베이스 `AGaugeZone`을 먼저 구현할지 결정 (아래 설계 메모 참고).
>   지금 답안지는 `AItemSellZone : AActor` 기준으로 쓰여 있다.

---

## 1. 가방 컴포넌트 정리 + 꺼내기 API — Source/ZombieHunter1/Public/Component/InventoryComponent.h

파일 전체 교체. `PopItem()` 추가와 함께, 실제로 쓰이지 않는 블루프린트 노출 매크로를 걷어낸다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemDataAsset;


// 아이템 보관 가방. 인벤토리 창은 없고 판매 발판이 한 개씩 꺼내 판다.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIEHUNTER1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 무게가 넘치면 안 넣고 false
	bool TryAddItem(UItemDataAsset* Item);

	// 상태를 바꾸지 않는 질문용. 넣을 때는 TryAddItem을 쓴다.
	bool HasRoomFor(UItemDataAsset* Item) const;

	float GetCurrentWeight() const;

	// 가방 전체를 팔았을 때 받는 금액
	int32 GetTotalSellPrice() const;

	void ClearAll();

	// 가방에서 한 개 꺼낸다. 비어 있으면 nullptr.
	UItemDataAsset* PopItem();

	FORCEINLINE const TArray<UItemDataAsset*>& GetItems() const { return Items; }
	FORCEINLINE float GetMaxWeight() const { return MaxWeight; }

private:
	// 담을 수 있는 총 무게
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"), Category = "Inventory")
	float MaxWeight = 20.0f;

	// UPROPERTY는 GC가 배열 안 UObject를 추적하게 하는 용도
	UPROPERTY(Transient)
	TArray<UItemDataAsset*> Items;
};
```

### 왜 매크로를 걷어내는가

`UFUNCTION` / `UPROPERTY`는 "블루프린트에 연다"가 아니라 **"엔진 리플렉션에 등록한다"**는 뜻이다.
BP 노출은 그 용도 중 하나일 뿐이고, 필요 없는데 붙이면 API 표면만 넓어진다.
BP가 노드를 한 번 물면 나중에 이름을 바꾸거나 지울 때 BP가 컴파일 에러로 터진다.

프로젝트 전체에서 맨 `UFUNCTION()`이 붙은 함수 11개 중 10개가 `On___` 콜백이고,
전부 `AddDynamic`으로 델리게이트에 꽂혀서 **떼면 컴파일 에러가 나는** 것들이다.
`ClearAll`만 그 패턴 밖에 있었다. => `CombatCharacter.h`의 `UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.` 참고

| 대상 | 판단 | 근거 |
|---|---|---|
| `TryAddItem` `ClearAll` `PopItem` | 매크로 제거 | 델리게이트도 아니고 호출처가 전부 C++ |
| `HasRoomFor` `GetCurrentWeight` `GetTotalSellPrice` | `BlueprintPure` 제거 | BP 참조 0곳. 위젯이 실제로 필요해질 때 다시 연다 |
| `MaxWeight`의 `EditAnywhere` | **유지** | 디테일 패널에서 값 조정. BP 노출과 무관한 에디터 기능 |
| `MaxWeight`의 `BlueprintReadWrite` | 제거 | BP 그래프에서 읽고 쓸 일이 없음 |
| `Items`의 `UPROPERTY` | **유지** | GC가 배열 안 `UObject*`를 추적하게 하는 용도. 떼면 조용히 수거돼 크래시 |
| `Items`의 `BlueprintReadOnly` | 제거 | BP 참조 0곳 |

`AllowPrivateAccess`는 private 멤버를 BP/에디터에 노출할 때만 필요한 메타다.
`Items`는 노출이 사라지므로 같이 빠지고, `MaxWeight`는 `EditAnywhere`가 남으므로 유지한다.

되돌리는 비용이 비대칭이라는 점이 기준이다. 나중에 붙이는 건 1초지만,
BP가 이미 물고 있는 걸 떼는 건 BP를 고쳐야 한다.

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

## 3. 발판 헤더 — Source/ZombieHunter1/Public/Zones/ItemSellZone.h

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

여기 매크로는 1번과 달리 **뗄 수 없는 것들**이다. 기준이 뒤집힌 게 아니라 용도가 다르다.

- `OnTriggerBeginOverlap` / `OnTriggerEndOverlap`의 맨 `UFUNCTION()`
  => `AddDynamic`이 리플렉션으로 함수를 찾으므로 없으면 컴파일 에러
- `OnItemSold`의 `BlueprintImplementableEvent`
  => 구현이 BP 쪽에 있다. 동전 이펙트/사운드를 붙이는 지점

---

## 4. 발판 구현 — Source/ZombieHunter1/Private/Zones/ItemSellZone.cpp

파일 전체 교체.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Zones/ItemSellZone.h"
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
  => 대신 추후 공통 베이스 `AGaugeZone`을 만들어 `MoneyPadZone`/`ItemSellZone`/퀘스트 존이 함께 상속한다.
     동작 확인 후 별도 작업으로 진행 (`Docs/answer-gauge-zone.md`).
- 가방은 `TArray` 하나로 충분하다. `TQueue`는 순회가 안 돼서 `GetCurrentWeight` / `GetTotalSellPrice`를
  만들 수 없고, `UPROPERTY`가 안 붙어 GC가 안에 든 UObject 포인터를 추적하지 못한다.
  스택이냐 큐냐는 컨테이너가 아니라 꺼내는 쪽 끝을 고르는 문제다.

## 다른 방식으로 가고 싶다면

- **즉발 전량** — Tick/타이머를 빼고 `OnTriggerBeginOverlap`에서 `while (SellOneItem()) {}` 후 `FlushSoldSummary()`
- **상시 판매** — `SellInterval`을 0에 가깝게 두면 사실상 같은 동작
