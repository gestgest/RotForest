// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Enemy.h"
#include "Characters/MyPlayer.h"
#include "Characters/Companion.h" //동료도 추격 대상
#include "NavigationSystem.h"  // 내비게이션 사용 시
#include "Kismet/GameplayStatics.h" //getCharacter, sound
//#include "Kismet/KismetSystemLibrary.h" //ray


#include "Navigation/PathFollowingComponent.h" //GetPathFollowingComponent에 있는 OnRequestFinished
#include "AITypes.h"
#include "GameFramework/CharacterMovementComponent.h" //죽을 때 이동 정지
#include "Components/CapsuleComponent.h"             //죽을 때 충돌 해제
#include <Characters/CombatRegistrySubsystem.h>

// Sets default values
AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true; //update

	//default
	TeamType = ETeam::Enemy;

	// 기본 AI 컨트롤러 클래스 지정 — SpawnDefaultController()가 항상 생성할 대상을 갖도록.
	// (BP에서 None으로 비워두면 SpawnDefaultController가 아무것도 안 만들어 크래시 위험)
	AIControllerClass = AAIController::StaticClass();

	// 머리 위 HP 바 — 생성/갱신 로직은 베이스(ACombatCharacter) 소유. 위젯 클래스는 BP_Enemy에서 지정.
	CreateHPBarComponent();


}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	CanAttack = true;
	
	UAnimInstance* animInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());
	aiController = Cast<AAIController>(GetController());

	if (animInstance)
	{
		//AttackMontage-> 몽타주 세팅
		animInstance->OnMontageEnded.AddDynamic(this, &AEnemy::OnMontageEnded);

		// 공격 Notify 배선은 베이스(ACombatCharacter)가 처리한다(→ HandleAttackNotify).
	}

	if (aiController)
	{
		//추격을 완료했다면 - 콜백
		aiController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TrackingPlayer();
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

//추격하는 함수 — 플레이어와 동료 중 "가장 가까운 살아있는 아군"을 쫓는다.
// (이름은 BP 호환을 위해 TrackingPlayer 유지)
void AEnemy::TrackingPlayer()
{
	// 시간 정지 중(플레이어 사망 연출)에는 추격 갱신 자체를 하지 않는다.
	if (bFrozen)
	{
		return;
	}

	// 갱신 간격 제한 — BP가 매 프레임 호출해도 여기서 걸러진다.
	// 경로 재요청은 프레임 단위로 할 필요가 없고, 매 프레임 하면 적 수만큼 길찾기 비용이 쌓인다.
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextTrackTime)
	{
		return;
	}
	if(IsDead)
	{
		return;
	}
	const FVector myLocation = GetActorLocation();
	UCombatRegistrySubsystem* Reg = GetWorld()->GetSubsystem<UCombatRegistrySubsystem>();
	if (!Reg)
	{
		return;
	}
	
	ACombatCharacter * target = Reg->FindNearestOfEnemy(myLocation, ETeam::Ally, 2500);


	// 전원 사망 → 쫓을 대상 없음
	if (!target)
	{
		return;
	}

	const float targetDist = FVector::Dist2D(myLocation, target->GetActorLocation());

	const float Interval = (targetDist > TrackFarDistance) ? TrackFarInterval : TrackNearInterval;
	NextTrackTime = Now + Interval * FMath::FRandRange(0.9f, 1.1f);

	if (aiController)
	{
		aiController->SetFocus(target);
		aiController->MoveToActor(target, AttackRange);
	}
}

//추격을 완료했다면
void AEnemy::MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (IsDead)
	{
		return; // 죽은 적은 추격 완료해도 공격하지 않음
	}

	if (Result.Code == EPathFollowingResult::Success)
	{
		if (CanAttack)
		{
			PlayAnimMontage(DefaultAttackMontage); //지속시간 매개변수 있음
			//UE_LOG(LogTemp, Warning, TEXT("Montage Duration: %f"), duration);
			CanAttack = false;
		}
	}

}

//공격 몽타주 애니메이션이 끝났다면
void AEnemy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	CanAttack = true;
}

//공격 몽타주 Notify 처리(베이스에서 호출) — "Attack" 신호에 자체 타격을 수행.
void AEnemy::HandleAttackNotify(FName NotifyName)
{
	//UE_LOG(LogTemp, Log, TEXT("attack signal"));
	if (NotifyName == FName("Attack"))
	{
		if (hit())
		{
			//사운드
			if (AttackSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,  // WorldContextObject
					AttackSound,
					GetActorLocation(),
					1.0f,  // VolumeMultiplier
					1.0f   // PitchMultiplier
				);
			}
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, FString::Printf(TEXT("NO")));
		}
	}
}


//일단 때리는 애니메이션
bool AEnemy::hit()
{
	if (IsDead)
	{
		return false; // 죽은 적은 데미지를 주지 않음 (몽타주 노티파이가 늦게 터져도 무효)
	}

	bool isAttack = false;

	//여기에 함수 죄다 넣어야 함
	TArray<FHitResult> hitResults;
	FVector start = GetActorLocation();
	FVector end = start + (GetActorForwardVector() * AttackRange);
	

	FCollisionShape sphere = FCollisionShape::MakeSphere(AttackRange);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, //TraceChannel
		sphere,
		queryParams
	);
	// 끝 위치에 구 그리기
	//DrawDebugSphere(
	//	GetWorld(),
	//	end,
	//	AttackRange,
	//	32,
	//	FColor::Green,
	//	false,
	//	2.0f
	//);

	// Sweep 경로를 선으로 연결
	DrawDebugLine(
		GetWorld(),
		start,
		end,
		FColor::Red,
		false,
		2.0f,
		0,
		2.0f
	);

	for (const FHitResult& hit : hitResults)
	{
		// 아군(플레이어/동료)만 때린다 — 적(AEnemy)끼리는 무시. 둘 다 ACombatCharacter라 공용으로 처리.
		ACombatCharacter* Victim = Cast<ACombatCharacter>(hit.GetActor());
		if (Victim && !Victim->GetIsDead() && !Victim->IsA(AEnemy::StaticClass()))
		{
			//UE_LOG(LogTemp, Log, TEXT("Hit Ally!"));
			Victim->AddHP(-Damage);
			FVector force = GetActorForwardVector() * 500 + FVector(0, 0, 100);
			Victim->LaunchCharacter(force, false, false);
			isAttack = true;
		}
	}
	return isAttack;
}

void AEnemy::DebugHPShow()
{
	UE_LOG(LogTemp, Log, TEXT("bobo : %d"), HP);
}

void AEnemy::SetID(int id)
{
	this->enemy_id = id;
}

// 살아있음 → 죽음 전환 시 베이스가 1회 호출. (HP/IsDead 갱신은 베이스 담당)
void AEnemy::OnDeath()
{
	// 죽는 즉시 추격/공격/이동을 끊는다 — 죽으면서 쫓아오거나 때리는 버그 방지.
	CanAttack = false;

	if (aiController)
	{
		aiController->StopMovement();                       // 진행 중이던 추격 취소
		aiController->ClearFocus(EAIFocusPriority::Gameplay); // 플레이어 주시 해제
	}

	StopAnimMontage(); // 진행 중이던 공격 몽타주 중단 (공격 노티파이 추가 발동 방지)

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	// 시체가 플레이어를 밀거나 길을 막지 않도록 캡슐 충돌 해제
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 처치 보상 — 죽음 "전환" 시점이라 정확히 1회만 지급된다(풀 재사용/중복 타격에도 안전).
	// 동료가 잡아도 경험치는 플레이어에게 간다.
	AMyPlayer* myPlayer = Cast<AMyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (myPlayer && !myPlayer->GetIsDead())
	{
		myPlayer->AddExp(ExpReward);
	}

	DeadEnemySignal(enemy_id); // BP: 사망 애님 재생 / 일정 시간 후 Destroy
}

// 죽음 → 부활 전환 시 베이스가 1회 호출 (게임모드 풀에서 SetHP(5)로 재사용).
// 이게 없으면 재활용된 적은 캡슐 콜리전이 꺼진 채라 공격 스윕(ECC_Pawn)에 안 맞고 피도 안 닳는다.
void AEnemy::OnRevive()
{
	CanAttack = true;

	// 캡슐 충돌 복구 — 이게 핵심. 안 켜면 때려도 SweepMultiByChannel에 안 잡힘.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// 이동 복구 — 죽을 때 DisableMovement()로 MOVE_None이 됐던 걸 다시 걷게.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
}

// 풀 대기: 숨김만으로는 부족하다 — 숨겨진 액터도 중력/틱이 돌아서
// 원점 청크가 언로드되면 풀 전체가 낙하하고, 대기 적들이 매 프레임 연산 낭비를 만든다.
void AEnemy::EnterPoolDormancy()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (aiController)
	{
		aiController->StopMovement();
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement(); // MOVE_None — 중력도 멈춘다
	}
}

void AEnemy::WakeFromPool()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
	// 캡슐 콜리전 복구는 OnRevive(SetHP로 부활 전환 시)가 담당한다.
}

void AEnemy::TeleportForLeash(const FVector& NewLocation)
{
	// 언로드된 지형을 가로지르던 옛 경로 폐기 (다음 TrackingPlayer가 새 위치에서 다시 잡는다)
	if (aiController)
	{
		aiController->StopMovement();
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move)
	{
		Move->StopMovementImmediately(); // 낙하 중이었다면 수직 속도 제거 (착지 순간 바닥 뚫기 방지)
	}

	SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (Move)
	{
		Move->SetMovementMode(MOVE_Walking); // 낙하(MOVE_Falling) 상태였어도 즉시 보행 복귀
	}
}

// 시간 정지 on/off — 플레이어가 죽으면 게임모드(SetEnemiesFrozen)가 풀 전체에 호출한다.
void AEnemy::SetFrozen(bool bNewFrozen)
{
	bFrozen = bNewFrozen;

	// 0이면 이 액터의 틱/이동/애니메이션이 전부 dt=0으로 얼어붙는다(그 자리에 정지 화면처럼).
	CustomTimeDilation = bNewFrozen ? 0.0f : 1.0f;

	if (bNewFrozen && aiController)
	{
		// AI 컨트롤러는 별개 액터라 시간 정지의 영향을 안 받는다 — 진행 중이던 경로를 직접 끊는다.
		aiController->StopMovement();
		aiController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	// 해제 시에는 아무것도 안 해도 된다 — 다음 TrackingPlayer가 추격을 다시 잡는다.
}

void AEnemy::OnAIController()
{
	// 이미 AI 컨트롤러에 빙의돼 있으면 그대로 재사용
	aiController = Cast<AAIController>(GetController());

	if (!aiController)
	{
		// 혹시 BP/직렬화로 AIControllerClass가 비어 있을 경우를 대비해 기본값을 런타임에 보장
		// (C++ 생성자 값보다 BP 저장값이 우선하므로, 여기서 한 번 더 못 박는다)
		if (AIControllerClass == nullptr)
		{
			AIControllerClass = AAIController::StaticClass();
		}

		// 비-AI 컨트롤러가 이미 빙의 중이면 SpawnDefaultController가 그냥 무시된다
		// (내부에서 Controller != null이면 early-return). 먼저 빙의를 풀어준다.
		if (AController* Existing = GetController())
		{
			Existing->UnPossess();
		}

		SpawnDefaultController(); //AI 컨트롤러 생성 + 빙의
		aiController = Cast<AAIController>(GetController());
	}

	// 그래도 실패하면 크래시 대신 경고. 정확한 원인 파악을 위해 상태를 모두 찍는다.
	if (!aiController)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AEnemy::SetAIController 실패 - AIControllerClass=%s, Controller=%s, AutoPossessAI=%d"),
			*GetNameSafe(AIControllerClass),
			*GetNameSafe(GetController()),
			(int32)AutoPossessAI);
		return;
	}

	if (UPathFollowingComponent* PFC = aiController->GetPathFollowingComponent())
	{
		PFC->OnRequestFinished.RemoveAll(this);   // ← 추가: 이미 걸려 있으면 제거 후 재등록
		PFC->OnRequestFinished.AddUObject(this, &AEnemy::MoveCompleted);
	}
}
