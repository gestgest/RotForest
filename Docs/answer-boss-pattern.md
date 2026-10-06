# 답안지 — 보스 패턴: 멈칫 → 돌진

흐름: `Chase(추격) → Windup(멈춰서 노려봄) → Charge(직선 돌진) → Recover(숨고르기) → Chase`
- Chase 중 `ChargeCheckInterval`마다 `ChargeChance` 확률로 굴림
- 거리가 `ChargeMinDistance` ~ `ChargeMaxDistance` 사이일 때만 발동
- 돌진 방향은 Windup이 끝나는 순간의 대상 위치로 고정 → 마지막에 피할 수 있음

---

## 1. 추격 차단 훅 — Enemy.h:31 (protected:)

```cpp
	// 패턴 중엔 false로 => 추격 멈춤
	virtual bool CanTrack() const { return true; }
```

## 2. 추격 차단 적용 — Enemy.cpp:78 (TrackingPlayer, bFrozen 체크 바로 아래)

```cpp
	if (!CanTrack())
	{
		return;
	}
```

## 3. 패턴 enum — Boss.h:7 (`#include "Boss.generated.h"` 위)

```cpp
enum class EBossPattern : uint8
{
	Chase,
	Windup,
	Charge,
	Recover
};
```

## 4. 선언 추가 — Boss.h

### 4-1. protected: (`LeaveCombatRadius` 아래)

```cpp
	virtual bool CanTrack() const override;

	// [돌진]
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeCheckInterval = 1.0f;

	// 0.25 => 굴릴 때마다 25%
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeChance = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeMinDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeMaxDistance = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float WindupTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float RecoverTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	int32 ChargeDamage = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	float ChargeHitRadius = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	UAnimMontage* WindupMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Charge")
	UAnimMontage* ChargeMontage = nullptr;
```

### 4-2. private: (`bInCombat` 아래)

```cpp
	// [패턴]
	EBossPattern Pattern = EBossPattern::Chase;
	float PatternTime = 0.0f;
	float ChargeCheckRemain = 0.0f;
	float DefaultWalkSpeed = 0.0f;
	FVector ChargeDir = FVector::ZeroVector;
	bool bChargeHit = false;
	TWeakObjectPtr<ACombatCharacter> PatternTarget;

	void UpdatePattern(float DeltaTime);
	void SetPattern(EBossPattern NewPattern);
	void TryStartCharge();
	void TickCharge();
```

## 5. include — Boss.cpp:7 (기존 include 아래)

```cpp
#include "Characters/CombatRegistrySubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/OverlapResult.h"
```

## 6. Tick에서 호출 — Boss.cpp (ABoss::Tick)

```cpp
void ABoss::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateEncounter();
    UpdatePattern(DeltaTime);
}
```

## 7. 죽으면 패턴 해제 — Boss.cpp (ABoss::OnDeath, Super::OnDeath() 바로 아래)

```cpp
    if (Pattern != EBossPattern::Chase)
    {
        SetPattern(EBossPattern::Chase);
    }
```

## 8. 패턴 본체 — Boss.cpp 맨 아래에 추가

```cpp
// [패턴]
bool ABoss::CanTrack() const
{
    return Pattern == EBossPattern::Chase;
}

void ABoss::UpdatePattern(float DeltaTime)
{
    // 전투 밖이면 추격으로 복귀
    if (IsDead || !bInCombat)
    {
        if (Pattern != EBossPattern::Chase)
        {
            SetPattern(EBossPattern::Chase);
        }
        return;
    }

    PatternTime += DeltaTime;

    switch (Pattern)
    {
    case EBossPattern::Chase:
        ChargeCheckRemain -= DeltaTime;
        if (ChargeCheckRemain <= 0.0f)
        {
            ChargeCheckRemain = ChargeCheckInterval;
            TryStartCharge();
        }
        break;

    case EBossPattern::Windup:
        if (PatternTime >= WindupTime)
        {
            SetPattern(EBossPattern::Charge);
        }
        break;

    case EBossPattern::Charge:
        TickCharge();
        if (PatternTime >= ChargeTime)
        {
            SetPattern(EBossPattern::Recover);
        }
        break;

    case EBossPattern::Recover:
        if (PatternTime >= RecoverTime)
        {
            SetPattern(EBossPattern::Chase);
        }
        break;
    }
}

void ABoss::TryStartCharge()
{
    // 일반 공격 중이면 패스
    UAnimInstance* Anim = GetMesh()->GetAnimInstance();
    if (Anim && Anim->IsAnyMontagePlaying())
    {
        return;
    }

    if (FMath::FRand() > ChargeChance)
    {
        return;
    }

    UCombatRegistrySubsystem* Reg = GetWorld()->GetSubsystem<UCombatRegistrySubsystem>();
    if (!Reg)
    {
        return;
    }

    ACombatCharacter* Target = Reg->FindNearestOfEnemy(GetActorLocation(), ETeam::Ally, ChargeMaxDistance);
    if (!Target)
    {
        return;
    }

    // 너무 가까우면 그냥 때리는 게 낫다
    if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) < ChargeMinDistance)
    {
        return;
    }

    PatternTarget = Target;
    SetPattern(EBossPattern::Windup);
}

void ABoss::SetPattern(EBossPattern NewPattern)
{
    Pattern = NewPattern;
    PatternTime = 0.0f;

    AAIController* AI = Cast<AAIController>(GetController());
    UCharacterMovementComponent* Move = GetCharacterMovement();

    switch (NewPattern)
    {
    case EBossPattern::Chase:
        Move->MaxWalkSpeed = DefaultWalkSpeed;
        PatternTarget.Reset();
        ChargeCheckRemain = ChargeCheckInterval;
        break;

    case EBossPattern::Windup:
        DefaultWalkSpeed = Move->MaxWalkSpeed;
        if (AI)
        {
            AI->StopMovement();
            AI->SetFocus(PatternTarget.Get());
        }
        Move->StopMovementImmediately();
        if (WindupMontage)
        {
            PlayAnimMontage(WindupMontage);
        }
        break;

    case EBossPattern::Charge:
    {
        if (AI)
        {
            AI->ClearFocus(EAIFocusPriority::Gameplay);
        }

        ChargeDir = GetActorForwardVector();
        if (ACombatCharacter* Target = PatternTarget.Get())
        {
            ChargeDir = Target->GetActorLocation() - GetActorLocation();
        }
        ChargeDir.Z = 0.0f;
        ChargeDir = ChargeDir.GetSafeNormal();

        SetActorRotation(ChargeDir.Rotation());
        Move->MaxWalkSpeed = ChargeSpeed;
        bChargeHit = false;
        if (ChargeMontage)
        {
            PlayAnimMontage(ChargeMontage);
        }
        break;
    }

    case EBossPattern::Recover:
        Move->MaxWalkSpeed = DefaultWalkSpeed;
        Move->StopMovementImmediately();
        break;
    }
}

void ABoss::TickCharge()
{
    AddMovementInput(ChargeDir, 1.0f);

    // 한 번 돌진에 한 번만 타격
    if (bChargeHit)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(ChargeHitRadius),
        Params);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        ACombatCharacter* Victim = Cast<ACombatCharacter>(Overlap.GetActor());
        if (Victim && !Victim->GetIsDead() && !Victim->IsA(AEnemy::StaticClass()))
        {
            Victim->AddHP(-ChargeDamage);
            Victim->LaunchCharacter(ChargeDir * 800.0f + FVector(0, 0, 200), true, true);
            bChargeHit = true;
        }
    }
}
```

## 9. 빌드 / 확인

- 헤더 변경 → 에디터 끄고 VS 빌드 (Live Coding 금지)
- BP_Boss → Class Defaults → `Pattern|Charge`에서 수치 조절
- 테스트할 땐 `ChargeChance = 1.0`으로 두면 거리만 맞으면 매번 돌진
- 몽타주가 없으면 멈춤/돌진 동작이 애니메이션 없이 미끄러지듯 보임 (정상)
