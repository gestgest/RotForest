# 에셋 추적 정책 (.gitignore 기준)

> 작성 2026-08-05. `.gitignore`를 고칠 때 / 새 마켓 팩을 받았을 때 / 저장소를 새로 클론할 때 이 문서를 먼저 본다.

## 왜 이 문서가 있나

`Content/`가 **5GB 넘는 마켓 에셋과 내가 만든 에셋이 같은 폴더에 섞여 있는 상태**다.
git에는 내 것만 올라가 있는데, 그 내 것들이 무시된 벤더 폴더를 하드 참조한다.

확인된 참조 (전부 추적 중인 파일 → 무시된 경로):

| 추적 중인 파일 | 참조하는 무시된 경로 |
|---|---|
| `Content/BP/BP_ZombieSlayerGameMode.uasset` | `Characters/ElfArden/...` |
| `Content/BP/BP_InfiniteMapGenerator.uasset` | `Necropolis/Materials/MI_floor_ground_mo` |
| `Content/BP/GameObject/BP_Coin.uasset` | `Characters/...` |
| `Content/BP/GameObject/BP_CompanionSpawnZone.uasset` | `Characters/...` |
| `Content/BP/Job/BP_WarriorJob.uasset` | `Characters/ElfArden/...` |
| `Content/Maps/ElfArden.umap` | `Characters/ElfArden/...` |
| `Content/Animation/ABP/ABP_Enemy.uasset` | `Characters/...` |
| `Content/Animation/AM/AM_Enemy_*.uasset` | `Characters/...` |
| `Content/Animation/AS/**` | `Characters/...` |

**즉 지금 상태로 클론하면 게임이 안 뜬다.** 이 문서 아래 "클론 후 셋업" 절차를 반드시 거쳐야 한다.

---

## 1. 기본 원칙

분리 기준은 **에셋의 종류가 아니라 출처**다.

- **내가 만든 것** → 추적한다. 잃으면 복구 불가.
- **마켓/Epic에서 받은 원본** → 무시한다. Fab에서 다시 받으면 된다.

`.uasset`이냐 아니냐는 기준이 아니다. `Content/` 안의 거의 모든 파일이 `.uasset`이다
(BP도, 몽타주도, 119MB짜리 텍스처도 전부 `.uasset`).

### 판별법: 임포트 날짜

마켓 팩은 임포트할 때 **모든 파일의 타임스탬프가 같은 날짜로 찍힌다.**
그 날짜보다 나중 = 내가 손댄 것.

```bash
# 특정 벤더 폴더에서 내가 건드린 파일 찾기 (ElfArden 예시, 임포트일 2026-06-30)
find Content/Characters/ElfArden -type f -newermt 2026-07-01 -printf '%TY-%Tm-%Td  %p\n' | sort -r
```

---

## 2. 벤더 팩 목록 — 클론 후 다시 받아야 하는 것

| 폴더 | 임포트일 | 용량 | Fab/스토어 이름 |
|---|---|---|---|
| `Content/Characters/ElfArden/` | 2026-06-30 | 186M | *(TODO: 스토어 페이지 링크 적기)* |
| `Content/Characters/SKnight_modular/` | 2026-06-30 | 1.1G | *(TODO)* |
| `Content/Characters/Mannequins/` | 2025-12-16 | 389M | Epic 3인칭 템플릿 (UE5 마네킹) |
| `Content/Characters/Mannequin_UE4/` | 2025-12-16 | 13M | Epic UE4 마네킹 |
| `Content/Assassin/` | 2026-06-21 | 1.1G | *(TODO)* |
| `Content/Necropolis/` | 2026-07-21 | 2.2G | *(TODO)* |
| `Content/StarterContent/` | 2026-01-05 | 585M | Epic Starter Content |

> ⚠️ 위 폴더들은 **원래 경로 그대로** 다시 넣어야 한다. 마켓 에셋은 내부적으로
> `/Game/ElfArden/...` 같은 경로를 하드코딩하고 있어서 위치를 옮기면 팩 업데이트 때 깨진다.

용량이 작아서 그냥 추적하는 서드파티 (다시 안 받아도 됨):
`Content/Joystick/` (844K), `Content/Mixed_Magic_VFX_Pack/` (808K),
`Content/ThirdPerson/` (68K), `Content/LevelPrototyping/` (280K)

---

## 3. ⚠️ 벤더 폴더 안에 있는 "내 것" — 반드시 추적할 것

**이 문서에서 제일 중요한 표.** 벤더 폴더 안에 내 작업물이 섞여 들어가 있어서,
폴더 통째로 무시하면 조용히 날아간다.

### `Content/Characters/ElfArden/` (벤더 임포트 2026-06-30)

| 경로 | 날짜 | 크기 | 뭔지 |
|---|---|---|---|
| `Blueprint/BP_ElfArden.uasset` | | 563K | 플레이어 본체 BP |
| `Blueprint/BP_sword.uasset` | | 24K | |
| `Blueprint/Rig/BS_ElfArden_IdleWalk.uasset` | 07-16 | 8.8K | 블렌드스페이스 |
| `Blueprint/Rig/BS_ElfArden_IdleWalk_Armed.uasset` | 07-16 | 8.9K | 블렌드스페이스 (무장) |
| `Animations/ABP_ElfArden.uasset` | 08-03 | 523K | 애님 BP, 스테이트머신 전부 내 것 |
| `Animations/AM/` 전체 (7개) | 08-02 | 92K | 직업별 몽타주 |
| `Animations/AS/` 전체 | 07-16 | 23M | **리타겟한 애님 시퀀스** |
| `BaseMesh/SKEL_ElfArden.uasset` | 08-02 | 62K | **스켈레톤 (무기 소켓)** |
| `BaseMesh/Separate/SK_body.uasset` | 08-02 | 2.6M | |

> **`Animations/AS/`를 벤더 것으로 착각하기 쉽다.** 벤더 원본 애님은
> `Animations/UE5_EpicSkeleton/` 쪽이다 (`AS_Idle_EpicSkeleon`, `AS_Walk_Fwd_EpicSkeleon`, 06-30).
> `AS/`는 `AS_attack_weapon`, `AS_equip`, `AS_dead_weapon` 처럼 내가 리타겟한 세트고,
> `ABP_ElfArden`이 전부 이걸 참조한다.

> **`SKEL_ElfArden.uasset`이 날아가면 무기 소켓이 사라져서 검이 손에 안 붙는다.**

### 그 밖의 벤더 폴더 안 내 작업물

| 경로 | 날짜 | 크기 | 뭔지 |
|---|---|---|---|
| `Characters/BP_Enemy.uasset` | | 2.4K | |
| `Characters/BP_Villager.uasset` | | 34K | |
| `Characters/Companion/` 전체 | 08-04 | 548K | BP_Companion + ABP_Companion |
| `Characters/Wizard/` 전체 | 07-16 | 25M | 믹사모 임포트 (힐러/마법사 모션) |
| `Characters/healerMotion.fbx` | | 2M | 임포트 원본 |
| `Characters/AS_Standing_Draw_Arrow_Anim_mixamo_com.uasset` | | 191K | |
| `Characters/SKnight_modular/BP_Enemy.uasset` | 08-03 | 94K | 적 BP |
| `Characters/SKnight_modular/Skeleton_Knight_07/mesh/weapon/SK_weapon_Skeleton.uasset` | 08-01 | 7.1K | |
| `Characters/SKnight_modular/Skeleton_Knight_07/mesh/weapon/SK_weapon_PhysicsAsset.uasset` | 08-01 | 13K | |
| `Characters/SKnight_modular/Skeleton_Knight_07/mesh/weapon/SK_Arrow_PhysicsAsset.uasset` | 08-01 | 30K | |
| **`Necropolis/Materials/MI_floor_ground_mo.uasset`** | 07-22 | 21K | **바닥 머티리얼. `BP_InfiniteMapGenerator`가 참조** |

추적 대상 총합 약 **55MB**. git이 감당 가능한 크기다.
제일 큰 단일 파일은 `AS_Standing_1H_Magic_Attack_01_Anim_mixamo_com1.uasset` (12.2M) —
GitHub 파일 한도 50MB 밑이라 문제없다.

> `.uasset`은 바이너리라 수정할 때마다 전체 사본이 저장소에 쌓인다.
> AS 애님은 리타겟 끝나면 되도록 건드리지 말 것.

---

## 4. 통째로 추적 중인 폴더 (내 것)

```
Content/Animation/      ABP_Enemy, AM_Enemy_*, AS/Enemy, AS/Player, BS_Enemy
Content/BP/             GameMode 4종, Job 4종, UI, GameObject
Content/Maps/           GamePlay, GameReady, MainMenu, ElfArden
Content/Material/
Content/Mesh/           Weapon/Bow, Grail, Stape
Content/Sound/
Content/__ExternalActors__/    World Partition 사이드카 (엔진 관리, 못 옮김)
Content/__ExternalObjects__/
```

---

## 5. 클론 후 셋업 절차

1. 저장소 클론
2. 위 **2번 표**의 벤더 팩을 Fab에서 받아 **원래 경로 그대로** 넣는다
3. `.uproject` 우클릭 → Generate Visual Studio project files
4. **VS2022로** Development Editor 빌드 (VS2019 불가)
5. 에디터 열고 참조 깨진 것 없는지 확인

관련 문서: [progress-log.md](progress-log.md)

---

## 6. 앞으로의 규칙

1. **새 에셋은 벤더 폴더 안에 만들지 않는다.** `Content/BP/`, `Content/Animation/`,
   `Content/Material/` 등 이미 추적 중인 폴더에 만든다.
   캐릭터별로 나누고 싶으면 `Content/Animation/AS/Player/` 처럼 추적 폴더 안에서 나눈다.
2. **벤더 에셋을 수정해야 하면 복제해서 내 폴더로 옮긴 뒤 수정한다.**
   (`MI_floor_ground_mo`가 Necropolis 안에 갇힌 게 이걸 안 해서 생긴 일)
3. **새 마켓 팩을 받으면 즉시 `.gitignore`에 추가한다.**
   지금 `.gitignore`는 블랙리스트 방식이라 빠뜨리면 몇 GB가 조용히 커밋에 딸려간다.
   (실제로 `Mixed_Magic_VFX_Pack`이 누락돼 있었다)
4. **소켓/노티파이를 벤더 스켈레톤·애님에 추가했으면** 그 파일을 예외 목록에 넣는다.
   `SKEL_ElfArden`이 그 사례.

---

## 7. 검증

`.gitignore`를 고친 뒤 확인:

```bash
# 특정 파일이 무시되는지 (출력 없으면 = 추적됨, 정상)
git check-ignore -v Content/Characters/ElfArden/Blueprint/BP_ElfArden.uasset

# 3번 표의 파일이 전부 추적되는지 한 번에 확인
git check-ignore -v \
  Content/Characters/ElfArden/Blueprint/BP_ElfArden.uasset \
  Content/Characters/ElfArden/Animations/ABP_ElfArden.uasset \
  Content/Characters/ElfArden/BaseMesh/SKEL_ElfArden.uasset \
  Content/Characters/Companion/BP_Companion.uasset \
  Content/Characters/SKnight_modular/BP_Enemy.uasset \
  Content/Necropolis/Materials/MI_floor_ground_mo.uasset
# → 아무것도 출력되지 않아야 정상

# 실수로 벤더 원본이 딸려오는지 확인 (커밋 전 필수)
git add -An | wc -l
git status --porcelain | head -50
```

### `.gitignore` 함정

`Content/Characters/*` 처럼 **디렉토리 자체가 제외되면 git이 그 안으로 들어가지 않는다.**
안쪽 파일을 `!`로 되살리려면 경로상의 디렉토리를 한 단계씩 다시 열어줘야 한다:

```gitignore
Content/Characters/*
!Content/Characters/ElfArden/          # 디렉토리를 먼저 열고
Content/Characters/ElfArden/*          # 안쪽을 다시 닫고
!Content/Characters/ElfArden/Blueprint/  # 필요한 것만 연다
```

순서도 중요하다 — **뒤에 오는 규칙이 이긴다.**
