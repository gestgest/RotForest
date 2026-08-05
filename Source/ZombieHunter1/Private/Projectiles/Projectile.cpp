#include "Projectiles/Projectile.h"
#include "Projectiles/ProjectilePoolSubsystem.h"
#include "Characters/Enemy.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/OverlapResult.h" // FOverlapResult (범위 폭발 오버랩 결과)
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h" // 디버그 범위 시각화
#include "NiagaraFunctionLibrary.h" 


//투사체
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // 디버그 그리기용. bDrawDebug=false면 Tick에서 즉시 반환.

	// 충돌 구체를 루트로. Pawn(적)과 겹침을 감지한다.
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootComponent = CollisionSphere;

	// 시각 메시 (BP에서 화살 메시를 지정). 충돌은 구체가 담당하므로 메시는 충돌 끔.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);
	//VisualRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 직선 비행
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 탑다운: 중력 영향 없음

	// 수명 처리는 InitialLifeSpan(Destroy 기반) 대신 LifeSeconds 타이머 → 풀 반환으로 한다.
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnSphereOverlap);

	// BP가 VisualRoot 아래에 붙인 시각 컴포넌트를 한 번만 수집해 캐시.
	// 첫 인자 true = 손자 컴포넌트까지 전부 포함.
	if (VisualRoot)
	{
		VisualRoot->GetChildrenComponents(true, CachedVisuals);
	}
}

void AProjectile::ActivateFromPool()
{
	bInFlight = true;

	// 이전 사용자의 설정이 남지 않게 튜닝 값을 클래스(BP) 기본값으로 리셋.
	// 궁수/마법사가 같은 발사체 클래스를 공유해도 폭발 반경 등이 새어 들어가지 않는다.
	// (직업이 매 발사마다 Damage/속도/디버그를, 마법사가 ExplosionRadius를 다시 설정한다)
	if (const AProjectile* CDO = GetClass()->GetDefaultObject<AProjectile>())
	{
		Damage = CDO->Damage;
		KnockbackForce = CDO->KnockbackForce;
		ExplosionRadius = CDO->ExplosionRadius;
		HitSound = CDO->HitSound;
		bDrawDebug = CDO->bDrawDebug;
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	// 재사용 시 이동 컴포넌트를 다시 가동. (정지 시 UpdatedComponent가 풀릴 수 있어 재바인딩)
	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(RootComponent);
		ProjectileMovement->Activate(true);
	}

	// BP가 붙인 시각 컴포넌트를 켠다. Activate는 자식으로 전파되지 않아 직접 순회한다.
	for (USceneComponent* Visual : CachedVisuals)
	{
		if (Visual)
		{
			Visual->Activate(/*bReset=*/true); // true = 나이아가라를 처음부터 다시 재생
		}
	}

	// 디버그 사거리 라인 기준점 기록 (발사 지점/방향)
	SpawnLocation = GetActorLocation();
	SpawnForward = GetActorForwardVector();

	// 빗나간 화살은 LifeSeconds 뒤 풀로 반환
	GetWorldTimerManager().SetTimer(LifeTimerHandle, this, &AProjectile::OnLifeExpired, LifeSeconds);
}

void AProjectile::DeactivateForPool()
{
	bInFlight = false;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}


	// 시각 컴포넌트 정지. 숨기기(SetActorHiddenInGame)만으로는 파티클 시뮬레이션이 안 멈춘다.
	for (USceneComponent* Visual : CachedVisuals)
	{
		if (Visual)
		{
			Visual->Deactivate();
		}
	}
}

void AProjectile::ReturnToPool()
{
	if (UProjectilePoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UProjectilePoolSubsystem>() : nullptr)
	{
		DeactivateForPool();
		Pool->Release(this);
	}
	else
	{
		Destroy(); // 풀이 없는 예외 상황(월드 종료 중 등) 폴백
	}
}

void AProjectile::OnLifeExpired()
{
	ReturnToPool();
}

void AProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrawDebug)
	{
		return;
	}
	

	// 비행 중 충돌 구체(실제 적중 판정 범위)를 매 프레임 그린다.
	DrawDebugSphere(GetWorld(), GetActorLocation(),
		CollisionSphere->GetScaledSphereRadius(), 12, FColor::Cyan, false, -1.0f, 0, 1.0f);

	// 폭발형이면 적중 시 터질 범위를 미리 노란 구체로 보여준다.
	if (ExplosionRadius > 0.0f)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(),
			ExplosionRadius, 16, FColor::Yellow, false, -1.0f, 0, 1.0f);
	}

	// 최대 사거리 = 속도 × 수명. 발사 지점에서 진행 방향으로 라인 + 끝점 표시.
	const float Speed = ProjectileMovement ? ProjectileMovement->MaxSpeed : 0.0f;
	const float MaxRange = Speed * LifeSeconds;
	if (MaxRange > 0.0f)
	{
		const FVector EndPoint = SpawnLocation + SpawnForward * MaxRange;
		DrawDebugLine(GetWorld(), SpawnLocation, EndPoint, FColor::Green, false, -1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), EndPoint, 20.0f, 8, FColor::Green, false, -1.0f, 0, 1.0f);
	}
}

void AProjectile::SetInitialSpeed(float Speed)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		// 이미 속도가 설정돼 발사된 경우를 대비해 현재 진행 방향으로 속도 갱신
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}
}

//적이 투사체를 맞춘다면
void AProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bInFlight)
	{
		return; // 이미 풀로 반환됐는데 같은 프레임에 도착한 잔여 오버랩 이벤트 무시
	}

	AEnemy* HitEnemy = Cast<AEnemy>(OtherActor);
	if (!HitEnemy)
	{
		return; // 적이 아니면 통과 (벽/플레이어 등 무시)
	}

	// 범위 폭발: 적중 지점 주변의 모든 적에게 피해 (마법사 파이어볼 등)
	if (ExplosionRadius > 0.0f)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		GetWorld()->OverlapMultiByChannel(
			Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params);

		if (bDrawDebug)
		{
			// 실제로 터진 범위를 1.5초간 빨간 구체로 남겨서 확인.
			DrawDebugSphere(GetWorld(), GetActorLocation(),
				ExplosionRadius, 16, FColor::Red, false, 1.5f, 0, 2.0f);
		}

		TSet<AEnemy*> Damaged;
		for (const FOverlapResult& O : Overlaps)
		{
			AEnemy* Enemy = Cast<AEnemy>(O.GetActor());
			if (Enemy && !Damaged.Contains(Enemy))
			{
				Damaged.Add(Enemy);
				ApplyHit(Enemy);
			}
		}
	}
	else
	{
		// 단일 대상 (화살 등)
		ApplyHit(HitEnemy);
	}

	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, GetActorLocation());
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}

	ReturnToPool();
}

void AProjectile::ApplyHit(AEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	Enemy->AddHP(-Damage);

	// 발사체(폭발 중심)에서 적을 향하는 수평 방향으로 넉백. 같은 위치면 진행 방향으로.
	FVector Dir = Enemy->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.0f;
	if (!Dir.Normalize())
	{
		Dir = GetActorForwardVector();
	}

	const FVector Force = Dir * KnockbackForce + FVector(0, 0, 100);
	Enemy->LaunchCharacter(Force, false, false);
}
