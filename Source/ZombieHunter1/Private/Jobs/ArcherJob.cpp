// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/ArcherJob.h"
#include "Projectiles/Projectile.h"

UArcherJob::UArcherJob()
{
	JobType = EJobType::Archer;

	Stats.MaxHP = 10;
	Stats.Damage = 1;
	Stats.Speed = 900;
	Stats.AttackInterval = 0.4;

	// 기본 발사체: 순수 AProjectile(메시 없는 충돌 구체). BP_Arrow로 바꾸면 화살이 보인다.
	ProjectileClass = AProjectile::StaticClass();

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_ArcherJob)에서 직접 지정한다.
}

void UArcherJob::Attack()
{
	// 공격 몽타주가 있으면 재생(시각 효과). 화살은 곧바로 발사한다.
	Super::Attack();

	FireArrow();
}
 
void UArcherJob::FireArrow()
{
	// 화살은 단일 대상 발사체 (폭발 없음). 스폰은 베이스 공용 헬퍼가 담당.
	SpawnProjectileForward(ProjectileClass, ProjectileSpeed, MuzzleOffset, MuzzleHeight);
}
