# 직업별 전용 스탯 설계 (2026-07-29)

목표: 직업(전사/궁수/마법사/힐러)마다 HP·이동속도·데미지 등 **전용 스탯**을 갖게 한다.
(할 일 목록의 "직업 데이터 구조(JobData) 설계 — 스탯·스킬·역할을 데이터로" 1단계)

## 결정 사항

**`USTRUCT FJobStats` 하나로 묶어서 `UJobComponent`가 멤버로 소유한다.**
값은 각 직업 서브클래스 생성자(또는 직업 BP 기본값)에서 채운다.

- DataAsset / DataTable 방식도 검토했으나, 지금 규모에선 오버킬. 구조체로 묶어두면
  나중에 `UJobDataAsset`으로 옮겨도 `Stats.MaxHP`를 읽는 코드는 안 바뀌므로 이전 비용이 싸다.
- **소유권 원칙: 직업이 원본(source of truth), 캐릭터는 적용받은 결과값을 들고 있다.**
  무기강화 등 런타임 증가분은 계속 직업 쪽에 얹는다(`UpgradeWeapon`이 이미 그렇게 동작).

## 핵심 구분 — 스탯은 두 종류

| 종류 | 예 | 처리 |
|---|---|---|
| (A) 직업이 직접 읽는 값 | Damage, AttackInterval, EngageRange | 컴포넌트가 들고 있으면 끝 |
| (B) 캐릭터/무브먼트가 소유한 값 | MaxHP, MaxWalkSpeed | 저장만으론 무효 → **초기화 시 캐릭터로 밀어넣어야(적용)** 함 |

(B) 때문에 오늘 만들 핵심은 저장소가 아니라 **적용 함수**다. 자리는 이미 있음:
`UJobComponent::InitializeForOwner` (지금은 `EquipWeapon()` 하나만 호출 중 — 무기 끼우기와 같은 패턴).

## 작업 순서

1. **`FJobStats` 정의** — `JobComponent.h`의 `EJobType` 아래.
   - `USTRUCT(BlueprintType)` + 내부 `GENERATED_BODY()`
   - 필드마다 `UPROPERTY(EditAnywhere, BlueprintReadWrite)` (구조체에 한 번 붙인다고 안쪽까지 노출 안 됨)
   - 컴포넌트에 `UPROPERTY(EditAnywhere, Category="Job|Stats") FJobStats Stats;`

2. **기존 필드 처리 결정** — `UJobComponent`의 `Damage`/`AttackInterval`/`EngageRange` 참조처:
   - `Companion.cpp:85, 91-92` (EngageRange·AttackInterval 폴백)
   - `MyPlayer.cpp:428` (AttackInterval), `MyPlayer.cpp:646` (강화 시 Damage 증가)
   - `JobComponent.cpp:100` (발사체에 Damage 전달), `WarriorJob.cpp:53` (근접 피해)

   (a) 통째로 `Stats.` 안으로 이동(6곳 수정, 스탯 한 군데로 정리) ← 권장
   (b) 새 스탯만 `Stats`에 넣고 기존은 유지(안 깨지지만 스탯이 두 군데 남음)

3. **`ApplyStatsToOwner()`** — `UJobComponent` protected에 추가, `InitializeForOwner`에서 호출.
   `ACombatCharacter`로 캐스팅해 MaxHP 적용 + `GetCharacterMovement()->MaxWalkSpeed = Stats.MoveSpeed`.
   (`GameFramework/CharacterMovementComponent.h` include 필요)
   → **보상**: `MyPlayer.cpp:245`와 `Companion.cpp:52`가 이미 둘 다 `InitializeForOwner`를 부르므로,
     여기 한 곳만 손대면 플레이어·동료에 동시 적용된다. 캐릭터 클래스는 안 건드림.

4. **4직업 생성자에 값 채우기** — `UWarriorJob::UWarriorJob()`의 기존 `EngageRange = 130.f` 자리에 `Stats.`로.

5. **인게임 확인** — 전사/궁수로 각각 시작해 HP 최대치·이동속도 차이, 발판에서 소환한 동료도 같은 값을 받는지.

## 지뢰

1. **`MyPlayer.cpp:252`의 `CurrentJob->Damage = Damage;`** — 캐릭터 값이 직업 값을 덮는다.
   `InitializeForOwner`(245줄) **뒤에** 있음. 방향을 뒤집든 이 줄을 지우든 결정 필요.
2. **MaxHP만 바꾸면 5/12로 시작** — `HP`는 별도 필드(`CombatCharacter.h:36`, 기본 5).
   `SetHP()`(HP바 갱신+죽음 판정 포함)를 쓸지 직접 대입할지 선택.
3. **`AEnemy`는 직업이 없다** — 적은 BP 값 유지됨(의도대로). 나중에 적에게 직업을 붙이면 재검토.
4. **직업 교체 시나리오**(할 일의 "스폰존 랜덤 직업") — 강화 Damage와 현재 HP 비율 승계 문제가 생긴다.
   지금 풀 필요는 없고, `ApplyStatsToOwner`를 "BeginPlay 전용"이 아니라 언제 불러도 되게 짜두면 그때 편함.

## 다음: 전사 AI

전사 AI("가까운/위협적인 적 막기·공격")가 쓸 값(리더 앞 진형 거리, 적 가로채기 반경 등)을
공용 `FJobStats`에 넣을지 `UWarriorJob` 전용 필드로 둘지는 스탯 구조를 끝내고 정한다.
