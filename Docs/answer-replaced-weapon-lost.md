# 답안지 — 더 좋은 무기를 주우면 전에 쓰던 무기가 사라지는 버그

## 진단 (기준: 로컬 소스 2026-09-23 21시)

무기가 **사라지는 줄**은 픽업이 아니라 캐릭터 쪽에 있다.

```cpp
// CombatCharacter.cpp — EquipWeaponItem
EquippedWeapon = Item;   // 이전 값을 어디에도 남기지 않고 덮어쓴다
```

```
AWeaponPickup::OnTriggerBeginOverlap
 └ Party->TryDistributeWeapon(새 무기)
    └ Receiver->EquipWeaponItem(새 무기)   ← 여기서 이전 무기가 증발
 └ Receiver가 있으면 → 가방 로직은 건너뛰고 Destroy
```

픽업은 **새 무기**의 행방만 처리한다(누가 쓰거나, 가방에 넣거나). **밀려난 무기**를 받아줄 코드가 아무 데도 없다.

### 어디서 고치나

| 후보 | 판단 |
|---|---|
| `EquipWeaponItem`이 이전 무기를 가방에 넣는다 | ✗ 캐릭터(동료 포함)는 가방을 모른다. 가방은 플레이어 것이다. `JobComponent`의 기본 무기 장착에도 같은 함수가 쓰인다 |
| `TryDistributeWeapon`이 장착 **전에** 이전 무기를 기억해 돌려준다 | ✓ 누가 받았는지 아는 유일한 곳 |
| 돌려받은 무기를 가방에 넣는 건 픽업이 한다 | ✓ "새 무기를 가방에 넣기"가 이미 여기 있다. 같은 자리에 둔다 |

### 가방이 꽉 차 있으면?

넣을 곳이 없다고 버리면 버그가 그대로 남는다. **이 픽업 액터가 밀려난 무기로 바뀌어 바닥에 남는다.** 새로 스폰할 필요가 없다 — 데이터와 메시만 바꾸고 `Destroy`하지 않으면 된다.

### 정해야 하는 것 (기획)

- 직업 **기본 무기**도 가방에 넣을지. 이 답안지는 **넣는다**(무엇도 사라지지 않는다는 규칙이 가장 단순, 팔 수도 있음).
  빼고 싶으면 5번에서 `Replaced != 기본 무기`일 때만 넣게 조건을 하나 추가한다.

작업 순서: 1 → 2 (파티) → 3 → 4 → 5 (픽업) → 6 (연관: 가방 무기 답안지) → 풀 리빌드 → PIE 확인

---

## 1. 밀려난 무기를 돌려주도록 선언 변경 — Source/ZombieHunter1/Public/Characters/PartyComponent.h:27

```cpp
// Before
	ACombatCharacter* TryDistributeWeapon(UWeaponDataAsset* Item);
```

```cpp
	// 무기 한 자루를 파티에 배분한다. 받은 캐릭터를 반환하고, 아무도 못 쓰면 nullptr.
	// OutReplaced: 받은 캐릭터가 원래 들고 있던 무기 (없으면 nullptr)
	ACombatCharacter* TryDistributeWeapon(UWeaponDataAsset* Item, UWeaponDataAsset*& OutReplaced);
```

---

## 2. 장착 전에 이전 무기 기억 — Source/ZombieHunter1/Private/Characters/PartyComponent.cpp:109

함수 전체 교체.

```cpp
ACombatCharacter* UPartyComponent::TryDistributeWeapon(UWeaponDataAsset* Item, UWeaponDataAsset*& OutReplaced)
{
	OutReplaced = nullptr;

	if (!Item)
	{
		return nullptr;
	}

	ACombatCharacter* OwnerCharacter = Cast<ACombatCharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->WantsWeaponItem(Item))
	{
		UWeaponDataAsset* Prev = OwnerCharacter->GetEquippedWeapon();
		if (OwnerCharacter->EquipWeaponItem(Item))
		{
			OutReplaced = Prev;
			NotifyWeaponTaken(Item, OwnerCharacter);
			return OwnerCharacter;
		}
	}

	// 쓸 수 있는 동료 중 지금 무기가 가장 약한 한 명 => 파티 전체 전투력 상승폭이 가장 크다
	ACompanion* Best = nullptr;
	for (ACompanion* Companion : Companions)
	{
		if (!IsValid(Companion) || Companion->GetIsDead() || !Companion->WantsWeaponItem(Item))
		{
			continue;
		}

		if (!Best || Companion->GetEquippedWeaponPower() < Best->GetEquippedWeaponPower())
		{
			Best = Companion;
		}
	}

	if (Best)
	{
		UWeaponDataAsset* Prev = Best->GetEquippedWeapon();
		if (Best->EquipWeaponItem(Item))
		{
			OutReplaced = Prev;
			NotifyWeaponTaken(Item, Best);
			return Best;
		}
	}

	return nullptr;
}
```

- `Prev`는 **`EquipWeaponItem` 호출 전에** 읽어야 한다. 호출 뒤에 `GetEquippedWeapon()`을 부르면 이미 새 무기다.
- `OutReplaced`는 장착이 **성공했을 때만** 채운다. 실패했는데 채우면 들고 있는 무기가 가방에도 복제된다.
- 함수 첫 줄에서 `nullptr`로 초기화한다. 호출자가 초기화를 잊어도 쓰레기값이 나가지 않는다.
- 기존의 `WantsWeaponItem(Item) && EquipWeaponItem(Item)` 한 줄짜리 조건은 사이에 `Prev`를 끼울 수 없어서 둘로 나눴다.

---

## 3. 픽업 헤더 — Source/ZombieHunter1/Public/Items/WeaponPickup.h:49

`private:`의 `EnablePickup` 아래에 추가.

```cpp
private:
	void EnablePickup();

	// 이 픽업을 다른 무기로 바꿔 바닥에 남긴다
	void BecomeWeapon(UWeaponDataAsset* NewItem);
```

---

## 4. 픽업이 다른 무기로 바뀌기 — Source/ZombieHunter1/Private/Items/WeaponPickup.cpp:112

파일 맨 끝(`EnablePickup` 아래)에 추가.

```cpp
void AWeaponPickup::BecomeWeapon(UWeaponDataAsset* NewItem)
{
	WeaponItemData = NewItem;
	WeaponMesh->SetSkeletalMeshAsset(NewItem ? NewItem->Mesh : nullptr);

	// 밟고 있는 채로 바로 다시 주워지지 않게 딜레이 후 다시 켠다
	TriggerBox->SetGenerateOverlapEvents(false);
	GetWorldTimerManager().SetTimer(PickupDelayHandle, this, &AWeaponPickup::EnablePickup, FMath::Max(PickupDelay, 0.5f));
}
```

- `bNoTakerNotified`는 건드리지 않는다. 5번에서 이미 "바닥에 내려놓았습니다"를 띄우고 `true`로 두므로, 딜레이 후 다시 겹쳤을 때 "가방이 가득 차…"가 한 번 더 뜨지 않는다.
- `PickupDelay`가 0이면 바로 다시 겹쳐 같은 프레임에 재진입할 수 있어 최소 0.5초를 보장한다.

---

## 5. 밀려난 무기를 가방으로 — Source/ZombieHunter1/Private/Items/WeaponPickup.cpp:87

87~106줄(`TryDistributeWeapon` 호출 ~ `if (!Receiver)` 블록 끝)을 교체.

```cpp
	// 누가 가져갈지(플레이어 우선, 없으면 동료)는 파티가 정한다. 획득 문구도 파티가 띄운다.
	UWeaponDataAsset* Replaced = nullptr;
	const ACombatCharacter* Receiver = Party->TryDistributeWeapon(WeaponItemData, Replaced);
	UInventoryComponent* Bag = MyPlayer->GetBag();

	if (!Receiver)
	{
		// 아무도 못 쓰면 가방으로. 무게가 넘치면 바닥에 그대로 남는다.
		if (!Bag || !Bag->TryAddItem(WeaponItemData))
		{
			if (!bNoTakerNotified)
			{
				bNoTakerNotified = true;
				FText Msg = FText::Format(FText::FromString(TEXT("가방이 가득 차 {0}을 넣을 수 없습니다.")), (WeaponItemData->Name));
				MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Blocked);
			}
			return;
		}

		FText Msg = FText::Format(FText::FromString(TEXT("{0}을 가방에 넣었습니다.")), (WeaponItemData->Name));
		MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);
	}
	else if (Replaced)
	{
		// 밀려난 무기 — 가방이 꽉 차면 이 픽업이 그 무기가 되어 바닥에 남는다
		if (!Bag || !Bag->TryAddItem(Replaced))
		{
			bNoTakerNotified = true;
			FText Msg = FText::Format(FText::FromString(TEXT("가방이 가득 차 {0}을 바닥에 내려놓았습니다.")), (Replaced->Name));
			MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Blocked);

			BecomeWeapon(Replaced);
			return;
		}

		FText Msg = FText::Format(FText::FromString(TEXT("{0}을 가방에 넣었습니다.")), (Replaced->Name));
		MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);
	}
```

- `Bag`을 `if` 밖으로 뺐다. 두 갈래가 모두 쓴다.
- `BecomeWeapon` 뒤에 `return` — 아래의 `Destroy()`까지 내려가면 방금 바닥에 남긴 무기가 같이 사라진다.
- `ShowOnItemText`는 `FText&`(const 아님)를 받으므로 `FText::Format(...)`을 인자에 바로 넣으면 컴파일 에러다. 지금처럼 `Msg` 변수에 담아 넘긴다.

---

## 6. (연관) 가방 무기 자동 장착에도 같은 구멍 — Docs/answer-companion-bag-weapon.md 5-2

아직 적용 전이라면, 그 답안지의 마지막 `if` 블록을 이걸로 바꿔서 옮긴다.
섭외된 동료가 가방 무기로 갈아낄 때도 기본 무기가 증발한다.

```cpp
// Before
	if (Best && Companion->EquipWeaponItem(Best))
	{
		Bag->RemoveItem(Best);
		NotifyWeaponTaken(Best, Companion);
	}
```

```cpp
	UWeaponDataAsset* Prev = Companion->GetEquippedWeapon();
	if (Best && Companion->EquipWeaponItem(Best))
	{
		Bag->RemoveItem(Best);
		NotifyWeaponTaken(Best, Companion);

		if (Prev)
		{
			Bag->TryAddItem(Prev);
		}
	}
```

- `RemoveItem` **뒤에** `TryAddItem` — 빈 자리가 생긴 다음에 넣어야 가방이 꽉 찬 상태에서도 들어갈 확률이 높다.
- 기본 무기가 꺼낸 무기보다 무거우면 여전히 안 들어갈 수 있다. 이 경우는 섭외 시점이라 바닥에 둘 픽업이 없어서 버린다(범위 밖 참고).

---

## 확인

1. 시작 무기를 든 채 더 센 같은 직업 무기를 줍는다 → "{새 무기}을 획득했습니다" + "{시작 무기}을 가방에 넣었습니다"
2. 판매 발판에서 판매 개수/금액이 1개 늘었는지
3. 가방을 무게 한도까지 채운 뒤 1을 반복 → 새 무기는 장착, "가방이 가득 차 {시작 무기}을 바닥에 내려놓았습니다", 바닥 메시가 시작 무기로 바뀜
4. 3의 바닥 무기 위에 계속 서 있어도 문구가 반복되지 않는지
5. 가방을 비우고(판매) 3의 무기를 다시 밟는다 → 가방에 들어감
6. 동료가 더 센 무기를 받을 때도 1과 같이 동료의 이전 무기가 가방으로 가는지

### 이번 범위 밖 (결정만 해두기)

- 밀려난 무기를 **다른 파티원에게 다시 배분**할지(플레이어의 옛 검을 전사 동료가 받는 식) — `Replaced`로 `TryDistributeWeapon`을 한 번 더 부르면 된다. 무기 공격력이 매번 낮아지므로 무한 반복은 없다
- 6번에서 기본 무기가 가방에 안 들어가는 경우 — 동료 발밑에 픽업을 스폰할지
