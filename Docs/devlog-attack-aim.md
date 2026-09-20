# 개발로그 — 공격 조준을 "방향"에서 "좌표"로

## 왜 하는가

공격은 두 시점으로 쪼개져 있다.

```
클릭 프레임 ────── 0.2~0.4초 (몽타주) ──────> Notify = 실제 발사
```

지금은 클릭 프레임에 **방향**을 고정해둔다. 방향은 내 위치를 원점으로 한 상대값이라,
그 0.3초 동안 내가 움직이면 같은 방향이 전혀 다른 곳을 가리키게 된다.

```
클릭:      나(0,0)      커서(1000,0)    -> 저장된 방향 (1,0)
0.3초 뒤:  나(0,300)으로 이동, 발사
           (1,0)으로 쏨        -> (1000,300) 쪽. 적과 평행하게 빗나감
           (1000,0)에서 다시 뺌 -> (1000,-300) 방향. 명중
```

**좌표는 내가 어디로 도망가든 안 변한다.** 그래서 좌표를 저장해두고 발사 순간에 방향을 다시 뽑는다.
이 문서는 그걸 위해 손대는 5군데를 순서대로 적는다.

---

# 1. AMyPlayer — 커서 좌표를 버리지 말고 멤버에 저장

## 유도: 왜 멤버여야 하나

`MouseInput`은 매 프레임 이렇게 생겼다.

```cpp
FVector Cursor;                       // <- 지역 변수
if (GetCursorGroundLocation(Cursor))
```

`Cursor`는 지역 변수다. 함수가 끝나면 사라진다.
그런데 발사는 0.3초 뒤 **다른 함수**(`TickAttack` → Notify)에서 일어난다.
지역 변수는 거기까지 못 간다. → **멤버로 승격시켜야 한다.**

그럼 기존 `MouseAim`에 실어보내면 안 되나? 안 된다.

```cpp
FVector2D MouseAim;   // 게임패드 스틱 포맷
```

`FVector2D`에 담기면서 정규화된다 → **거리 정보가 버려지고 방향만 남는다.**
좌표는 이 관을 통과할 수가 없다. 그래서 **별도 경로**가 필요하다.
이 작업이 한 줄로 안 끝나는 이유가 이거 하나다.

| 경로 | 자료형 | 살아남는 정보 | 용도 |
|---|---|---|---|
| `MouseAim` (기존) | `FVector2D` | 방향만 | 클릭 중 몸이 커서를 바라보게 |
| `LastCursorPoint` (신설) | `FVector` | 좌표 | 발사 시점 방향 재계산 |

## 무엇을 추가하나 — MyPlayer.h:204 `LastCursorDir` 옆

```cpp
	// 커서가 마지막으로 유효했던 월드 좌표
	FVector LastCursorPoint = FVector::ZeroVector;
	bool bHasLastCursorPoint = false;
```

**`bool`이 왜 따로 필요한가:** `FVector` 기본값 `(0,0,0)`은 "값 없음"이 아니라 **월드 원점**이라는 좌표다.
둘을 구분할 방법이 없다. 이 bool이 없으면 커서 변환이 한 번도 성공 안 한 상태에서
플레이어가 맵 원점을 향해 쏜다.

## 무엇을 바꾸나 — MouseInput

**Before** — 조준이 `MoveDir`에 얹혀 있다

```cpp
if (ToCursor.SizeSquared() > CursorStopRadius * CursorStopRadius && ToCursor.Normalize())
{
    LastCursorDir = ToCursor;
    MoveDir = ToCursor;
}

if (!MoveDir.IsNearlyZero() && bLeftMouseHeld)   // <- MoveDir 의존
{
    MouseAim = FVector2D(MoveDir.Y, MoveDir.X);
}
```

**After** — 좌표 캐시 + 조준을 독립 계산

```cpp
if (GetCursorGroundLocation(Cursor))
{
    LastCursorPoint = Cursor;
    bHasLastCursorPoint = true;

    FVector ToCursor = Cursor - GetActorLocation();
    ToCursor.Z = 0.0f;

    // [이동] CursorStopRadius는 너무 가까우면 player가 진자운동하는 버그 해결용
    if (ToCursor.SizeSquared() > CursorStopRadius * CursorStopRadius && ToCursor.Normalize())
    {
        LastCursorDir = ToCursor;
        MoveDir = ToCursor;
    }
}
else
{
    MoveDir = LastCursorDir;
}

// [이동] 월드 방향 (dx,dy) -> 스틱 포맷 FVector2D(Y=dx, X=dy)
if (bRightMouseHeld && !MoveDir.IsNearlyZero())
{
    MouseMove = FVector2D(MoveDir.Y, MoveDir.X);
}

// [조준] StopRadius 컷을 타지 않는다
if (bLeftMouseHeld && bHasLastCursorPoint)
{
    FVector ToAim = LastCursorPoint - GetActorLocation();
    ToAim.Z = 0.0f;
    const FVector AimDir = ToAim.GetSafeNormal();
    if (!AimDir.IsNearlyZero())
    {
        MouseAim = FVector2D(AimDir.Y, AimDir.X);
    }

    // Tick 순서상 TickAttack보다 먼저 실행된다 => 자세한 내용은 노션 개발문서 참고
    SetDesiredAimPoint(LastCursorPoint);
}
```

## 왜 이렇게 바뀌나

**(a) 조준을 `MoveDir`에서 떼어낸 이유 — 버그다**

`MoveDir`은 `CursorStopRadius`보다 **멀 때만** 채워진다. 그 컷은 "커서가 가까우면 캐릭터가
진자운동한다"는 **이동 전용 사정**이다. 조준은 그 사정과 무관한데 같이 걸려 있었다.

| 커서 위치 | `MoveDir` | 기존 `MouseAim` | 결과 |
|---|---|---|---|
| StopRadius 밖 | 채워짐 | 갱신됨 | 정상 |
| StopRadius 안 (발 근처) | **비어 있음** | **갱신 안 됨** | `bAiming=false` → **공격이 아예 안 나감** |

발 근처를 클릭하면 공격이 씹혔다. 조준을 `LastCursorPoint`에서 새로 계산하면 이게 없어진다.

**(b) `ToCursor`를 재사용하지 않는 이유 — 단락 평가**

`&&`는 앞이 false면 뒤를 건너뛴다. 즉 `Normalize()`가 실행된 경우와 아닌 경우로 상태가 갈린다.

| 상황 | SizeSquared 검사 | `Normalize()` | 그 시점 `ToCursor` |
|---|---|---|---|
| 커서가 멀다 | 통과 | 실행됨 | 길이 1 |
| 커서가 가깝다 | 실패 | **건너뜀** | 원래 짧은 길이 그대로 |

아래에서 `ToCursor`를 그냥 쓰면 두 케이스가 다른 값이다. `LastCursorPoint`에서 새로 빼야 한다.

**(c) `else` 분기에 조준 폴백을 안 넣은 이유**

`LastCursorPoint`는 덮어쓰지 않으면 직전 값을 그대로 들고 있다.
커서 변환이 실패해도 조준은 자동으로 직전 좌표를 쓴다. 따로 쓸 게 없다.

---

# 2. ACombatCharacter — 멤버 2쌍과 `SetDesiredAimPoint`

## 유도: 왜 변수가 **두 쌍**인가

이게 이 작업에서 제일 안 보이는 부분이다. 이름이 비슷한 좌표가 두 개 필요하다.

| 변수 | 의미 | 언제 바뀌나 |
|---|---|---|
| `DesiredAimPoint` | **지금** 커서가 가리키는 곳 | 매 프레임 덮어써짐 (live) |
| `AttackAimPoint` | **이번 공격이** 조준한 곳 | 공격 시작 순간에 한 번 복사 (박제) |

하나로 합치면 안 되는 이유 — 예시:

```
t=0.0   (1000,0) 클릭. 공격 시작. 몽타주 재생 시작
t=0.1   마우스를 휙 돌려 커서가 (0,1000)으로 이동
t=0.3   Notify 발사

변수 1개면   -> 발사 방향이 (0,1000). 내가 조준한 적이 없는 곳으로 쏜다
변수 2쌍이면 -> AttackAimPoint는 t=0.0의 (1000,0) 그대로. 조준한 곳으로 쏜다
```

**이미 시작한 공격은 그때 조준한 곳으로 가야 한다.** 이걸 래치(latch)라고 하고,
래치하려면 "흐르는 값"과 "박제된 값"이 따로 있어야 한다.

## 무엇을 추가하나 — CombatCharacter.h:207 `AttackAimDir` 옆 (protected)

```cpp
	// 공격 시작 순간의 정면 방향(TickAttack이 고정)
	FVector AttackAimDir = FVector::ZeroVector;

	// 공격 시작 순간에 고정한 조준 좌표(월드)
	FVector AttackAimPoint = FVector::ZeroVector;
	bool bHasAttackAimPoint = false;

	// 외부가 밀어넣는 조준 좌표. 안 들어오면 방향 방식으로 동작한다
	FVector DesiredAimPoint = FVector::ZeroVector;
	bool bHasDesiredAimPoint = false;
```

**왜 `AMyPlayer`가 아니라 `ACombatCharacter`인가:** 이 값을 읽는 건 `TickAttack`인데,
`ACompanion::Tick`(Companion.cpp:90)도 `TickAttack`을 부른다.
읽는 함수가 있는 클래스에 값도 있어야 한다.

> 멤버를 어느 계층에 둘지는 **"누가 그 함수를 부르는가"**가 정한다.

## 무엇을 추가하나 — CombatCharacter.h:264 `GetAttackAimDir()` 옆 (public)

```cpp
	// 조준 좌표 지정. 동료/적은 부르지 않는다
	void SetDesiredAimPoint(const FVector& Point);
```

```cpp
void ACombatCharacter::SetDesiredAimPoint(const FVector& Point)
{
	DesiredAimPoint = Point;
	bHasDesiredAimPoint = true;
}
```

## `SetDesiredAimPoint`는 무슨 함수이고 왜 함수인가

**하는 일:** 바깥(= `AMyPlayer::MouseInput`)에서 베이스 클래스로 좌표를 **밀어넣는 통로**.
1단계에서 만든 `LastCursorPoint`가 이 함수를 타고 `ACombatCharacter`로 건너간다.

```
AMyPlayer::MouseInput               ACombatCharacter
  LastCursorPoint ──SetDesiredAimPoint()──> DesiredAimPoint
                                                 │ (TickAttack이 복사)
                                                 v
                                            AttackAimPoint ──> 발사 방향
```

`DesiredAimPoint`를 public으로 열지 않고 굳이 함수로 만든 이유 셋:

1. **두 값이 짝으로 움직이는 걸 강제한다.** 좌표만 넣고 `bHasDesiredAimPoint` 세팅을
   까먹으면 조용히 폴백으로 새는데, 함수 안에 넣으면 그럴 수가 없다.
2. **안 부르면 기존 동작이 그대로 남는다.** 동료·적은 커서가 없으니 이 함수를 안 부른다.
   → `bHasDesiredAimPoint`가 false로 유지 → 4단계의 폴백을 타고 원래 방식으로 동작.
   **호출하지 않는 것이 곧 옵트아웃**이라 동료/적 코드는 한 줄도 안 건드려도 된다.
3. 나중에 AI가 타겟 위치로 조준하게 하고 싶으면 같은 함수만 부르면 된다. 진입점이 하나.

**왜 "Desired"(희망)인가:** 밀어넣는다고 그 프레임에 바로 쓰이는 게 아니다.
공격이 실제로 나갈 때(쿨다운 통과) `AttackAimPoint`로 승격된다. 그전까진 후보값이다.

---

# 3. TickAttack — 좌표도 같이 박제

## 무엇을 바꾸나 — CombatCharacter.cpp:414

```cpp
	// 공격 방향은 "지금" 고정한다 — 타격은 몽타주 Notify라 0.2~0.4초 뒤에 일어난다
	AttackAimDir = GetActorForwardVector();
	AttackAimPoint = DesiredAimPoint;              // <- 추가
	bHasAttackAimPoint = bHasDesiredAimPoint;      // <- 추가

	CurrentJob->Attack(); // 직업이 공격 방식을 결정(몽타주 재생 → Notify → OnAttackNotify)
	return true;
```

## 왜 하필 이 자리인가

`TickAttack` 위쪽에는 조기 리턴이 세 개 있다.

```cpp
if (!bWantsToAttack || !CurrentJob || IsDead) { return false; }   // 공격 의사 없음
if (TimeSinceLastAttack < GetAttackInterval()) { return false; }  // 쿨다운 미달
TimeSinceLastAttack = 0.0f;
```

그걸 다 통과한 이 지점이 **"이번 프레임에 공격이 진짜 나간다"가 확정되는 유일한 곳**이다.
여기가 live 값 → 박제 값으로 넘어가는 경계선이고, 2단계에서 변수를 두 쌍으로 나눈 이유가 여기서 쓰인다.

쿨다운에 막힌 프레임에서 복사해버리면, 실제로 나간 공격과 조준 시점이 어긋난다.

## 왜 bool까지 복사하나

"이번 공격이 좌표를 갖고 있었는가"도 같이 박제돼야 한다.
동료는 `bHasDesiredAimPoint`가 false라서 false가 복사되고, 4단계에서 자동으로 폴백을 탄다.
bool을 안 복사하면 동료가 `AttackAimPoint`의 초기값 `(0,0,0)` = 월드 원점을 향해 쏜다.

## 왜 `AttackAimDir` 줄을 지우지 않나

4단계의 **2순위 폴백**이다. 동료·적, 그리고 발밑 클릭이 이 줄에 의지한다. 지우면 안 된다.

---

# 4. GetAttackAimDir — 단순 반환에서 3단 폴백으로

## 무엇을 바꾸나 — CombatCharacter.cpp:421 전체 교체

**Before**

```cpp
FVector ACombatCharacter::GetAttackAimDir() const
{
	return AttackAimDir.IsNearlyZero() ? GetActorForwardVector() : AttackAimDir;
}
```

**After**

```cpp
// 좌표 > 고정 방향 > 현재 정면 순으로 폴백한다
FVector ACombatCharacter::GetAttackAimDir() const
{
	if (bHasAttackAimPoint)
	{
		FVector To = AttackAimPoint - GetActorLocation();
		To.Z = 0.0f;

		const FVector Dir = To.GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			return Dir;
		}
	}

	return AttackAimDir.IsNearlyZero() ? GetActorForwardVector() : AttackAimDir;
}
```

## 왜 바뀌나 — 여기가 실제로 문제를 고치는 지점

이 함수는 **발사 순간(Notify)에 호출된다.** `JobComponent.cpp:110`, `WarriorJob.cpp:35`, `:59`.

`AttackAimPoint - GetActorLocation()`에서 `GetActorLocation()`은 **호출되는 그 순간의 내 위치**다.
그래서 클릭 후 내가 얼마나 움직였든 그만큼이 자동으로 보정된다.

```
AttackAimPoint = (1000,0)  (박제됨, 안 변함)

t=0.0  내 위치(0,0)    -> (1000,0)-(0,0)   = (1000,0)    정면
t=0.3  내 위치(0,300)  -> (1000,0)-(0,300) = (1000,-300) 오른쪽 앞  <- 자동 보정
```

저장한 값은 그대로인데 나오는 방향이 프레임마다 다르다. 이게 좌표를 쓰는 이유 전부다.

## 왜 3단인가 — 읽는 쪽이 셋이라서

| 순위 | 반환값 | 누가 여기 걸리나 |
|---|---|---|
| 1 | 좌표에서 재계산한 방향 | 플레이어 |
| 2 | `AttackAimDir` | **동료·적** (좌표 없음), 발밑 클릭 |
| 3 | `GetActorForwardVector()` | 한 번도 공격한 적 없음 |

동료·적은 `SetDesiredAimPoint`를 안 부르므로 `bHasAttackAimPoint`가 false →
`if` 블록을 통째로 건너뛰고 기존 코드 그대로 실행된다. **동료 코드는 수정이 0줄이다.**

## 줄 단위로 왜

- **`To.Z = 0.0f`** — 커서 평면은 플레이어 Z 기준이라, 언덕에서 클릭하면 높이차만큼 위/아래로 쏜다.
- **`GetSafeNormal()` + 0 체크** — 발밑을 정확히 클릭하면 `To`가 거의 0이고, `GetSafeNormal()`은
  그때 **0벡터**를 돌려준다. 0벡터를 `Rotation()`에 넣으면 yaw=0 → 월드 +X축으로 쏜다.
  `if`를 통과 못 하고 아래 2순위로 떨어지게 만드는 게 목적.
- **`Dir`가 0이면 return 안 하고 흘려보냄** — `else`를 쓰지 않고 그냥 아래로 떨어뜨린다. 폴백 사다리라서.
- **180도 뒤집힘은 막지 않는다** — 목표점을 지나쳐 달리면 방향이 뒤로 뒤집히는데, 그게 맞는 동작이다.
  목표가 실제로 내 뒤에 있으니까.

---

# 5. UpdateAimAndAttack — 공격 중에 몸도 그 좌표를 보게

## 무엇을 바꾸나 — MyPlayer.cpp:373

**Before**

```cpp
if (bAiming) { ... }
else if (AttackFacingHold <= 0.0f && Move.SizeSquared() > InputDeadzone * InputDeadzone)
{
    // 이동 방향으로 보간
}
```

**After** — `else if` 하나를 **중간에** 끼워넣는다

```cpp
    if (bAiming)
    {
        const FVector AimDir(Aim.Y, Aim.X, 0.0f); // 이동과 동일한 축 매핑
        SetActorRotation(FRotator(0.0f, AimDir.Rotation().Yaw, 0.0f));
    }
    else if (AttackFacingHold > 0.0f)
    {
        // 공격 모션 중에는 고정해둔 조준 좌표를 계속 바라본다
        SetActorRotation(FRotator(0.0f, GetAttackAimDir().Rotation().Yaw, 0.0f));
    }
    else if (Move.SizeSquared() > InputDeadzone * InputDeadzone)
    {
        const FVector MoveDir(Move.Y, Move.X, 0.0f);
        const FRotator TargetRot(0.0f, MoveDir.Rotation().Yaw, 0.0f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnInterpSpeed));
    }
```

## 왜 바뀌나 — 동결에서 추적으로

`AttackFacingHold`가 하던 일이 바뀐다.

| `AttackFacingHold > 0` 일 때 | 기존 | 변경 후 |
|---|---|---|
| 동작 | **아무 분기도 안 탐** = 회전 동결 | 조준 좌표를 바라봄 = 추적 |

기존엔 `AttackFacingHold <= 0.0f &&` 조건으로 이동 회전을 **막기만** 했다.
막으면 클릭 순간의 각도에 몸이 그대로 멈춘다. 근데 4단계에서 발사 방향은 매 프레임 재계산된다.
→ **몸은 멈춰 있고 화살만 각도가 바뀐다.** 옆으로 달리면 몸이랑 화살이 따로 논다.

이제 `GetAttackAimDir()`로 회전시키면 발사 방향과 **똑같은 계산**을 몸에도 적용하는 셈이다.
달리는 동안 몸이 클릭 지점을 따라 돌고, 몸 방향 = 발사 방향이 항상 보장된다.
→ 별도 각도 클램프가 필요 없어진다.

## 왜 하필 그 자리(중간)인가

`if / else if` 사슬은 위에서부터 걸린다. 우선순위가 곧 순서다.

```
1. bAiming              좌클릭 꾹 누르는 중 -> 커서 실시간 추적 (제일 우선)
2. AttackFacingHold > 0  탭 클릭 후 몽타주 재생 중 -> 조준 좌표 추적
3. 이동 중               평상시 -> 이동 방향으로 부드럽게 보간
```

2번을 맨 아래로 내리면 이동 중일 때 3번이 먼저 걸려서 몸이 이동 방향으로 돌아간다 = 원래 버그.
2번을 맨 위로 올리면 꾹 누르는 중에도 옛날 좌표를 봐서 커서 추적이 끊긴다.

**`RInterpTo`가 아니라 `SetActorRotation` 스냅인 이유:** 보간하면 회전이 끝나기 전에 Notify가 터져서
몸이 중간 각도일 때 발사된다. 발사 프레임에 몸 = 조준 방향이 되려면 스냅이어야 한다.

---

# 검증 포인트

- `SetDesiredAimPoint` 호출이 `TickAttack`보다 먼저인가
  → Tick 순서 `MouseInput` → `UpdateMovement` → `UpdateAimAndAttack`(안에서 `TickAttack`). 1단계에 넣으면 만족.
- `To.Z = 0.0f`를 4단계에 넣었는가
- `GetSafeNormal()` 0 체크를 1단계·4단계 **둘 다** 했는가
- 축 스왑 `FVector2D(AimDir.Y, AimDir.X)`를 조준 경로에도 했는가 (월드 X=앞,Y=오른쪽 → 스틱 X=오른쪽,Y=위)

# 테스트

| # | 조작 | 기대 | 무엇을 판별하나 |
|---|---|---|---|
| 1 | 제자리 탭 클릭 | 커서로 나감 | 회귀 확인 |
| 2 | **옆으로 달리며 탭 클릭** | 클릭 지점으로 수렴 | **이번 작업의 핵심** (4단계) |
| 3 | 발 근처(StopRadius 안) 클릭 | 공격이 씹히지 않음 | 1단계 분리가 됐는지 |
| 4 | 적에게 달려가며 근접 공격 | 몸이 이상하게 안 돎 | 5단계 |
| 5 | 발밑을 정확히 클릭 | +X축으로 안 쏨 | 0벡터 폴백 |
| 6 | 동료가 적을 때림 | 정상 | 회귀 (2순위 폴백) |

# 알려진 한계 (이번 범위 밖)

우클릭+좌클릭 동시에 누르면 커서가 하나라 **이동 방향 = 공격 방향**이 된다.
"달리면서 반대쪽 공격"이 구조적으로 불가능하다.
원하면 우클릭을 목적지 지정형으로 바꾸거나, 좌클릭 중엔 이동 방향을 직전 값으로 고정해야 한다.
1~5단계와 독립이라 나중에 정해도 된다.
