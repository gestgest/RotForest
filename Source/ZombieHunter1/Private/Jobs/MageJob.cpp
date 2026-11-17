// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/MageJob.h"
#include "Jobs/SkillDataAsset.h"
#include "Characters/Enemy.h"

#include "Projectiles/FireZone.h"
#include "Projectiles/Projectile.h"

UMageJob::UMageJob()
{
	JobType = EJobType::Mage;

	Stats.MaxHP = 8;
	Stats.Damage = 4;
	Stats.Speed = 600;
	Stats.AttackInterval = 1.2f;
	// 기본 발사체: 순수 AProjectile(메시 없는 구체). BP_Fireball로 바꾸면 이펙트가 보인다.
	ProjectileClass = AProjectile::StaticClass();

	EngageRange = 800.0f;
	FireZoneClass = AFireZone::StaticClass();
}

void UMageJob::Attack()
{
	// 공격 몽타주가 있으면 재생(시전 모션). 발사체는 곧바로 발사.
	Super::Attack();
}
void UMageJob::OnAttackNotify(FName NotifyName)
{
	if (NotifyName == "Attack")
		CastSpell();
}


void UMageJob::UseSkill()
{
	if (!FireZoneClass)
	{
		return;
	}

	FVector Loc;
	ACombatCharacter* Target = OwnerCharacter->GetSkillTarget();

	// 적이 있는 경우
	if (Target && !Target->GetIsDead())
	{
		Loc = Target->GetActorLocation();
		Loc.Z -= Target->GetSimpleCollisionHalfHeight(); // 밑바닥
	}
	else
	{
		Loc = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * DefaultFireZoneForwardDistance;
		Loc.Z -= OwnerCharacter->GetSimpleCollisionHalfHeight(); // 밑바닥
	}

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AFireZone* Zone = GetWorld()->SpawnActor<AFireZone>(FireZoneClass, Loc, FRotator::ZeroRotator, Params))
	{
		// 화염 장판 init
		Zone->Init(FMath::RoundToInt(GetDamage() * SkillData->DamageRatio), OwnerCharacter);
	}
}

bool UMageJob::WantsToUseSkill(AEnemy* Target)
{
	if (!Target || Target->GetIsDead())
	{
		return false;
	}

	// 거리가 너무 길면
	if (FVector::Dist(Target->GetActorLocation(), OwnerCharacter->GetActorLocation()) > EngageRange)
	{
		return false;
	}
	
	return RollAISkill(OwnerCharacter->GetAttackInterval());
}

// 화염구 소환
void UMageJob::CastSpell()
{
	// 베이스 공용 헬퍼로 발사체를 스폰한 뒤, 폭발 반경만 마법사 값으로 설정한다.
	AProjectile* Spell = SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight);
	if (Spell)
	{
		Spell->ExplosionRadius = ExplosionRadius;
		PlayAttackSound();
	}
}

