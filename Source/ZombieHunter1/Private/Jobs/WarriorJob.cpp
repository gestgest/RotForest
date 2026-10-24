// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/WarriorJob.h"
#include "Characters/MyPlayer.h"
#include "Characters/Enemy.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"

#include "Components/CapsuleComponent.h"
#include "CollisionShape.h"


#include "DrawDebugHelpers.h" //근접 스윕 범위 디버그



UWarriorJob::UWarriorJob()
{
	JobType = EJobType::Warrior;

	Stats.MaxHP = 25;
	Stats.Damage = 1;
	Stats.Speed = 700;
	Stats.AttackInterval = 0.6;

	// 근접: 붙어서 휘둘러야 하므로 교전 사거리를 짧게
	EngageRange = 160.0f;

	// 무기(WeaponMesh)는 BP 서브클래스(예: BP_WarriorJob)에서 직접 지정한다.
}


// 최종 공격 함수
void UWarriorJob::OnAttackNotify(FName NotifyName)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// 전방으로 부채꼴 범위의 적들을 타격
	UWorld* World = OwnerCharacter->GetWorld();
	const FVector Origin = OwnerCharacter->GetActorLocation();
	const FVector AimDir = OwnerCharacter->GetAttackAimDir().GetSafeNormal2D(); // 정규화된 공격 방향

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectParams;
	FCollisionQueryParams QueryParams;

	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	// 멀티로 맞기
	World->OverlapMultiByObjectType(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ObjectParams, // TraceChannel
		FCollisionShape::MakeSphere(AttackRange),
		QueryParams
	);

	bool bDidHit = false;
	TSet<AEnemy*> HitEnemies;
	for (const FOverlapResult& Result : OverlapResults)
	{
		bDidHit = bDidHit || HitEnemy(HitEnemies, Result, Origin, AimDir);
	}

	// 디버그: 근접 스윕 범위(시작/끝 구체 + 경로)를 그린다. 적중=초록, 빗나감=빨강. 스윙마다 1초간.
	if (bDebugAttack)
	{
		const FColor Color = bDidHit ? FColor::Green : FColor::Red;
		const float Life = 1.0f;
		const int32 Segments = 16;

		FVector Prev = Origin;
		for (int32 i = 0; i < Segments; i++)
		{
			const float Angle = -SwingAngle * 0.5f + SwingAngle * i / Segments;
			const FVector Point = Origin + AimDir.RotateAngleAxis(Angle, FVector::UpVector) * AttackRange;
			DrawDebugLine(World, Prev, Point, Color, false, Life, 0, 1.5f);
			Prev = Point;
		}
		DrawDebugLine(World, Prev, Origin, Color, false, Life, 0, 1.5f);
	}

	// 적중했을 때만 공격 사운드 재생 (기존 동작 유지)
	if (bDidHit)
	{
		PlayAttackSound();
	}
}

bool UWarriorJob::HitEnemy(TSet<AEnemy*> & HitEnemies, const FOverlapResult& Result, const FVector& Origin, const FVector & AimDir)
{
	AEnemy* NewHitEnemy = Cast<AEnemy>(Result.GetActor());
	if (!NewHitEnemy || HitEnemies.Contains(NewHitEnemy))
	{
		return false;
	}
	
	// cos 값 => -1 ~ 1
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SwingAngle * 0.5f));

	// 부채꼴 검사
	FVector ToEnemy = NewHitEnemy->GetActorLocation() - Origin;
	ToEnemy.Z = 0.0f;

	// 적 자체의 반지름
	const float EnemyRadius = NewHitEnemy->GetCapsuleComponent()->GetScaledCapsuleRadius();

	if (ToEnemy.Size() > EnemyRadius)
	{
		// 적과 공격 방향의 내적 : 전부 노멀벡터라 |A| |B| cos랑 같다.
		const float Dot = FVector::DotProduct(AimDir, ToEnemy.GetSafeNormal()); // 내적

		// 한마디로 cos값 => 노션에 공식 있다.
		if (Dot < CosHalfAngle)
		{
			return false;
		}
	}
	// else 몸이 겹칠만큼 붙어있다면 검사 생략 => 대체로 안 생긴다.


	HitEnemies.Add(NewHitEnemy);
	// 맞은 경우
	UE_LOG(LogTemp, Log, TEXT("Hit Enemy!"));

	NewHitEnemy->TakeHit(GetDamage(), OwnerCharacter);
	
	const FVector force = AimDir * KnockbackForce + FVector(0, 0, 100);
	NewHitEnemy->LaunchCharacter(force, false, false);
	return true;
}
