// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Villager.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AVillager::AVillager()
{
	PrimaryActorTick.bCanEverTick = true;

	TeamType = ETeam::Ally;

	// 걷는 방향으로 자연스럽게 회전 (동료와 동일한 설정)
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	// 스폰되면 자동으로 AI 컨트롤러가 빙의
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 주민은 비무장 — UCombatAnimInstance가 이 값으로 비무장 idle/walk 블렌드를 고른다
	bArmed = false;

	// 머리 위 HP 바 — 적/동료처럼 맞으면 닳는 게 보이게. 위젯 클래스는 BP에서 지정(안 하면 그냥 안 보임).
	CreateHPBarComponent();
}

void AVillager::BeginPlay()
{
	Super::BeginPlay();

	SetHP(MaxHP);

	AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		SpawnDefaultController();
		AICon = Cast<AAIController>(GetController());
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 첫 이동 시점을 랜덤 분산 — 주민 여럿이 동시에 일제히 걷기 시작하는 부자연스러움 방지
	WaitRemaining = FMath::FRandRange(0.0f, WaitTimeMax);
}

void AVillager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead)
	{
		return;
	}

	// 이동 중에는 대기 타이머를 세지 않는다 — "도착한 뒤부터" 기다렸다가 다음 목적지를 고른다
	if (GetVelocity().SizeSquared2D() > 25.0f)
	{
		return;
	}

	WaitRemaining -= DeltaTime;
	if (WaitRemaining <= 0.0f)
	{
		PickNewWanderPoint();
		WaitRemaining = FMath::FRandRange(WaitTimeMin, WaitTimeMax);
	}
}

void AVillager::PickNewWanderPoint()
{
	if (!AICon)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return;
	}

	// 배회 중심은 자기 위치가 아니라 HomeLocation — 배회가 누적돼도 마을 밖으로 새어나가지 않는다
	FNavLocation Out;
	if (NavSys->GetRandomReachablePointInRadius(HomeLocation, WanderRadius, Out))
	{
		AICon->MoveToLocation(Out.Location, 50.0f);
	}
	// 실패하면(NavMesh 미생성 등) 이번 턴은 건너뛰고 다음 대기 후 재시도
}

void AVillager::OnDeath()
{
	// 이동을 끊고, 시체가 길을 막지 않게 콜리전을 끈 뒤 잠시 후 제거 (ACompanion::OnDeath와 같은 패턴)
	if (AICon)
	{
		AICon->StopMovement();
	}

	StopAnimMontage();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetLifeSpan(CorpseLifeSpan);
}
