// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Companion.h"
#include "Jobs/JobComponent.h"
#include "Characters/Enemy.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h" //진단 디버그
#include "Engine/Engine.h"     //GEngine 화면 메시지

ACompanion::ACompanion()
{
	PrimaryActorTick.bCanEverTick = true;
	
	//디폴트 설정
	TeamType = ETeam::Ally;

	// 따라다닐 땐 이동 방향으로 자연스럽게 회전. 공격 시엔 코드(FaceActor)로 적을 바라보게 덮어쓴다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	// 스폰되면 자동으로 AI 컨트롤러가 빙의하도록.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 머리 위 HP 바 — 생성/갱신 로직은 베이스(ACombatCharacter) 소유. 위젯 클래스는 BP_Companion에서 지정.
	CreateHPBarComponent();

	// 자동 공격 간격 폴백(베이스 소유). 동료는 플레이어보다 조금 느리게 — 직업에 값이 있으면 그쪽이 이긴다.
	AttackInterval = 0.6f;
}

void ACompanion::BeginPlay()
{
	Super::BeginPlay();

	SetHP(MaxHP); // 동료도 체력을 가진다(베이스). 적에게 맞으면 닳고 0이면 죽는다.

	AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		SpawnDefaultController();
		AICon = Cast<AAIController>(GetController());
	}

	// 직업 컴포넌트 생성/부착은 베이스(ACombatCharacter)가 담당 — 플레이어와 완전히 같은 경로.
	// 공격 몽타주는 이 동료(JobAttackMontages[JobType])가 자기 스켈레톤에 맞게 소유한다.
	CreateJobComponent();

	// 공격 몽타주 Notify 배선/전달도 베이스가 처리한다(→ HandleAttackNotify → CurrentJob->OnAttackNotify).

	// 의사결정 시점을 랜덤 오프셋으로 분산 — 동료 여럿이 같은 프레임에 몰려 스캔하는 스파이크 방지.
	TimeSinceDecision = FMath::FRandRange(0.0f, DecisionInterval);
}

void ACompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead)
	{
		return; // 죽었으면 추격/공격 로직 정지
	}

	TimeSinceDecision += DeltaTime;

	// 무거운 의사결정(전체 적 스캔 + 이동 명령)은 DecisionInterval마다만 실행.
	if (TimeSinceDecision >= DecisionInterval)
	{
		TimeSinceDecision = 0.0f;
		UpdateDecision();
	}

	// 반응성이 필요한 것만 매 프레임: 사거리 안 대상 조준 + 공격 타이밍.
	// "사거리 안에 살아있는 타겟이 있는가"만 판단하고, 타이머/간격/직업 호출은 베이스가 처리한다.
	const bool bTargetInRange =
		IsValid(CurrentTarget) && !CurrentTarget->GetIsDead() &&
		FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation()) <= GetEngageRange();

	if (bTargetInRange)
	{
		FaceActor(CurrentTarget);
	}

	if (TickAttack(DeltaTime, bTargetInRange) && bDebugCombat)
	{
		// 공격을 실제로 호출하는 순간 표시(노란 선 + 메시지)
		DrawDebugLine(GetWorld(), GetActorLocation(),
			CurrentTarget->GetActorLocation(), FColor::Yellow, false, 0.4f, 0, 3.0f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.4f, FColor::Yellow,
				TEXT("[Companion] Attack() 호출!"));
		}
	}
}

// 멈춰서 공격할 거리 = 직업의 교전 사거리(근접=짧게, 원거리=길게). 직업이 없으면 동료 AttackRange로 폴백.
// Tick(조준/공격)과 UpdateDecision(이동 명령)이 이 함수 하나를 공유해야 값이 어긋나지 않는다.
float ACompanion::GetEngageRange() const
{
	return (CurrentJob && CurrentJob->GetEngageRange() > 0.0f) ? CurrentJob->GetEngageRange() : AttackRange;
}

// 느린 업데이트 함수 : DecisionInterval마다 호출 — 타겟 재탐색과 이동 명령만 여기서. (매 프레임 돌릴 필요 없는 것들)
void ACompanion::UpdateDecision()
{
	CurrentTarget = FindNearestEnemy();


	if (CurrentTarget)
	{
		// === 교전 ===
		State = EState::Fighting;

		// Tick의 공격 판정과 반드시 같은 값을 써야 한다(GetEngageRange 하나로 통일).
		const float Engage = GetEngageRange();

		const float Dist = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
		if (bDebugCombat && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White,
				FString::Printf(TEXT("Dist: %.0f / Engage: %.0f"), Dist, Engage));
		}

		if (AICon)
		{
			if (Dist > Engage)
			{
				// 사거리 밖 → 적에게 접근 (조금 더 가깝게 멈추도록 수용 반경을 줄임)
				AICon->MoveToActor(CurrentTarget, Engage * 0.8f, false);
			}
			else
			{
				// 사거리 안 → 멈추고 공격 (조준/공격 타이밍은 Tick이 매 프레임 처리)
				AICon->StopMovement();
			}
		}
	}
	else if (Leader)
	{
		// === 따라다님 ===
		State = EState::Following;

		const float Dist = FVector::Dist(GetActorLocation(), Leader->GetActorLocation());
		if (Dist > FollowDistance)
		{
			if (AICon)
			{
				AICon->MoveToActor(Leader, FollowDistance);
			}
		}
		else if (AICon)
		{
			AICon->StopMovement(); // 충분히 가까우면 멈춤(졸졸 따라가다 정지)
		}
	}
	else if (bGuardHome)
	{
		// === 경비 (마을 경비병) ===
		// 리더가 없고 적도 없으면 자기 자리(HomeLocation)로 복귀 — 교전으로 자리를 벗어나도 다시 돌아온다.
		State = EState::Following;

		const float Dist = FVector::Dist2D(GetActorLocation(), HomeLocation);
		if (Dist > HomeAcceptRadius)
		{
			if (AICon)
			{
				AICon->MoveToLocation(HomeLocation, HomeAcceptRadius * 0.5f);
			}
		}
		else if (AICon)
		{
			AICon->StopMovement(); // 자리에 도착 — 대기
		}
	}
}

AEnemy* ACompanion::FindNearestEnemy() const
{
	// v1: 단순히 모든 적을 훑어 가장 가까운 살아있는 적을 고른다.
	// (적이 많아지면 오버랩 쿼리로 최적화 가능)
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);

	AEnemy* Best = nullptr;
	float BestDistSq = DetectRadius * DetectRadius;
	const FVector Loc = GetActorLocation();

	for (AActor* A : Enemies)
	{
		AEnemy* E = Cast<AEnemy>(A);
		if (!E || E->GetIsDead())
		{
			continue; // 죽은 적은 무시
		}
		const float DSq = FVector::DistSquared(Loc, E->GetActorLocation());
		if (DSq < BestDistSq)
		{
			BestDistSq = DSq;
			Best = E;
		}
	}
	return Best;
}

void ACompanion::FaceActor(AActor* Target)
{
	if (!Target)
	{
		return;
	}
	FVector Dir = Target->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.0f;
	if (Dir.Normalize())
	{
		SetActorRotation(FRotator(0.0f, Dir.Rotation().Yaw, 0.0f));
	}
}

// HandleAttackNotify는 베이스(ACombatCharacter)의 기본 구현이 직업으로 전달한다 — 플레이어와 동일.

void ACompanion::OnDeath()
{
	// 추격/공격을 끊고, 시체가 길을 막지 않게 콜리전·이동을 끈 뒤 잠시 후 제거.
	if (AICon)
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
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

	SetLifeSpan(CorpseLifeSpan); // 일정 시간 후 자동 Destroy (리더의 Companions 목록은 다음 섭외 때 정리됨)
}
