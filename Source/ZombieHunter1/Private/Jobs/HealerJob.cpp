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
	float BestRatio = 1.0f;

	// todo
	for (ACombatCharacter* Ally : Allies)
	{
		if (Ally->IsHidden() || Ally->GetIsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(Origin, Ally->GetActorLocation());

		// 힐 범위 넘어서면
		if (DistSq > HealRadiusSq)
		{
			continue;
		}

		// 죽거나 풀피라면
		if (Ally->GetHP() <= 0 || Ally->GetHP() >= Ally->GetMaxHP())
		{
			continue;
		}

		float Ratio = static_cast<float>(Ally->GetHP()) / Ally->GetMaxHP();

		// 플레이어냐
		if (Ally->IsPlayerControlled())
		{
			Ratio -= PlayerPriorityBonus; // 플레이어에게 더 가중치
		}

		if (Ratio < BestRatio)
		{
			Best = Ally;
			BestRatio = Ratio;
		}

	}
	return Best;
}


// 힐하는 공식
void UHealerJob::HealCharacter(ACombatCharacter* Target)
{
	if (!Target || Target->GetIsDead() || Target->GetHP() >= Target->GetMaxHP())
	{
		return;
	}
	const int32 NewHP = FMath::Min(Target->GetHP() + GetDamage(), Target->GetMaxHP());
	Target->SetHP(NewHP);

	if (HealEffect)
	{
		const FVector Feet = Target->GetActorLocation() 
			- FVector(0.0f, 0.0f, Target->GetSimpleCollisionHalfHeight());

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(OwnerCharacter, HealEffect, Feet);
	}
}
