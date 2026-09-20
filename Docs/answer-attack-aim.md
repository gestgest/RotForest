# 답안지 — 공격 조준을 좌표로

```
클릭:      나(0,0)  커서(1000,0)
0.3초 뒤:  나(0,300)
  방향 (1,0)으로 쏨       -> (1000,300) 쪽. 빗나감
  (1000,0)-(0,300) 재계산 -> (1000,-300). 명중
```

---

## 1-A. AMyPlayer 멤버 추가 — MyPlayer.h:203 `LastCursorDir` 옆

`Cursor`는 지역 변수라 0.3초 뒤 Notify까지 못 가고, `MouseAim`은 `FVector2D`라 거리가 버려진다.
bool이 따로 있는 건 `FVector` 기본값 `(0,0,0)`이 "값 없음"이 아니라 월드 원점이라서.

```cpp
	// 직전 유효 커서 방향(월드, 수평). 커서 변환이 실패한 프레임에 이걸 재사용해 끊김 방지.
	FVector LastCursorDir = FVector::ForwardVector;

	// 직전 유효 커서 좌표(월드). bHasLastCursorPoint가 true일 때만 유효.
	FVector LastCursorPoint = FVector::ZeroVector;
	bool bHasLastCursorPoint = false;
```

## 1-B. MouseInput 전체 교체 — MyPlayer.cpp:271

조준이 `MoveDir`에 얹혀 있어서 발 근처 클릭 시 `bAiming=false`, 공격이 아예 안 나갔다.
`ToCursor`는 단락 평가 때문에 가까울 때 정규화가 안 되므로 재사용 금지.

```cpp
void AMyPlayer::MouseInput(FVector2D & MouseMove, FVector2D & MouseAim)
{
    if (!bRightMouseHeld && !bLeftMouseHeld)
    {
        return;
    }

    FVector MoveDir = FVector::ZeroVector;
    FVector Cursor;

    // [커서] 이번 프레임 커서 좌표. 실패하면 직전 값을 그대로 쓴다.
    if (GetCursorGroundLocation(Cursor))
    {
        LastCursorPoint = Cursor;
        bHasLastCursorPoint = true;

        FVector ToCursor = Cursor - GetActorLocation();
        ToCursor.Z = 0.0f;

        // [이동] CursorStopRadius는 너무 가까우면 player가 진자운동하는 버그 해결용.
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

    // [조준] StopRadius 컷을 타지 않는다.
    if (bLeftMouseHeld && bHasLastCursorPoint)
    {
        FVector ToAim = LastCursorPoint - GetActorLocation();
        ToAim.Z = 0.0f;
        const FVector AimDir = ToAim.GetSafeNormal();
        if (!AimDir.IsNearlyZero())
        {
            MouseAim = FVector2D(AimDir.Y, AimDir.X);
        }

        // Tick 순서상 TickAttack보다 먼저 실행된다. => 자세한 내용은 노션 개발문서 참고
        SetDesiredAimPoint(LastCursorPoint);
    }
}
```

## 2. ACombatCharacter 멤버 — CombatCharacter.h:207 `AttackAimDir` 옆 (protected)

`DesiredAimPoint`는 매 프레임 덮어써지는 live, `AttackAimPoint`는 공격 시작에 박제.
`AMyPlayer`가 아닌 이유는 `ACompanion::Tick`도 `TickAttack`을 부르기 때문.

```
t=0.0  (1000,0) 클릭, 몽타주 시작
t=0.1  커서를 (0,1000)으로 휙 이동
t=0.3  Notify 발사
  1개면   -> (0,1000)으로 쏨. 조준한 적도 없는 곳
  2쌍이면 -> (1000,0) 그대로. 조준한 곳으로 쏨
```

```cpp
	//공격 시작 순간의 정면 방향(TickAttack이 고정).
	FVector AttackAimDir = FVector::ZeroVector;

	//공격 시작 순간에 고정한 조준 좌표(월드).
	FVector AttackAimPoint = FVector::ZeroVector;
	bool bHasAttackAimPoint = false;

	//외부가 밀어넣는 조준 좌표. 안 들어오면 방향 방식으로 동작한다.
	FVector DesiredAimPoint = FVector::ZeroVector;
	bool bHasDesiredAimPoint = false;
```

## 2-B. SetDesiredAimPoint — CombatCharacter.h:264 `GetAttackAimDir()` 옆 (public)

좌표와 bool을 짝으로 묶고, 안 부르는 게 곧 옵트아웃이 된다 — 동료·적은 안 부른다.

```
LastCursorPoint ──SetDesiredAimPoint()──> DesiredAimPoint ──TickAttack이 복사──> AttackAimPoint
```

```cpp
	// 조준 좌표 지정. 동료/적은 부르지 않는다.
	void SetDesiredAimPoint(const FVector& Point);
```

```cpp
void ACombatCharacter::SetDesiredAimPoint(const FVector& Point)
{
	DesiredAimPoint = Point;
	bHasDesiredAimPoint = true;
}
```

## 3. TickAttack에서 좌표도 고정 — CombatCharacter.cpp:414

조기 리턴을 다 통과한 이 지점이 "공격이 진짜 나간다"가 확정되는 유일한 곳.
bool을 안 복사하면 동료가 `(0,0,0)` = 월드 원점을 쏜다.

```cpp
	// 공격 방향은 "지금" 고정한다 — 타격은 몽타주 Notify라 0.2~0.4초 뒤에 일어난다.
	AttackAimDir = GetActorForwardVector();
	AttackAimPoint = DesiredAimPoint;
	bHasAttackAimPoint = bHasDesiredAimPoint;

	CurrentJob->Attack(); // 직업이 공격 방식을 결정(몽타주 재생 → Notify → OnAttackNotify)
	return true;
```

## 4. GetAttackAimDir 3단 폴백 — CombatCharacter.cpp:421 전체 교체

발사 순간(Notify)에 불리고, `GetActorLocation()`이 그 순간 값이라 이동분이 자동 보정된다.
2순위 `AttackAimDir`는 동료·적과 발밑 클릭이 쓰므로 지우면 안 된다.

```
AttackAimPoint = (1000,0) 고정
t=0.0  나(0,0)   -> (1000,0)    정면
t=0.3  나(0,300) -> (1000,-300) 오른쪽 앞
```

```cpp
// 좌표 > 고정 방향 > 현재 정면 순으로 폴백한다.
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

## 5. 공격 중 조준 좌표 바라보기 — MyPlayer.cpp:373 `UpdateAimAndAttack`

기존엔 회전을 막기만 해서 몸은 멈추고 화살만 각도가 바뀌었다.
`else if`는 중간에 — 아래면 이동 분기가 먼저 걸리고, 위면 커서 추적이 끊긴다.

```cpp
    if (bAiming)
    {
        const FVector AimDir(Aim.Y, Aim.X, 0.0f); // 이동과 동일한 축 매핑
        SetActorRotation(FRotator(0.0f, AimDir.Rotation().Yaw, 0.0f));
    }
    else if (AttackFacingHold > 0.0f)
    {
        // 공격 모션 중에는 고정해둔 조준 좌표를 계속 바라본다.
        SetActorRotation(FRotator(0.0f, GetAttackAimDir().Rotation().Yaw, 0.0f));
    }
    else if (Move.SizeSquared() > InputDeadzone * InputDeadzone)
    {
        const FVector MoveDir(Move.Y, Move.X, 0.0f);
        const FRotator TargetRot(0.0f, MoveDir.Rotation().Yaw, 0.0f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnInterpSpeed));
    }
```

---

## 검증 포인트
- `SetDesiredAimPoint` 호출이 `TickAttack`보다 먼저인가 — Tick 순서 `MouseInput` -> `UpdateMovement` -> `UpdateAimAndAttack`
- `To.Z = 0.0f`를 4단계에 넣었는가 — 안 넣으면 언덕에서 위아래로 쏜다
- `GetSafeNormal()` 0 체크를 1·4단계 둘 다 했는가 — 0벡터를 `Rotation()`에 넣으면 +X축으로 쏜다
- 축 스왑 `FVector2D(AimDir.Y, AimDir.X)`를 조준 경로에도 했는가

## 테스트
| # | 조작 | 기대 | 판별 대상 |
|---|---|---|---|
| 1 | 제자리 탭 클릭 | 커서로 나감 | 회귀 |
| 2 | **옆으로 달리며 탭 클릭** | 클릭 지점으로 수렴 | **핵심** (4단계) |
| 3 | 발 근처(StopRadius 안) 클릭 | 공격이 안 씹힘 | 1-B |
| 4 | 적에게 달려가며 근접 공격 | 몸이 이상하게 안 돎 | 5단계 |
| 5 | 발밑을 정확히 클릭 | +X축으로 안 쏨 | 0벡터 폴백 |
| 6 | 동료가 적을 때림 | 정상 | 회귀 (2순위 폴백) |
