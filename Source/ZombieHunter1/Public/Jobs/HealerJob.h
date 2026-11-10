// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobComponent.h"
#include "HealerJob.generated.h"

class ACombatCharacter;
class AEnemy;
class UNiagaraSystem;

UCLASS()
class ZOMBIEHUNTER1_API UHealerJob : public UJobComponent
{
	GENERATED_BODY()

public:
	UHealerJob();

	virtual void OnAttackNotify(FName NotifyName) override;
	virtual void OnProjectileHit(AEnemy* HitEnemy) override;

protected:
	// 대상 아군(플레이어/동료)을 MaxHP까지 회복 
	void HealCharacter(ACombatCharacter* Target);
	
	ACombatCharacter* FindHealTarget() const;

	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Job|Healer", meta = (ClampMin = "0", ClampMax = "1"))
	float PlayerPriorityBonus = 0.1f;

	UPROPERTY(EditAnywhere, Category="Job|Healer")
	UNiagaraSystem* HealEffect = nullptr;


	// 힐 이펙트 지속시간
	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	float HealEffectDuration = 0.5f;


	// 힐 범위 반경(cm)
	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	float HealRadius = 1000.0f;

	// [투사체]
	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	float ProjectileSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	float MuzzleOffset = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Job|Healer")
	float MuzzleHeight = 50.0f;
};
