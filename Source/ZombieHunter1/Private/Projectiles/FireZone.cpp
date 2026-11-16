// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/FireZone.h"
#include "Characters/Enemy.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"

#include "NiagaraComponent.h"


// Sets default values
AFireZone::AFireZone()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Effect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Effect"));
	Effect->SetupAttachment(RootComponent);
}

void AFireZone::Init(int32 NewDamage, ACombatCharacter* NewOwner)
{
	Damage = NewDamage;
	OwnerCharacter = NewOwner;

}

// Called when the game starts or when spawned
void AFireZone::BeginPlay()
{
	Super::BeginPlay();
	
	// Duration 시간이 지나면 없어지게 해라
	SetLifeSpan(Duration);

	GetWorldTimerManager().SetTimer(DamageTimer, this, &AFireZone::DealDamage, TickInterval, true, 0.1f);
}

void AFireZone::DealDamage()
{
	UWorld* World = GetWorld();
	// 이거 DealDamage가 Begin함수나 Tick에서 생성된다면 null 체크 안해도 됨

	const FVector Center = GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius));

	TSet<AEnemy*> HitEnemies;
	for (FOverlapResult& Result : Overlaps)
	{
		AEnemy* Enemy = Cast<AEnemy>(Result.GetActor());
		// enemy가 아니거나 죽었거나 중첩된 enemy인 경우
		if (!Enemy || Enemy->GetIsDead() || Enemy->IsHidden() || HitEnemies.Contains(Enemy))
		{
			continue;
		}
		HitEnemies.Add(Enemy);
		Enemy->TakeHit(Damage, OwnerCharacter.Get());
	}

	if (bDrawDebug)
	{
		DrawDebugCircle(World, Center, Radius, 32,
			FColor::Orange, false, TickInterval,
			0, 3.0f,FVector(1, 0, 0), FVector(0, 1, 0), false); // 0이면 다른 물체에 가려짐, 굵기, 높이 축, 평면 축, 축 화살표 그릴지
	}
}
