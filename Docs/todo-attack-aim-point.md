# 답안지 (완성 코드)

## 1. AMyPlayer 멤버 추가 — MyPlayer.h:203 `LastCursorDir` 옆

```cpp
	// 직전 유효 커서 방향(월드, 수평). 커서 변환이 실패한 프레임에 이걸 재사용해 끊김 방지.
	FVector LastCursorDir = FVector::ForwardVector;

	// 직전 유효 커서 좌표(월드). bHasLastCursorPoint가 true일 때만 유효.
	FVector LastCursorPoint = FVector::ZeroVector;
	bool bHasLastCursorPoint = false;
```

## 1. MouseInput 전체 교체 — MyPlayer.cpp:271

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

`ToCursor`를 조준에 재사용하지 않고 `LastCursorPoint`에서 새로 계산하는 게 핵심.
`else` 분기에서도 `LastCursorPoint`가 직전 값을 유지하므로 조준 폴백이 자동으로 걸린다.

## 2. ACombatCharacter 멤버 + setter

**CombatCharacter.h:207 `AttackAimDir` 옆 (protected)**

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

**CombatCharacter.h:264 `GetAttackAimDir()` 옆 (public)**

```cpp
	// 조준 좌표 지정. 동료/적은 부르지 않는다.
	void SetDesiredAimPoint(const FVector& Point);
```

**CombatCharacter.cpp — `GetAttackAimDir()` 아래**

```cpp
void ACombatCharacter::SetDesiredAimPoint(const FVector& Point)
{
	DesiredAimPoint = Point;
	bHasDesiredAimPoint = true;
}
```

## 3. TickAttack에서 좌표도 고정 — CombatCharacter.cpp:414

```cpp
	// 공격 방향은 "지금" 고정한다 — 타격은 몽타주 Notify라 0.2~0.4초 뒤에 일어난다.
	AttackAimDir = GetActorForwardVector();
	AttackAimPoint = DesiredAimPoint;
	bHasAttackAimPoint = bHasDesiredAimPoint;

	CurrentJob->Attack(); // 직업이 공격 방식을 결정(몽타주 재생 → Notify → OnAttackNotify)
	return true;
```

## 4. GetAttackAimDir 3단 폴백 — CombatCharacter.cpp:421 전체 교체

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

동료/적은 `SetDesiredAimPoint`를 안 부르므로 `bHasAttackAimPoint`가 false로 남아 기존 경로를 탄다.

## 5. 공격 중 조준 좌표 바라보기 — MyPlayer.cpp:373 `UpdateAimAndAttack`

`else if` 하나를 **중간에** 끼워넣는다. 순서가 중요하다.

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

`GetAttackAimDir()`가 매 프레임 좌표에서 다시 계산하므로, 달리는 동안 몸이 클릭 지점을 따라 돈다.
발사 방향과 몸 방향이 항상 일치하게 된다.

## 검증 포인트
- `SetDesiredAimPoint` 호출이 `TickAttack`보다 먼저인가 — Tick 순서 `MouseInput` -> `UpdateMovement` -> `UpdateAimAndAttack`
- `To.Z = 0.0f`를 4단계에 넣었는가
- `GetSafeNormal()` 결과 0 체크를 1·4단계 둘 다 했는가
- 축 스왑 `FVector2D(AimDir.Y, AimDir.X)`를 조준 경로에도 했는가
