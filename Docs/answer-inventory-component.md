# 가방 + 무기 DataAsset 전환 기록 (2026-09-21)

`FWeaponItemData`(USTRUCT) → `UWeaponDataAsset`(DataAsset) 전면 전환. 코드는 Claude가 직접 수정함.
아이템 종류가 늘어날 것이 확정돼 struct 컴포지션 방식을 버리고 진짜 다형성으로 감.

---

## 리빌드 전에 먼저 할 것

**빌드하는 순간 BP에 저장된 무기 값이 전부 None이 된다.** 타입이 바뀌어서 리다이렉트로도 살릴 수 없다.
아직 옛 DLL이 살아있으니 **에디터를 열어 아래 값들을 먼저 메모/스크린샷**으로 남겨라.

| 에셋 | 적어둘 것 |
|---|---|
| `BP_WarriorJob` / `BP_ArcherJob` / `BP_MageJob` / `BP_HealerJob` | `DefaultWeapon`의 WeaponName / WeaponPower / Mesh / JobType |
| `BP_KnifePickup` | `WeaponItemData` 전체 |
| 보스 BP | `DropTable`이 가리키던 행 이름들 |
| `DT_WeaponDataTable` | 모든 행의 값 (에셋째로 사라질 예정) |

---

## 새 타입

`Public/Items/ItemDataAsset.h`

```
UItemDataAsset : UPrimaryDataAsset
  FText Name
  float Weight = 1.0      // 가방 무게
  int32 Price  = 0        // 판매 금액

UWeaponDataAsset : UItemDataAsset
  int32 WeaponPower
  USkeletalMesh* Mesh
  EJobType JobType
```

`Weight` / `Price`가 베이스에 있어서, 나중에 포션·재료를 추가해도 가방과 판매 발판은 손댈 필요가 없다.

## 삭제된 파일

- `Public/Items/WeaponItemData.h` (`FWeaponItemData`)
- `Public/Items/ItemData.h`, `Private/Items/ItemData.cpp` (`FItemData` — DataAsset으로 가면서 불필요)

## 수정된 파일

| 파일 | 변경 |
|---|---|
| `CombatCharacter.h/.cpp` | `EquippedWeapon`이 포인터로. `EquipWeaponItem` / `WantsWeaponItem`이 `UWeaponDataAsset*`를 받음. `GetEquippedWeaponPower()` 추가 |
| `JobComponent.h/.cpp` | `DefaultWeapon`이 포인터로. `GetDefaultWeaponMesh()`가 .cpp로 이동(전방 선언만으론 역참조 불가) |
| `PartyComponent.h/.cpp` | `TryDistributeWeapon` / `NotifyWeaponTaken` 시그니처 |
| `WeaponPickup.h/.cpp` | `WeaponItemData`가 포인터로. 아무도 못 쓰면 가방으로 |
| `Boss.h/.cpp` | `DropTable`이 `TArray<FDataTableRowHandle>` → `TArray<UWeaponDataAsset*>` |
| `MyPlayer.h/.cpp` | `Bag` 컴포넌트 + `GetBag()` + `GainMoney(int32)` |

## 새 파일

- `Public/Component/InventoryComponent.h` + `Private/` — `TryAddItem` / `HasRoomFor` / `GetCurrentWeight` / `GetTotalSellPrice` / `ClearAll`
- `Public/Items/ItemSellZone.h` + `Private/` — 밟으면 가방 전량 판매

---

## 에디터 작업

1. **풀 리빌드** (새 UCLASS 3개 — Live Coding 금지)
2. `Content/DataTable/WeaponDataTable.uasset` **삭제** — 행 구조체가 사라져서 열 수 없다
3. 무기 에셋 생성: 콘텐츠 브라우저 우클릭 → Miscellaneous → Data Asset → `WeaponDataAsset` 선택 → `DA_기존무기이름`
   - 1번에서 메모해둔 값을 채운다. `Weight`(기본 1) / `Price`(판매가)는 새로 정해야 함
4. `BP_WarriorJob` 외 직업 BP 3개 → `DefaultWeapon`에 해당 DA 지정
5. `BP_KnifePickup` → `WeaponItemData`에 DA 지정
6. 보스 BP → `DropTable`에 DA들 지정
7. `BP_ElfArden` → `Bag` 컴포넌트가 자동 생성됨. `MaxWeight` 확인 (기본 20)
8. `BP_ItemSellZone` 생성 (부모 `ItemSellZone`) → `PadMesh` 지정
9. 마을 청크에 배치 — `InfiniteMapGenerator`의 마을 생성 쪽에 스폰 추가 필요 (미구현)

## 남은 것

- 판매 발판을 마을에 스폰하는 코드 (9번)
- DataAsset은 정의를 공유한다. 무기별 강화 수치처럼 **개체마다 다른 값**이 생기면 에셋에 쓰면 안 되고,
  `{ UItemDataAsset* Def; int32 UpgradeLevel; }` 같은 인스턴스 구조체를 따로 둬야 한다.
  지금은 강화가 플레이어 소유(`WeaponLevel`)라 해당 없음.
