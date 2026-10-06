#include "Characters/Boss.h"
#include "Characters/MyPlayer.h"
#include "Characters/CombatRegistrySubsystem.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/OverlapResult.h"

#include "InfiniteMapGenerator.h" // 클리어 기록을 남길 곳 (POIStates)

#include "Items/WeaponPickup.h" // 바닥에 떨굴 전리품
#include "Items/ItemDataAsset.h"

#include "UI/MyCanvas.h"

#include "Kismet/GameplayStatics.h" // FinishSpawningActor


void ABoss::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateEncounter();
    UpdatePattern(DeltaTime);
}

void ABoss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bInCombat)
    {
        bInCombat = false;
        if (UMyCanvas* Canvas = GetPlayerCanvas())
        {
            Canvas->EndBossEncounter(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ABoss::SetHome(AInfiniteMapGenerator* InGenerator, const FIntPoint& InCenterChunk)
{
    HomeGenerator = InGenerator;
    HomeChunk = InCenterChunk;
    bHomeSet = (InGenerator != nullptr);
}

void ABoss::UpdatePattern(float DeltaTime)
{
    // 죽었거나 전투중이 아닌 경우
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
        ChargeRemainTime -= DeltaTime;

        // 돌진 쿨타임
        if (ChargeRemainTime <= 0.0f)
        {
            ChargeRemainTime = ChargeCheckInterval;
            TryStartCharge();
        }
        break;

    // 대기
    case EBossPattern::Windup:
        if (PatternTime >= WindupTime)
        {
            // 충분히 쉬었다면 돌진
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


bool ABoss::CanTrack() const
{
    return Pattern == EBossPattern::Chase;
}

void ABoss::OnDeath()
{
    Super::OnDeath();   // AI정지 + 몽타주중단 + 이동차단 + 콜리전해제 + 경험치 + DeadEnemySignal

    SetLifeSpan(5.0f);  // 보스는 풀 반납이 없으니 시체를 직접 치운다

    // 드롭은 마을 소속과 무관하다 — bHomeSet 체크보다 위에 둬야 소속 없는 보스도 전리품을 남긴다
    SpawnDropPickup();

    // 여기서 기록하지 않으면 청크가 언로드될 때 시체까지 Destroy되고,
    // 다시 방문했을 때 생성기가 아무것도 모른 채 풀피 보스를 새로 세운다.
    if (!bHomeSet)
    {
        return;
    }

    //죽였다는 값 설정
    if (AInfiniteMapGenerator* Generator = HomeGenerator.Get())
    {
        Generator->MarkBossKilled(HomeChunk);
    }
}

void ABoss::SpawnDropPickup()
{
    UWorld* World = GetWorld();
    if (!World || !DropPickupClass || DropTable.Num() == 0)
    {
        return;
    }
    // 떨굴 무기 한 자루를 고른다
    UWeaponDataAsset* Drop = DropTable[FMath::RandRange(0, DropTable.Num() - 1)];
    if (!Drop)
    {
        return;
    }

    const FTransform SpawnTM(FRotator::ZeroRotator, GetActorLocation());

    // 지연 스폰: WeaponPickup의 BeginPlay가 WeaponItemData->Mesh를 읽어 외형을 꽂는다.
    // 그 전에 데이터를 넣어야 무기가 보인다.
    AWeaponPickup* Pickup = World->SpawnActorDeferred<AWeaponPickup>(
        DropPickupClass, SpawnTM, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (!Pickup)
    {
        return;
    }

    Pickup->WeaponItemData = Drop;

    UGameplayStatics::FinishSpawningActor(Pickup, SpawnTM);

    // 청크가 언로드될 때 같이 치워지게 등록 — 안 하면 아무도 안 치우는 미아가 된다
    if (AInfiniteMapGenerator* Generator = HomeGenerator.Get())
    {
        Generator->RegisterChunkActor(HomeChunk, Pickup);
    }
}

void ABoss::UpdateEncounter()
{
    // this는 월드 안에 사는 객체 => 즉 객체를 넣으면 알아서 월드를 알 수 있다.
    AMyPlayer* Player = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(this, 0));

    bool bShouldCombat = false;

    // 플레이어가 죽지 않았다면
    if (!IsDead && Player && !Player->GetIsDead())
    {
        // 범위를 벗어나면 UI 종료
        const float Radius = bInCombat ? LeaveCombatRadius : CombatRadius;
        bShouldCombat = FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(Radius);
    }

    if (bShouldCombat == bInCombat)
    {
        return;
    }
    bInCombat = bShouldCombat;

    UMyCanvas* Canvas = GetPlayerCanvas();
    if (!Canvas)
    {
        return;
    }

    // 전투중
    if (bInCombat)
    {
        Canvas->StartBossEncounter(this, BossName);
    }
    else
    {
        Canvas->EndBossEncounter(this);
    }
}

UMyCanvas* ABoss::GetPlayerCanvas() const 
{
    AMyPlayer* Player = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(this, 0));
    return Player ? Player->GetCanvasWidget() : nullptr;
}

void ABoss::SetPattern(EBossPattern NewPattern)
{
    Pattern = NewPattern;
    PatternTime = 0.0f; // 일단 패턴 시간 초기화

    AAIController* AI = Cast<AAIController>(GetController());
    UCharacterMovementComponent* Move = GetCharacterMovement();

    switch (NewPattern) {
    case EBossPattern::Chase:
        Move->MaxWalkSpeed = TempWalkSpeed;
        PatternTarget.Reset();
        ChargeRemainTime = ChargeCheckInterval;
        break;

    case EBossPattern::Windup:
        TempWalkSpeed = Move->MaxWalkSpeed;

        // 멈춰
        if (AI)
        {
            AI->StopMovement(); // 명령 취소 (관성은 있음)
            AI->SetFocus(PatternTarget.Get());
        }
        Move->StopMovementImmediately(); // 즉시 속도 0

        if (WindupMontage)
        {
            PlayAnimMontage(WindupMontage);
        }
        break;

    case EBossPattern::Charge:
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
        ChargeDir = ChargeDir.GetSafeNormal(); // 정규화

        SetActorRotation(ChargeDir.Rotation()); // 벡터 -> 쿼터니언
        Move->MaxWalkSpeed = ChargeSpeed;
        bChargeHit = false;
        if (ChargeMontage)
        {
            PlayAnimMontage(ChargeMontage);
        }
        break;
    case EBossPattern::Recover:
        Move->MaxWalkSpeed = TempWalkSpeed;
        Move->StopMovementImmediately(); //멈추기
        break;
    }

}

// 돌진 주사위 시도
void ABoss::TryStartCharge()
{
    UAnimInstance* Anim = GetMesh()->GetAnimInstance();
    if (Anim && Anim->IsAnyMontagePlaying())
    {
        return;
    }

    // 랜덤에 실패한 경우
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

    // 거리가 가깝다면
    if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) < ChargeMinDistance)
    {
        return;
    }
    PatternTarget = Target;
    SetPattern(EBossPattern::Windup);
}

// 돌진중... => 닿았다면 피해를 입히는 함수
void ABoss::TickCharge()
{
    AddMovementInput(ChargeDir, 1.0f);

    // 한번 때린 경우
    if (bChargeHit)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); //자기자신은 무시

    GetWorld()->OverlapMultiByChannel(
        Overlaps, // output
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(ChargeHitRadius),
        Params
    );

    for (const FOverlapResult& Overlap : Overlaps)
    {
        // 맞는 사람
        ACombatCharacter* Victim = Cast<ACombatCharacter>(Overlap.GetActor());

        // 그 캐릭터가 죽지 않은 경우, 아군이 아닌 경우 => 맞아버림
        if (Victim && !Victim->GetIsDead() && Victim->TeamType != TeamType)
        {
            Victim->AddHP(-Damage * ChargeDamageMultiplier);

            Victim->LaunchCharacter(ChargeDir * ChargePower + FVector(0, 0, ChargeDamagedHeight), true, true);
            bChargeHit = true;

            if (AttackSound)
            {
                continue;
            }

            // 사운드
            UGameplayStatics::PlaySoundAtLocation(
                this, //GetWorld 참조용. 플레이어 뿐만 아니라 월드에 있는 아무 오브젝트 사용 가능
                AttackSound,
                this->GetActorLocation()
            );
        }
    }
}
