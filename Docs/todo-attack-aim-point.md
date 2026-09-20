# TODO: 공격 조준을 "방향"에서 "좌표"로

## 목표
좌클릭한 **그 지점**으로 공격이 나가게 한다.
지금은 클릭 순간의 **방향**만 고정해서, 공격 모션(0.2~0.4초) 동안 이동하면 화살이 적과 평행하게 날아간다.

```
클릭 순간:  플레이어(0,0) → 커서(1000,0)     방향 = (1,0)
0.3초 뒤:   플레이어(0,300)으로 이동, Notify 발사
  방향 저장 -> (1,0)으로 발사 -> (1000,300) 쪽, 빗나감
  좌표 저장 -> (1000,0)-(0,300) 재계산 -> 명중
```

## 현재 상태 (이미 되어 있는 것)
- `ACombatCharacter::AttackAimDir` — `TickAttack`이 `Attack()` 직전에 정면을 고정 (CombatCharacter.cpp:414)
- `ACombatCharacter::GetAttackAimDir()` — 발사/스윕이 이 값을 씀 (CombatCharacter.cpp:421)
- `AMyPlayer::AttackFacingHold` — 공격 중 이동 방향으로 몸이 되돌아가지 않게 막음 (MyPlayer.cpp:363)

---

## 배경: MouseInput의 구조 (2026-09-20 확인)

`MouseInput`은 세 덩어리다.

```
(A) 커서 -> 월드 좌표     : GetCursorGroundLocation(Cursor)
(B) 좌표 -> 쓸 만한 방향  : ToCursor, StopRadius 컷, Normalize, LastCursorDir 폴백
(C) 결과 배분             : bRight -> MouseMove / bLeft -> MouseAim
```

- 커서는 화면에 하나뿐 -> **(A)는 공통**
- `CursorStopRadius` 컷은 "가까우면 캐릭터가 진자운동" 을 막는 **이동 전용 사정** -> **(B)가 분기점**
- 따라서 좌/우 분리는 **(A) 뒤, (B) 앞**에서 자른다

`MouseAim`은 스틱 포맷 `FVector2D`로 정규화되면서 **거리 정보가 버려진다.**
-> 좌표는 이 파이프라인에 못 태운다. 멤버 캐시 + setter라는 **별도 경로**가 필요하다.

| 경로 | 값 | 용도 |
|---|---|---|
| `MouseAim` (기존 파이프라인) | 방향 | 클릭하는 동안 몸이 커서를 바라보게 |
| `LastCursorPoint` (새 경로) | 좌표 | 발사 시점에 방향을 재계산 |

둘 다 필요하다. 하나로 못 합친다.

---

## 할 일

### 1. 커서 좌표를 버리지 말고 저장
- [x] 좌클 / 우클 처리 블록 분리 (MyPlayer.cpp:294 `// 마우스 공격 에임`)
- [ ] **조준 블록이 아직 `MoveDir`에 의존한다** — `if (!MoveDir.IsNearlyZero() && bLeftMouseHeld)`
      `MoveDir`은 StopRadius 컷을 통과해야만 채워지므로, 자리만 옮겼고 동작은 그대로다.
      조준은 `Cursor`(원본 좌표)에서 **새로 계산**해야 한다.
- [ ] `LastCursorDir`(MyPlayer.h:204) 옆에 `LastCursorPoint` 추가하고 유효할 때 같이 캐시
- [ ] 조준용 0 방어 추가 — StopRadius가 대신 막아주던 게 없어졌다
- [ ] `else` 분기(커서 변환 실패) 조준 폴백 결정 — 직전 좌표 재사용 권장. 거의 안 일어나는 케이스

### 2. 베이스에 조준 좌표 추가
- [ ] `ACombatCharacter`에 `AttackAimPoint`(FVector) + `bHasAttackAimPoint`(bool) 추가 — `AttackAimDir` 옆
- [ ] 좌표를 넣어줄 public 함수 추가 (예: `SetDesiredAimPoint(const FVector& Point)`)
      => 동료/적은 안 부른다. 안 부르면 `bHasAttackAimPoint`가 false로 남아 기존 동작 그대로.

### 3. 공격 시작할 때 좌표도 같이 고정
- [ ] `TickAttack`에서 `AttackAimDir` 고정하는 자리(CombatCharacter.cpp:414)에 `AttackAimPoint`도 같이 확정
- [ ] **순서 주의**: 좌표를 밀어넣는 건 `TickAttack`보다 **먼저** 실행돼야 한다.
      Tick 순서는 `MouseInput` -> `UpdateMovement` -> `UpdateAimAndAttack`(안에서 TickAttack) 이다.

### 4. 발사 방향을 좌표에서 다시 계산
- [ ] `GetAttackAimDir()`(CombatCharacter.cpp:421)를 3단 폴백으로 바꾼다

| 순위 | 값 | 조건 |
|---|---|---|
| 1 | `AttackAimPoint`에서 재계산한 방향 | 좌표가 있고 `To`가 0이 아닐 때 (플레이어) |
| 2 | `AttackAimDir` | 좌표 없음(동료·적) or 발밑 클릭 |
| 3 | `GetActorForwardVector()` | 한 번도 공격한 적 없음 |

```
To = AttackAimPoint - GetActorLocation()
To.Z = 0
좌표 없음 or To가 거의 0  ->  AttackAimDir 반환 (폴백)
그 외                     ->  To 정규화해서 반환
```
- [ ] 지나쳐서 180도 뒤집히는 건 **막지 않는다** (타겟이 실제로 뒤에 있는 것이므로 맞는 동작)

### 5. 공격 중에도 그 좌표를 계속 바라보게
- [ ] `UpdateAimAndAttack`(MyPlayer.cpp:373)의 분기 구조를 바꾼다
```
조준 중(bAiming)        -> 커서 방향으로 스냅 (지금 그대로)
AttackFacingHold > 0    -> GetAttackAimDir() 방향으로 회전  <= 새로 추가
그 외 + 이동 중          -> 이동 방향으로 보간 (지금 그대로)
```
      => 몸과 발사 방향이 항상 같아지므로 각도 클램프가 필요 없어진다

---

## 함정

- **`ToCursor` 재사용 금지.** `&&`는 단락 평가라 `//todo` 자리에 도달했을 때 상태가 둘로 갈린다.

| 상황 | SizeSquared 검사 | Normalize() 실행 | 그 시점 `ToCursor` |
|---|---|---|---|
| 커서가 멀다 | 통과 | O | 길이 1 |
| 커서가 가깝다 | 실패 | X | 원래 짧은 길이 그대로 |

  조준은 `Cursor`(원본)에서 다시 계산하거나, 위 `if`를 `GetSafeNormal()` 방식으로 바꿔라.
- **0 나누기**: `GetSafeNormal()`은 너무 짧으면 **0벡터**를 반환한다. 그대로 `Rotation()`에 넣으면 yaw=0 => 월드 +X축을 보고 쏜다. 반드시 폴백.
- **Z 성분**: 커서 평면은 플레이어 Z 기준이라 높이 차가 있으면 위/아래로 쏜다. `To.Z = 0` 필수.
- **축 스왑**: `MouseAim`에 넣을 땐 월드(X=앞,Y=오른쪽) -> 스틱(X=오른쪽,Y=위)이라 `FVector2D(Dir.Y, Dir.X)`. 조준 경로에서도 빼먹지 말 것.
- **`LastCursorPoint` 초기값**: (0,0,0)은 월드 원점이라 위험하다. 그래서 `bHasAttackAimPoint` 같은 bool이 따로 필요하다.
- **동료/적 영향**: `SetDesiredAimPoint`를 안 부르면 기존 방향 방식으로 동작해야 한다. 동료 공격이 이상해지면 이 폴백이 안 걸린 것.
- **`AttackAimDir`를 지우면 안 된다**: `ACompanion::Tick`(Companion.cpp:90)도 `TickAttack`을 부른다. 그쪽은 커서가 없다. 4단계의 2순위 폴백 자리이기도 하다.
- **전사 넉백**: `WarriorJob.cpp:59`도 같은 방향을 쓴다. 이미 한 번 뽑은 `AimDir` 재사용 중이니 그대로 두면 된다.

## 남은 설계 결정
- 우클릭+좌클릭 동시에 누르면 커서가 하나라 이동 방향 = 공격 방향이 된다.
  "달리면서 다른 쪽 공격"을 원하면 우클릭을 목적지 지정형으로 바꾸거나, 좌클릭 중 이동 방향을 직전 값으로 고정해야 한다.
  1~5단계와 독립이므로 나중에 정해도 된다.

## 테스트
1. 제자리에서 탭 클릭 -> 커서로 나가나 (지금도 됨, 회귀 확인용)
2. **옆으로 달리면서** 탭 클릭 -> 화살이 클릭 지점으로 수렴하나 (이번 작업의 핵심)
3. **발 근처(StopRadius 안) 클릭** -> 공격이 씹히지 않나 (1단계 분리가 진짜 됐는지 판별)
4. 적을 향해 달리면서 근접 공격 -> 몸이 이상하게 돌지 않나
5. 발 밑을 정확히 클릭 -> 엉뚱한 방향(+X축)으로 안 쏘나 (0 폴백 확인)
6. 동료가 적을 제대로 때리나 (회귀)
