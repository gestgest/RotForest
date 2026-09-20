# TODO: 공격 조준을 "방향"에서 "좌표"로

## 목표
좌클릭한 **그 지점**으로 공격이 나가게 한다.
지금은 클릭 순간의 **방향**만 고정해서, 공격 모션(0.2~0.4초) 동안 이동하면 화살이 적과 평행하게 날아간다.

## 현재 상태 (이미 되어 있는 것)
- `ACombatCharacter::AttackAimDir` — `TickAttack`이 `Attack()` 직전에 정면을 고정 (CombatCharacter.cpp:412)
- `ACombatCharacter::GetAttackAimDir()` — 발사/스윕이 이 값을 씀 (CombatCharacter.cpp:421)
- `AMyPlayer::AttackFacingHold` — 공격 중 이동 방향으로 몸이 되돌아가지 않게 막음 (MyPlayer.cpp:363)

---

## 할 일

### 1. 커서 좌표를 버리지 말고 저장
- [ ] 일단 좌클 우클 기능 분리
- [ ] 좌클인 경우 그냥 cursor

- [ ] `AMyPlayer::MouseInput` (MyPlayer.cpp:275~) — `GetCursorGroundLocation`이 좌표를 구해놓고 방향만 쓰고 버린다
- [ ] `LastCursorDir`(MyPlayer.h:204) 옆에 `LastCursorPoint` 추가하고 유효할 때 같이 캐시

### 2. 베이스에 조준 좌표 추가
- [ ] `ACombatCharacter`에 `AttackAimPoint`(FVector) + `bHasAttackAimPoint`(bool) 추가 — `AttackAimDir` 옆
- [ ] 좌표를 넣어줄 public 함수 추가 (예: `SetDesiredAimPoint(const FVector& Point)`)
      => 동료/적은 안 부른다. 안 부르면 `bHasAttackAimPoint`가 false로 남아 기존 동작 그대로.

### 3. 공격 시작할 때 좌표도 같이 고정
- [ ] `TickAttack`에서 `AttackAimDir` 고정하는 자리(CombatCharacter.cpp:412)에 `AttackAimPoint`도 같이 확정
- [ ] **순서 주의**: 좌표를 밀어넣는 건 `TickAttack`보다 **먼저** 실행돼야 한다.
      Tick 순서는 `MouseInput` → `UpdateMovement` → `UpdateAimAndAttack`(안에서 TickAttack) 이다.

### 4. 발사 방향을 좌표에서 다시 계산
- [ ] `GetAttackAimDir()`를 아래처럼 바꾼다
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
- **0 나누기**: `GetSafeNormal()`은 너무 짧으면 **0벡터**를 반환한다. 그대로 `Rotation()`에 넣으면 yaw=0 => 월드 +X축을 보고 쏜다. 반드시 폴백.
- **Z 성분**: 커서 평면은 플레이어 Z 기준이라 높이 차가 있으면 위/아래로 쏜다. `To.Z = 0` 필수.
- **동료/적 영향**: `SetDesiredAimPoint`를 안 부르면 기존 방향 방식으로 동작해야 한다. 동료 공격이 이상해지면 이 폴백이 안 걸린 것.
- **전사 넉백**: `WarriorJob.cpp:59`도 같은 방향을 쓴다. 이미 한 번 뽑은 `AimDir` 재사용 중이니 그대로 두면 된다.

## 테스트
1. 제자리에서 탭 클릭 -> 커서로 나가나 (지금도 됨, 회귀 확인용)
2. **옆으로 달리면서** 탭 클릭 -> 화살이 클릭 지점으로 수렴하나 (이번 작업의 핵심)
3. 적을 향해 달리면서 근접 공격 -> 몸이 이상하게 돌지 않나
4. 발 밑을 클릭 -> 엉뚱한 방향(+X축)으로 안 쏘나 (0 폴백 확인)
5. 동료가 적을 제대로 때리나 (회귀)

