// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/MageJob.h"
#include "Projectiles/Projectile.h"

UMageJob::UMageJob()
{
	JobType = EJobType::Mage;


	// 기본 발사체: 순수 AProjectile(메시 없는 구체). BP_Fireball로 바꾸면 이펙트가 보인다.
	ProjectileClass = AProjectile::StaticClass();
}

void UMageJob::Attack()
{
	// 공격 몽타주가 있으면 재생(시전 모션). 발사체는 곧바로 발사.
	Super::Attack();

	CastSpell();
}

void UMageJob::CastSpell()
{
	// 베이스 공용 헬퍼로 발사체를 스폰한 뒤, 폭발 반경만 마법사 값으로 설정한다.
	AProjectile* Spell = SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight);
	if (Spell)
	{
		Spell->ExplosionRadius = ExplosionRadius;
	}
}
