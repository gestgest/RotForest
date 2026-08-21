# RotForest — 게임플레이 문서


<img width="400" height="225" alt="bandicam 2026-08-20 20-49-07-148" src="https://github.com/user-attachments/assets/d0055de6-78cc-4984-8e4d-6f7dd528ca51" />

---
## 기본 시스템
### 전투 시스템
![bandicam 2025-12-14 21-58-13-948](https://github.com/user-attachments/assets/6d7ee84f-4018-4288-8023-b7dcdfd99186)
- 적은 플레이어와 마주치면 따라갑니다.


### 코인 수집
![bandicam 2025-12-14 21-58-13-948 (1)](https://github.com/user-attachments/assets/0637a540-b307-41fb-8c95-13b5a1e2f602)
- 맵에 배치된 코인을 수집하여 점수를 획득합니다.

### 사망 연출
<img width="400" height="225" alt="bandicam 2026-08-21 _dead" src="https://github.com/user-attachments/assets/ffb8ab75-1727-42fe-bd47-4df18ae515f3" />

플레이어가 죽으면 화면 속 적들의 시간이 멈춥니다.
게임모드가 적 전체에 `CustomTimeDilation = 0`을 걸고 스폰·리쉬 타이머까지 일시정지시킨 뒤 사망 패널을 띄웁니다.


### 적, 코인 생성
![bandicam 2026-02-13 00-58-35-815_success](https://github.com/user-attachments/assets/f7958179-a355-402e-aa4b-c12fcf40be96)

오브젝트 pooling 알고리즘을 활용해서 적과 코인을 주변에 생성합니다.

---

## 직업 시스템

| 직업 | 공격 방식 | MaxHP | 데미지 | 이동속도 | 공격 간격 |
|---|---|---|---|---|---|
| 전사 | 전방 구체 스윕(다중 타격 + 넉백) | 25 | 1 | 700 | 0.6s |
| 궁수 | 화살 발사, 시위 당김 애니 연동 | 10 | 1 | 900 | 0.4s |
| 마법사 | 느린 발사체 + 착탄 지점 범위 폭발 | 8 | 4 | 600 | 1.2s |
| 힐러 | 공격 대신 전방 아군 회복(적에겐 무효) | 8 | 1 | 600 | 10s |

<table>
<tr>
<td><img width="400" src="https://github.com/user-attachments/assets/ed01960d-9ce0-4ca4-9700-00d44e2d865e" /><br>전사</td>
<td><img width="400" src="https://github.com/user-attachments/assets/c3bc110d-14d7-4647-928f-d3e0bb9076ac" /><br>궁수</td>
<tr>
<td><img width="400" src="https://github.com/user-attachments/assets/62562274-254a-4805-838c-efb248ca21af" /><br>마법사</td>
<td><img width="400" src="https://github.com/user-attachments/assets/19722ab5-d22d-4d89-916c-114a040a2b29" /><br>힐러</td>
</tr>
</table>

---

## 무한 맵 생성

<img width="400" height="225" alt="bandicam 2026-08-20 20-49-07-148 (1)" src="https://github.com/user-attachments/assets/3f01b599-75c6-4eb7-8f05-f91ebcc5cd05" />

레벨에 배치된 `AInfiniteMapGenerator` 하나가 플레이어를 따라다니며 지형을 만들고 지웁니다.

- 같은 시드·같은 좌표는 언제나 같은 결과를 내므로, 떠났다 돌아와도 그 자리에 같은 지형이 다시 깔립니다. 마인크래프트식 청크 생성 시스템이라고 보면 됩니다.
- 청크가 스폰한 액터는 `FMapChunk`에 묶여 있어 언로드 시 한 번에 정리됩니다.
- NavMesh 동적 생성: 런타임에 생성된 바닥 위에 길찾기가 되도록, 플레이어에 `UNavigationInvokerComponent`를 붙이고 `NavMeshBoundsVolume`을 플레이어를 따라 옮기며 내비 시스템에 경계 변경을 통지합니다.
- 스폰존이나 보스 처치같은 기록해야하는 정보는 POIStateStore로 저장합니다.

---
### POI

<img width="400" alt="image" src="https://github.com/user-attachments/assets/6c43baba-e7c6-45ab-8e34-7a7e0ba28ff3" />

[POI 설명 사이트](https://velog.io/@gestgest/Unreal-%EC%B2%AD%ED%81%AC%EC%8B%9C%EC%8A%A4%ED%85%9C2)

마인크래프트를 예시로 들면 마을 중간에 오브젝트가 랜덤하게 배치되고 있으면 어떨까? 어색할 것입니다.
그래서 POI라는 큰 구역을 두고 그 구역에는 랜덤 오브젝트를 넣지 않고 마을처럼 정해진 오브젝트를 추가합니다.

#### 마을
<img width="400" height="225" alt="bandicam 2026-08-20_villege" src="https://github.com/user-attachments/assets/e40c3ec0-ae6e-4f82-9966-e17d2a481560" />
- 경비병(`Companion`의 경비 모드): 리더를 따라다니는 대신 자기 자리를 지키고, 교전이 끝나면 원위치로 복귀합니다.
- 주민(`Villager`): 비전투 NPC. 마을 주변을 배회하고, 피해를 받을 수 있습니다.
- 적 스폰 링이 마을을 피합니다. 게임모드가 맵 생성기에 질의해 마을 안에는 스폰하지 않습니다.
- 마을 외곽 구조물은 고정 슬롯 6곳에 배치됩니다. 구조물끼리 겹치지 않습니다.

#### 역병마을
- 적 밀집, 보스(`Boss`)가 있습니다.
- 보스는 자신이 속한 마을의 청크 좌표를 알고 있어서, 죽을 때 그 좌표를 클리어로 기록합니다.

#### 상태 영속
청크가 사라져도 진행도는 남습니다.
청크가 언로드되면 액터가 전부 파괴됩니다. 지형은 시드로 다시 생성되지만 플레이어가 바꾼 상태(발판에 넣은 돈, 보스 처치 여부)는 남지 않습니다.
이 값들은 `FPOIStateStore`(POI 중심 좌표 → 상태 맵)에 따로 보관합니다.

저장소는 GameInstance가 아니라 맵 생성기의 멤버로 뒀습니다. GameInstance에 두면 죽고 다시 시작해도 이전 판의 마을 강화가 남습니다. 생성기는 레벨과 함께 파괴되므로 새 판은 초기 상태로 시작합니다.

---

## 성장
<img width="400" height="225" alt="bandicam 2026-08-20_spawnzone" src="https://github.com/user-attachments/assets/b415613b-5c2b-4733-8a54-a889e380b32a" />
코인으로 모은 돈은 발판 위에 서 있는 동안 소비됩니다. 

```
AMoneyPadZone (공통 베이스)
 ├─ 게이지가 가득 차면 HandleZoneFilled() 호출
 ├─ ACompanionSpawnZone  → 랜덤 직업 동료 소환
 └─ AWeaponUpgradeZone   → 무기 강화 (강화할수록 다음 비용 상승)
```

- 진행도는 `FPOIStateStore`에 저장되므로, 청크가 언로드되어도 이어서 채울 수 있습니다.
- 결제·게이지·쿨다운·디버그 바는 베이스가 처리하므로, 새 발판은 `HandleZoneFilled()`만 구현하면 추가됩니다.

---

## AI

### 적 AI (`Enemy`)

- 플레이어와 동료 중 가까운 쪽을 추격합니다.
- 거리 기반 LOD: 가까우면 0.25초, 20m 이상 멀면 0.8초마다 경로를 갱신합니다. 매 프레임 경로를 재요청하지 않습니다.
- 스폰 링: 적은 최대한 시야 밖(22~35m)의 NavMesh 위에 생성됩니다. 
- 리쉬(회수): 45m 이상 떨어진 적은 스폰될 때 위치만 수정해서 스폰됩니다.

### 동료 AI (`Companion`)
<img width="400" height="225" alt="bandicam 2026-08-20_ai" src="https://github.com/user-attachments/assets/86d00dd1-9405-4740-a112-9b9213d94d88" />

- 의사결정(적 스캔·이동 명령)은 0.2초 간격으로 돌고, 조준과 공격 타이밍은 매 프레임 갱신합니다.
- 동료마다 시작 오프셋을 랜덤화해 여러 동료의 연산이 같은 프레임에 몰리지 않게 분산했습니다.
- 이동 명령이 쓰는 거리와 공격이 쓰는 거리는 같은 함수(`GetEngageRange`)를 통합니다. 두 값이 어긋나면 사거리 밖에 서서 공격하지 않는 버그가 납니다.

---

## 최적화
매 프레임 도는 연산과 매번 새로 스폰하는 액터를 줄였습니다.

| 기법 | 내용 |
|---|---|
| 오브젝트 풀링 (적/코인) | 적 20, 코인 10을 미리 만들어 재사용. 반납 시 숨김뿐 아니라 이동도 정지시킨다. 안 그러면 발밑 청크가 언로드될 때 대기 중인 적이 낙하한다. |
| 발사체 풀링 | `UProjectilePoolSubsystem`이 화살/파이어볼을 클래스별로 풀링. 적중·수명만료 시 Destroy 대신 반환. |
| 전투 캐릭터 레지스트리 | `UCombatRegistrySubsystem`에 전투 캐릭터를 등록해두고 가장 가까운 적을 조회. `GetAllActorsOfClass` 전체 순회를 대체. `TWeakObjectPtr`로 담아 죽은 액터를 잡아두지 않는다. |
| AI 틱 간격화 | 추격 갱신·의사결정을 프레임이 아닌 시간 간격으로 전환 + 거리 LOD. |
| 청크 언로드 | 반경 밖 청크의 액터를 묶음 단위로 파괴. |

---

## 코드 구조

```
ACombatCharacter (전투 공통 베이스: HP · 죽음 · Notify 배선 · HP바)
 ├─ AMyPlayer      : 입력 · 카메라 · 돈/레벨 · 동료 관리
 ├─ ACompanion     : 아군 AI (추종/교전, 경비 모드)
 ├─ AEnemy         : 적 AI (추격 · 풀 · 리쉬)
 │   └─ ABoss      : 역병마을 보스 (처치 시 POI 상태 기록)
 └─ AVillager      : 비전투 NPC (배회)

UJobComponent (전투 방식 캡슐화 — 플레이어/동료 공용)
 ├─ UWarriorJob  ├─ UArcherJob  ├─ UMageJob  └─ UHealerJob

AInfiniteMapGenerator : 청크 생성/언로드 · POI 배치 · FPOIStateStore
AZombieSlayerGameMode : 적/코인 풀 · 스폰 링 · 리쉬 · 사망 시 시간 정지

AMoneyPadZone         : 돈 발판 베이스
├─ ACompanionSpawnZone
└─ AWeaponUpgradeZone

UZombieGameInstance   : 레벨을 넘어 다른 레벨에게 전송되는 데이터(선택한 직업)
World Subsystems      : UProjectilePoolSubsystem · UCombatRegistrySubsystem : Pool시스템
```
