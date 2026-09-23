# 답안지 — 동료 섭외 시 가방 무기 자동 장착

## 진단 (기준: `gestgest/rotforest` 2184520 "add debug weapon pickup")

**미구현.** 무기 배분은 **주울 때 한 번**만 일어나고, 동료가 **새로 들어올 때**는 가방을 보지 않는다.

| 시점 | 코드 | 가방 무기를 보는가 |
|---|---|---|
| 무기 주움 | `AWeaponPickup::OnTriggerBeginOverlap` → `UPartyComponent::TryDistributeWeapon` → 아무도 못 쓰면 `Bag->TryAddItem` | 넣기만 함 |
| 동료 섭외 | `ACompanionSpawnZone::HandleZoneFilled` → `UPartyComponent::RecruitCompanion` | **안 봄** |

재현: 궁수 동료가 없을 때 궁수 무기를 주우면 가방으로 간다 → 그 뒤 궁수 동료를 섭외해도 기본 무기(`DefaultWeapon`)만 든다.
가방 무기는 판매 발판에서 팔려 사라지기 전까지 계속 쓰이지 않는다.

### 핵심 포인트

- **호출 위치는 `FinishSpawningActor` 뒤여야 한다.**
  직업(`CurrentJob`)은 동료 `BeginPlay` → `CreateJobComponent()`에서 생긴다. 지연 스폰에서 `BeginPlay`는 `FinishSpawningActor` 안에서 돈다.
  그 전에 부르면 `WantsWeaponItem`이 `!CurrentJob`에 걸려 항상 `false` → 조용히 아무 일도 안 일어난다.
- `DefaultWeapon`도 같은 `BeginPlay`에서 장착되므로, 그 뒤에 부르면 "기본 무기보다 센가" 비교(`WantsWeaponItem`)가 그대로 맞게 동작한다.
- **장착 성공 뒤에 가방에서 뺀다.** 순서를 뒤집으면 장착 실패 시 무기가 증발한다.
- 가방에서 뺄 때 `RemoveSingle` — 같은 에셋이 여러 개 들어 있을 수 있으니 한 개만 빼야 한다. `Remove`는 전부 뺀다.
  `RemoveSingleSwap`은 순서를 섞어서 `PopItem`(스택)의 판매 순서가 바뀐다.
- 원래 들고 있던 기본 무기는 가방에 넣지 않는다. `TryDistributeWeapon`도 교체된 무기를 버리므로 그것과 규칙을 맞춘다.

작업 순서: 1 → 2 (가방) → 3 → 4 → 5 (파티) → 풀 리빌드(헤더에 함수 추가) → PIE 확인

---

## 1. 가방에서 한 개 빼기 선언 — Source/ZombieHunter1/Public/Component/InventoryComponent.h:33

`PopItem()` 아래에 추가.

```cpp
	UItemDataAsset * PopItem(); //스택식 pop

	// 같은 에셋이 여러 개면 하나만 뺀다
	bool RemoveItem(UItemDataAsset* Item);
```

---

## 2. 가방에서 한 개 빼기 구현 — Source/ZombieHunter1/Private/Component/InventoryComponent.cpp:84

파일 맨 끝(`PopItem()` 아래)에 추가.

```cpp
bool UInventoryComponent::RemoveItem(UItemDataAsset* Item)
{
	return Items.RemoveSingle(Item) > 0;
}
```

---

## 3. 파티 헤더에 함수 선언 — Source/ZombieHunter1/Public/Characters/PartyComponent.h:59

`// [장비]` 섹션, `NotifyWeaponTaken` 아래에 추가.

```cpp
	// [장비]
	// 무기를 누가 가져갔는지 플레이어 UI에 알린다.
	void NotifyWeaponTaken(UWeaponDataAsset* Item, ACombatCharacter* Receiver) const;

	// 가방에서 이 동료가 쓸 수 있는 가장 강한 무기를 꺼내 장착시킨다.
	void EquipBestWeaponFromBag(ACompanion* Companion);
```

---

## 4. 섭외 직후 호출 — Source/ZombieHunter1/Private/Characters/PartyComponent.cpp:46

`RecruitCompanion` 끝부분.

```cpp
// Before
	UGameplayStatics::FinishSpawningActor(Companion, SpawnTM);

	Companions.Add(Companion);
}
```

```cpp
	UGameplayStatics::FinishSpawningActor(Companion, SpawnTM);

	Companions.Add(Companion);

	// BeginPlay에서 직업이 생긴 뒤여야 한다
	EquipBestWeaponFromBag(Companion);
}
```

---

## 5. 가방 무기 고르기 구현 — Source/ZombieHunter1/Private/Characters/PartyComponent.cpp

### 5-1. include — 11번째 줄 근처

```cpp
#include "Component/InventoryComponent.h" //섭외 시 가방 무기 장착
```

### 5-2. 함수 — 파일 맨 끝(`NotifyWeaponTaken` 아래)에 추가

```cpp
void UPartyComponent::EquipBestWeaponFromBag(ACompanion* Companion)
{
	AMyPlayer* Player = Cast<AMyPlayer>(GetOwner());
	UInventoryComponent* Bag = Player ? Player->GetBag() : nullptr;
	if (!Bag || !IsValid(Companion))
	{
		return;
	}

	UWeaponDataAsset* Best = nullptr;
	for (UItemDataAsset* Item : Bag->GetItems())
	{
		UWeaponDataAsset* Weapon = Cast<UWeaponDataAsset>(Item);
		if (!Weapon || !Companion->WantsWeaponItem(Weapon))
		{
			continue;
		}

		if (!Best || Weapon->WeaponPower > Best->WeaponPower)
		{
			Best = Weapon;
		}
	}

	if (Best && Companion->EquipWeaponItem(Best))
	{
		Bag->RemoveItem(Best);
		NotifyWeaponTaken(Best, Companion);
	}
}
```

- 루프 안에서 `RemoveItem`을 부르지 않는다. `GetItems()`는 원본 배열의 참조라, 순회 중에 빼면 범위 기반 for가 깨진다(에디터에서 `ensure`/크래시).
- `Cast<UWeaponDataAsset>` — 가방에는 무기 외 아이템도 들어갈 수 있으니 무기만 거른다.
- 획득 문구는 기존 `NotifyWeaponTaken`을 그대로 쓴다 => "궁수 동료가 {무기}을 획득했습니다."

---

## 확인

1. 궁수 동료가 없는 상태에서 궁수 무기(디버그 픽업)를 줍는다 → "가방에 넣었습니다"
2. 궁수 동료 소환 발판을 채운다 → "궁수 동료가 ~을 획득했습니다" + 동료 왼손 메시가 바뀜
3. 판매 발판에서 팔리는 개수가 1 줄었는지 확인
4. 반대 케이스: 가방 무기의 `WeaponPower`가 직업 기본 무기 이하이면 아무 일도 안 일어나야 정상

### 이번 범위 밖 (결정만 해두기)

- 이미 파티에 있는 동료가 **죽고 새로 뽑히기 전**, 혹은 플레이어가 직업을 바꿨을 때도 가방을 다시 볼지
- 교체된 기존 무기를 가방에 돌려넣을지 (지금은 `TryDistributeWeapon`과 같이 버림)
