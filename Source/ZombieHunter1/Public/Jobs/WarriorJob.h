// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobComponent.h"
#include "WarriorJob.generated.h"

/**
 * 전사(근접) 직업.
 * 전방으로 구체 스윕을 날려 범위 내 적을 동시에 타격하고 넉백시킨다.
 * (기존 AMyPlayer::hit() 로직을 그대로 옮긴 것)
 */
UCLASS()
class ZOMBIEHUNTER1_API UWarriorJob : public UJobComponent
{
	GENERATED_BODY()

public:
	UWarriorJob();


	virtual void OnAttackNotify(FName NotifyName) override;

protected:
	// 부채꼴 반지름 (cm)
	UPROPERTY(EditAnywhere, Category = "Job|Combat")
	float AttackRange = 200.0f;

	// 각도
	UPROPERTY(EditAnywhere, Category = "Job|Combat", meta = (ClampMin = "0", ClampMax = "360"))
	float SwingAngle = 120.0f;

	/** 적중 시 적을 밀어내는 힘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Combat")
	float KnockbackForce = 500.0f;

	virtual bool HitEnemy(TSet<AEnemy*>& HitEnemies, const FOverlapResult& Result, const FVector & Origin, const FVector& AimDir) override;
};
