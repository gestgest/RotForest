// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/WarriorJob.h"
#include "Characters/MyPlayer.h"
#include "Characters/Enemy.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h" //근접 스윕 범위 디버그

UWarriorJob::UWarriorJob()
{
	JobType = EJobType::Warrior;

	Stats.MaxHP = 25;
	Stats.Damage = 1;
	Stats.Speed = 700;
	Stats.AttackInterval = 0.6;

	// 근접: 붙어서 휘둘러야 하므로 교전 사거리를 짧게(스윕 사거리 150보다 조금 안쪽).
	EngageRange = 130.0f;

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_WarriorJob)에서 직접 지정한다.
}

void UWarriorJob::OnAttackNotify(FName NotifyName)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// 전방으로 구체를 스윕해 범위 내 모든 적을 타격한다.
	TArray<FHitResult> hitResults;
	const FVector start = OwnerCharacter->GetActorLocation();
	const FVector end = start + (OwnerCharacter->GetActorForwardVector() * AttackRange);

	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHit = OwnerCharacter->GetWorld()->SweepMultiByChannel(
		hitResults,
		start,
		end,
		FQuat::Identity,
		ECC_Pawn, // TraceChannel
		Sphere,
		queryParams
	);

	bool bDidHit = false;
	for (const FHitResult& Result : hitResults)
	{
		AEnemy* hitEnemy = Cast<AEnemy>(Result.GetActor());
		if (hitEnemy)
		{
			UE_LOG(LogTemp, Log, TEXT("Hit Enemy!"));
			hitEnemy->AddHP(-GetDamage());
			const FVector force = OwnerCharacter->GetActorForwardVector() * KnockbackForce + FVector(0, 0, 100);
			hitEnemy->LaunchCharacter(force, false, false);
			bDidHit = true;
		}
	}

	// 디버그: 근접 스윕 범위(시작/끝 구체 + 경로)를 그린다. 적중=초록, 빗나감=빨강. 스윙마다 1초간.
	if (bDebugAttack)
	{
		const FColor Color = bDidHit ? FColor::Green : FColor::Red;
		const float Life = 1.0f;
		UWorld* World = OwnerCharacter->GetWorld();
		DrawDebugSphere(World, start, AttackRadius, 12, Color, false, Life, 0, 1.5f);
		DrawDebugSphere(World, end, AttackRadius, 12, Color, false, Life, 0, 1.5f);
		DrawDebugLine(World, start, end, Color, false, Life, 0, 1.5f);
	}

	// 적중했을 때만 공격 사운드 재생 (기존 동작 유지)
	if (bDidHit)
	{
		PlayAttackSound();
	}
}
