# RotForest — 게임플레이 문서

<img width="400" height="225" alt="bandicam 2026-08-20 20-49-07-148" src="https://github.com/user-attachments/assets/d0055de6-78cc-4984-8e4d-6f7dd528ca51" />

---
## 기본 시스템
### 전투 시스템
![bandicam 2025-12-14 21-58-13-948](https://github.com/user-attachments/assets/6d7ee84f-4018-4288-8023-b7dcdfd99186)
- 적은 플레이어와 마주치면 따라갑니다.
- 사거리 안의 가장 가까운 적을 자동으로 조준합니다. 조준된 적에는 림라이트 오버레이가 걸리며, 머리·투구·갑옷 같은 모듈형 메시 전체에 적용됩니다.

![](https://velog.velcdn.com/images/gestgest/post/8ba40ec7-7dbb-4a20-8e18-98ed90b50f8d/image.png)

- 주변에 적이 없으면 조이스틱(이동) 방향으로 공격합니다.


### 코인 수집
![bandicam 2025-12-14 21-58-13-948 (1)](https://github.com/user-attachments/assets/0637a540-b307-41fb-8c95-13b5a1e2f602)
- 맵에 배치된 코인을 수집하여 점수를 획득합니다.

### 사망 연출
<img width="400" height="225" alt="bandicam 2026-08-21 _dead" src="https://github.com/user-attachments/assets/ffb8ab75-1727-42fe-bd47-4df18ae515f3" />

플레이어가 죽으면 화면 속 적들의 시간이 멈춥니다.
게임모드가 적 전체에 `CustomTimeDilation = 0`을 걸고 스폰·리쉬 타이머까지 일시정지시킨 뒤 사망 패널을 띄웁니다.


### 적, 코인 생성
![bandicam 2026-02-13 00-58-35-815_success](https://github.com/user-attachments/assets/f7958179-a355-402e-aa4b-c12fcf40be96)

오브젝트 풀링 방식을 활용해서 적과 코인을 시야 밖 링에 생성합니다.
레벨이 오를수록 동시에 깔리는 수가 늘어납니다.

| | 레벨 1 | 레벨당 | 최대 |
|---|---|---|---|
| 적 | 5 | +10 | 30 |
| 코인 | 5 | +5 | 20 |

---

## 직업 시스템

| 직업 | 공격 방식 | MaxHP | 데미지 | 이동속도 | 공격 간격 |
|---|---|---|---|---|---|
| 전사 | 전방 120° 부채꼴(다중 타격 + 넉백) | 25 | 1 | 700 | 0.6s |
| 궁수 | 화살 발사, 시위 당김 애니 연동 | 10 | 1 | 900 | 1.3s |
| 마법사 | 느린 발사체 + 착탄 지점 범위 폭발 | 8 | 4 | 600 | 1.2s |
| 힐러 | 공격 대신 전방 아군 회복(적에겐 무효) | 8 | 1 | 600 | 10s |

<table>
<tr>
<td><img width="400" src="https://github.com/user-attachments/assets/ed01960d-9ce0-4ca4-9700-00d44e2d865e" /><br>전사</td>
<td><img width="400" src="https://github.com/user-attachments/assets/c3bc110d-14d7-4647-928f-d3e0bb9076ac" /><br>궁수</td>
</tr>
<tr>
<td><img width="400" src="https://github.com/user-attachments/assets/62562274-254a-4805-838c-efb248ca21af" /><br>마법사</td>
<td><img width="400" src="https://github.com/user-attachments/assets/19722ab5-d22d-4d89-916c-114a040a2b29" /><br>힐러</td>
</tr>
</table>

- 플레이어와 동료가 같은 `UJobComponent`를 씁니다. 직업을 바꾸면 컴포넌트만 교체되고, 캐릭터 코드는 그대로입니다.
- 궁수는 몽타주가 끝나는 순간(시위를 놓는 순간) 화살을 발사하고, 활 시위 애니메이션의 재생 속도를 몽타주 남은 시간에 맞춰 자동으로 계산합니다.
- 궁수는 공격 몽타주 동안 몸 전체를 `AttackYawOffset`만큼 돌려 활이 정면을 향하게 합니다. 무기를 쥐는 소켓과 각도는 무기 데이터 에셋에서 가져옵니다.
- 전사는 공격 방향과 적 방향의 내적을 `cos(SwingAngle/2)`와 비교해 부채꼴 안의 적을 모두 때립니다. 적 캡슐 반지름까지 계산에 넣어, 덩치 큰 적은 가장자리에 걸쳐도 맞습니다.

---

## 무한 맵 생성

<img width="400" height="225" alt="bandicam 2026-08-20 20-49-07-148 (1)" src="https://github.com/user-attachments/assets/3f01b599-75c6-4eb7-8f05-f91ebcc5cd05" />

레벨에 배치된 `AInfiniteMapGenerator` 하나가 플레이어를 따라다니며 지형을 만들고 지웁니다.

- 같은 시드·같은 좌표는 언제나 같은 결과를 내므로, 떠났다 돌아와도 그 자리에 같은 지형이 다시 깔립니다. 마인크래프트식 청크 생성 시스템이라고 보면 됩니다.
- 청크가 스폰한 액터는 `FMapChunk`에 묶여 있어 언로드 시 한 번에 정리됩니다.
- NavMesh 동적 생성: 런타임에 생성된 바닥 위에서도 길찾기가 되도록, 플레이어에 `UNavigationInvokerComponent`를 붙이고 `NavMeshBoundsVolume`을 플레이어를 따라 옮기며 내비게이션 시스템에 경계 변경을 통지합니다.
- 스폰존이나 보스 처치 같은, 기록해야 하는 정보는 POIStateStore로 저장합니다.
- 플레이어 주변 7x7 청크를 안개로 덮고, 플레이어가 청크를 옮기면 안개도 따라 이동합니다.

---
### POI

<img width="400" alt="image" src="https://github.com/user-attachments/assets/6c43baba-e7c6-45ab-8e34-7a7e0ba28ff3" />

[POI 설명 사이트](https://velog.io/@gestgest/Unreal-%EC%B2%AD%ED%81%AC%EC%8B%9C%EC%8A%A4%ED%85%9C2)

마인크래프트를 예로 들면, 마을 한가운데에 오브젝트가 랜덤하게 배치되면 매우 어색합니다.
그래서 POI라는 큰 구역을 따로 두고, 그 구역 안에는 랜덤 오브젝트를 넣지 않는 대신 마을처럼 정해진 오브젝트를 배치합니다.

#### 마을
<img width="400" height="225" alt="bandicam 2026-08-20_villege" src="https://github.com/user-attachments/assets/e40c3ec0-ae6e-4f82-9966-e17d2a481560" />

- 경비병(`AGuardCharacter`): 리더를 따라다니는 대신 자기 자리를 지키고, 교전이 끝나면 원위치로 복귀합니다.
- 주민(`Villager`): 비전투 NPC. 마을 주변을 배회하고, 피해를 받을 수 있습니다. 로봇 모델에서 사람 모델로 교체했습니다.

![](https://velog.velcdn.com/images/gestgest/post/561c0c27-ce36-42ef-ab58-16343dcda5b2/image.png)

- 적 스폰 링은 마을을 피해서 생성됩니다. 게임모드가 맵 생성기에 질의해 마을 안에는 스폰하지 않습니다.
- 마을 외곽 구조물은 고정 슬롯 6곳에 배치되며, 구조물끼리 겹치지 않습니다.

#### 역병마을

![](https://velog.velcdn.com/images/gestgest/post/e51ca240-a224-425b-8227-4c92dec19bc6/image.png)

- 적이 밀집해 있고, 보스(`Boss`)가 등장합니다.
- 보스는 자신이 속한 마을의 청크 좌표를 알고 있어서, 죽을 때 그 좌표를 클리어로 기록합니다.
- 맵 생성기가 POI를 만들 때 촛불과 장식물을 원형으로 배치하고, 전용 바닥 머티리얼(`M_PlagueGround`, `MI_PlagueVillage`)을 깝니다.

#### 상태 영속
청크가 사라져도 진행도는 남습니다.
청크가 언로드되면 액터는 전부 파괴됩니다. 지형은 시드로 다시 생성되지만, 액터에 담겨 있던 상태(발판에 넣은 돈, 보스 처치 여부)까지 자동으로 남지는 않습니다.
그래서 이 값들은 `FPOIStateStore`(POI 중심 좌표 → 상태 맵)에 따로 보관합니다.

---

## 보스

![](https://velog.velcdn.com/images/gestgest/post/a7e6af25-9797-4d3b-89c7-cb5d6483b1ef/image.png)

### 돌진 패턴

![](https://velog.velcdn.com/images/gestgest/post/b7a94374-aa2b-464a-b54c-4c1affd71cac/image.gif)

보스는 네 가지 상태를 순서대로 돕니다.

```
Chase(추격) → Windup(노려봄) → Charge(돌진) → Recover(숨고르기) → Chase
```

- 추격 중 1.2초마다 30% 확률로 돌진을 시작합니다.
- 돌진 방향은 Windup(0.5초)이 끝나는 순간 고정됩니다. 노려보는 동안 플레이어가 피할 여지를 줍니다.
- 돌진에 맞으면 평타의 2배 데미지와 넉백을 받습니다.
- 돌진은 시간이 다 되거나 마을 울타리 밖으로 나가려는 순간 끝납니다. 돌진 한 번에 같은 대상은 한 번만 맞습니다.
- 처치하면 정해진 무기 중 하나를 바닥에 떨굽니다.

### 전투 진입 · 이탈
- 플레이어가 15m(`CombatRadius`) 안에 들어오면 전투가 시작되고, 30m(`LeaveCombatRadius`) 밖으로 벗어나면 전투가 끝나며 HP바가 사라집니다.
- 진입과 이탈 반경을 따로 둬서, 돌진으로 보스가 시야 밖까지 나가도 플레이어를 놓치지 않습니다.

### 보스 HP바

![](https://velog.velcdn.com/images/gestgest/post/316aebf5-10b6-43af-b09a-3163394734c4/image.gif)

- `OnHPChanged` 델리게이트로 UI가 보스를 구독합니다.
- 빨간 바는 피격 즉시 줄고, 노란 잔상 바는 0.4초 버틴 뒤 따라서 줄어듭니다.
- 받은 데미지를 텍스트로 띄우고, 보스가 죽으면 HP바가 자동으로 사라집니다.
- HP바는 `UHPBar` 클래스 하나로 플레이어·보스가 같이 씁니다.

### 화면 밖 보스 화살표

![](https://velog.velcdn.com/images/gestgest/post/bbbef4be-7f8a-4ba1-b89a-b181929a5efa/image.png)

보스 위치를 화면 좌표로 투영해서, 화면 밖이면 가장자리에 화살표를 붙이고 보스 방향으로 회전시킵니다.

```cpp
// 화면 중심 → 보스 방향
FVector2D Dir = Screen - Center;

// 가장 먼저 닿는 화면 벽까지의 비율
const float T = FMath::Min(
    Half.X / FMath::Abs(Dir.X),
    Half.Y / FMath::Abs(Dir.Y));
ArrowSlot->SetPosition(Center + Dir * T);
```

---

## 성장
![](https://velog.velcdn.com/images/gestgest/post/9610f32a-e96b-4ccd-8a97-afc431f9e07b/image.png)

코인으로 모은 돈은 발판 위에 서 있는 동안 소비됩니다.
- 발판 위 게이지(`WBP_ZoneGauge`, C++ `UZoneGaugeWidget`)에 진행률 · 이름 · 비용을 표시합니다.
- 동료 모집 발판은 소환될 직업의 아이콘과 이름(예: Archer → 궁수)을 띄웁니다.

```
AGaugeZone (밟으면 게이지가 차는 공통 베이스)
 ├─ AMoneyPadZone (돈을 내는 발판)
 │   ├─ ACompanionSpawnZone  → 랜덤 직업 동료 소환
 │   └─ AWeaponUpgradeZone   → 무기 강화 (강화할수록 다음 비용 상승)
 └─ AItemSellZone            → 가방 속 아이템 판매
```

- 진행도는 `FPOIStateStore`에 저장되므로, 청크가 언로드되어도 이어서 채울 수 있습니다.
- 결제 간격·게이지·쿨다운·일회성 여부·디버그 바는 베이스가 처리하므로, 새 발판은 `TryFillOnce()`와 `HandleZoneFilled()`만 구현하면 추가됩니다.

---

## 무기 · 아이템

### 데이터 에셋
```
UItemDataAsset   : 이름 · 무게 · 판매 가격
 └─ UWeaponDataAsset : 공격력 · 메시 · 사용 가능 직업 · 쥐는 각도/위치
```
- 새 무기는 코드 수정 없이 데이터 에셋만 추가하면 됩니다.
- 무기마다 손에 쥐는 각도(`GripRotation`)와 위치(`GripLocation`)를 에셋에 저장해, 메시마다 다른 축 방향을 에셋에서 맞춥니다.

### 무기 줍기와 파티 배분
- 바닥의 무기(`WeaponPickup`)를 밟으면 플레이어가 먼저 판단하고, 플레이어가 안 쓰면 같은 직업 동료 중 무기가 가장 약한 동료에게 넘깁니다(`PartyComponent`).
- 아무도 못 쓰는 무기라는 안내는 한 번만 띄웁니다.

### 교체된 무기
- 밀려난 무기는 가방(`InventoryComponent`, 최대 무게 20)으로 들어갑니다.
- 가방이 차면 픽업 액터가 그 무기로 바뀌어 바닥에 남습니다.
- 인벤토리 창은 따로 없고, 판매 발판에서 가방을 통째로 팔아 돈으로 바꿉니다.

---

## 레벨 로딩

![](https://velog.velcdn.com/images/gestgest/post/ea85f18e-723f-412c-a914-eb24d58839d2/image.png)

- `ULevelLoaderSubsystem`이 다음 레벨을 비동기로 불러오고, 진행률을 로딩 위젯에 표시합니다.
- 실제 진행률은 95%까지만 반영하고, 패키지 로드가 끝나야 100%로 채웁니다. 최소 표시 시간을 둬서 로딩 화면이 깜빡이고 사라지지 않게 했습니다.

## 모바일 · 데스크톱
| 입력 | 이동 | 공격 |
|---|---|---|
| 마우스 (로스트아크식) | 우클릭 | 좌클릭 |
| 게임패드 | 왼쪽 스틱 | 오른쪽 스틱(조준 + 공격) |
| 모바일 | 가상 조이스틱(`VirtualJoystick`) | 공격 버튼 (자동 타겟) |

- 안드로이드 뒤로가기를 누르면 게임이 일시정지되고 "계속하기 / 나가기" 창(`ExitPanelWidget`)이 뜹니다.
- 메뉴 버튼은 `UMainButton`을 부모로 하는 `WBP_Button` 하나로 통일했습니다. 메인 메뉴 · 게임 준비 · 사망 패널 · 나가기 창이 같은 버튼을 씁니다.
- 구글 플레이 비공개 테스트를 진행했습니다. ([개인정보처리방침](../PRIVACY.md))

---

## AI

### 적 AI (`Enemy`)

- 플레이어와 동료 중 가까운 쪽을 추격합니다.
- 거리 기반 LOD: 가까우면 0.25초, 20m 이상 멀면 0.8초마다 경로를 갱신합니다. 매 프레임 경로를 재요청하지 않습니다.
- 스폰 링: 적은 최대한 시야 밖(22~35m)의 NavMesh 위에 생성됩니다.
- 리쉬(회수): 45m 이상 떨어진 적은 파괴하지 않고, 위치만 다시 스폰 지점으로 옮겨 재사용합니다. 재사용할 때 HP를 초기화합니다.

### 동료 AI (`AAllyCharacter`)
<img width="400" height="225" alt="bandicam 2026-08-20_ai" src="https://github.com/user-attachments/assets/86d00dd1-9405-4740-a112-9b9213d94d88" />

- 의사결정(적 스캔·이동 명령)은 0.2초 간격으로 돌고, 조준과 공격 타이밍은 매 프레임 갱신합니다.
- 이동 명령과 공격이 사용하는 사거리 값은 같은 함수(`GetEngageRange`)에서 가져옵니다. 두 값이 어긋나면 사거리 밖에 멈춰 서서 공격을 못 하는 버그가 생깁니다.
- 더 좋은 무기를 얻으면 동료가 자동으로 교체합니다.
- 동료(`ACompanion`)와 경비병(`AGuardCharacter`)은 교전 · 복귀 로직을 공통 베이스 `AAllyCharacter`에서 같이 씁니다. 동료는 리더를, 경비병은 자기 자리를 기준점으로 삼습니다.

#### 복귀 · 순간이동
![](https://velog.velcdn.com/images/gestgest/post/95e654bd-1943-42aa-a97f-2462cf37b8e5/image.gif)

기준점(리더)과의 거리에 따라 행동이 바뀝니다.

| 거리 | 행동 |
|---|---|
| 12m 밖 (`SoftTrackingDistance`) | 싸우던 적은 계속 때리고, 새 타겟은 리더 근처에서만 찾는다 |
| 18m 밖 (`HardTrackingDistance`) | 적을 무시하고 리더에게 돌아간다 |
| 30m 밖 (`TeleportDistance`) | 리더 뒤쪽 땅으로 순간이동한다. 자리가 없으면 리더 주변, 둘 다 없으면 하지 않는다 |

---

## 최적화
매 프레임 도는 연산과 매번 새로 스폰하는 액터를 줄였습니다.

| 기법 | 내용 |
|---|---|
| 오브젝트 풀링 (적/코인) | 적 30, 코인 20을 미리 만들어 재사용한다. 반납 시 숨김뿐 아니라 이동도 정지시킨다. 안 그러면 발밑 청크가 언로드될 때 대기 중인 적이 낙하한다. |
| 발사체 풀링 | `UProjectilePoolSubsystem`이 화살/파이어볼을 클래스별로 풀링한다. 적중·수명만료 시 Destroy 대신 반환한다. |
| 전투 캐릭터 레지스트리 | `UCombatRegistrySubsystem`에 전투 캐릭터를 등록해두고 가장 가까운 적을 조회한다. |
| AI 틱 간격화 | 추격 갱신·의사결정을 프레임이 아닌 시간 간격으로 전환하고, 거리 LOD를 적용한다. |
| 청크 언로드 | 반경 밖 청크의 액터를 묶음 단위로 파괴한다. |

---

## 코드 구조

```
ACombatCharacter (전투 공통 베이스: HP · 죽음 · Notify 배선 · HP바 · 무기 슬롯)
 ├─ AMyPlayer      : 입력 · 카메라 · 돈/레벨 · 동료 관리
 ├─ AAllyCharacter : 아군 AI 공통 (교전 · 복귀)
 │   ├─ ACompanion      : 리더 추종 · 순간이동
 │   └─ AGuardCharacter : 마을 경비병 (자리 지킴)
 ├─ AEnemy         : 적 AI (추격 · 풀 · 리쉬)
 │   └─ ABoss      : 역병마을 보스 (돌진 패턴 · 무기 드롭 · 처치 시 POI 상태 기록)
 └─ AVillager      : 비전투 NPC (배회)

UJobComponent (전투 방식 캡슐화 — 플레이어/동료 공용)
 ├─ UWarriorJob  ├─ UArcherJob  ├─ UMageJob  └─ UHealerJob

UPartyComponent       : 동료 섭외 · 명단 · 무기 배분
UInventoryComponent   : 무게 제한 가방

AInfiniteMapGenerator : 청크 생성/언로드 · POI 배치 · FPOIStateStore
AZombieSlayerGameMode : 적/코인 풀 · 스폰 링 · 리쉬 · 사망 시 시간 정지

AGaugeZone            : 발판 공통 베이스
├─ AMoneyPadZone
│   ├─ ACompanionSpawnZone
│   └─ AWeaponUpgradeZone
└─ AItemSellZone

UItemDataAsset
└─ UWeaponDataAsset
AWeaponPickup         : 바닥에 떨어진 무기

UHPBar                : 플레이어·보스 공용 HP바 (잔상 애니메이션)
UBossStatusWidget     : 보스 HP바 · 이름 · 데미지 텍스트
UZoneGaugeWidget      : 발판 게이지 (진행률 · 이름 · 비용)
UMainButton           : 메뉴 공용 버튼 (WBP_Button의 부모)

UZombieGameInstance   : 레벨을 넘어 다른 레벨에게 전송되는 데이터(선택한 직업)
GameInstance Subsystem: ULevelLoaderSubsystem : 비동기 레벨 로딩
World Subsystems      : UProjectilePoolSubsystem · UCombatRegistrySubsystem : Pool시스템
```
