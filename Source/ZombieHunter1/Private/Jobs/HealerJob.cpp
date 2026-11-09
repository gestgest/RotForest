// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/HealerJob.h"

#include "Characters/CombatCharacter.h"
#include "Characters/Companion.h"
#include "Characters/Enemy.h"
#include "Characters/MyPlayer.h"
#include "Characters/CombatRegistrySubsystem.h"

#include "Projectiles/Projectile.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h" 

#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h" // 힐 범위 디버그


// 힐러는 적을 때리지 않는다. "공격"은 힐 시전이고, 닿은 아군을 회복한다.
UHealerJob::UHealerJob()
{
	JobType = EJobType::Healer;

	Stats.MaxHP = 80;
	Stats.Damage = 10;
	Stats.Speed = 600;
	Stats.AttackInterval = 2.0f;

	ProjectileClass = AProjectile::StaticClass();
}


void UHealerJob::OnAttackNotify(FName NotifyName)
{
	if (NotifyName != "Attack")
	{
		return;
	}

	if (SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight))
	{
		PlayAttackSound();
	}
}

void UHealerJob::OnProjectileHit(AEnemy* HitEnemy)
{
	if (!OwnerCharacter || OwnerCharacter->GetIsDead())
	{
		return;
	}

	ACombatCharacter* HealTarget = FindHealTarget();
	HealCharacter(HealTarget);

	if (bDebugAttack && HealTarget && IsValid(HitEnemy))
	{
		DrawDebugLine(OwnerCharacter->GetWorld(), HitEnemy->GetActorLocation(),
			HealTarget->GetActorLocation(), FColor::Green, false, 0.5f, 0, 2.0f);
	}
}

ACombatCharacter* UHealerJob::FindHealTarget() const
{
	UWorld* World = OwnerCharacter->GetWorld();
	UCombatRegistrySubsystem* Registry = World
		? World->GetSubsystem<UCombatRegistrySubsystem>() : nullptr;
	
	if (!Registry)
	{
		return nullptr;
	}

	TArray<ACombatCharacter*>Allies;
	Registry->GetAllOfTeam(OwnerCharacter->TeamType, Allies);

	const FVector Origin = OwnerCharacter->GetActorLocation();
	const float HealRadiusSq = HealRadius * HealRadius;

	ACombatCharacter* Best = nullptr;
	// 최고값 설정
	float BestRatio = TNumericLimits<float>::Max();
	float BestDistSq = TNumericLimits<float>::Max();

	// todo
	for (ACombatCharacter* Allie : Allies)
	{

	}

	return Best;
}


// 힐하는 공식
void UHealerJob::HealCharacter(ACombatCharacter* Target)
{
	if (!Target)
	{
		return;
	}

	// 피를 채워야 하는 경우 + 살아있는 경우
	if (Target->GetHP() < Target->GetMaxHP() && !Target->GetIsDead())
	{
		const int32 NewHP = FMath::Min(Target->GetHP() + GetDamage(), Target->GetMaxHP());
		Target->SetHP(NewHP);
	}
	// else 풀피임
}
