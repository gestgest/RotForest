// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobComponent.h"
#include "MageJob.generated.h"

class AProjectile;

/**
 * 마법사(원거리/범위) 직업.
 * 공격 시 전방으로 마법 발사체를 쏘고, 적중 지점에서 범위 폭발(AOE) 피해를 준다.
 * 궁수보다 느리지만 한 발로 여러 적을 동시에 타격한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UMageJob : public UJobComponent
{
	GENERATED_BODY()

public:
	UMageJob();
	
	virtual void OnAttackNotify(FName NotifyName) override;


	/** 발사할 마법 발사체 클래스. BP에서 이펙트 달린 BP_Fireball을 지정하면 보임. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Mage")
	TSubclassOf<AProjectile> ProjectileClass;

	/** 마법 발사체 속도(cm/s) — 궁수 화살보다 느리게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Mage")
	float ProjectileSpeed = 1500.0f;

	/** 적중 지점 폭발 반경(cm). 이 안의 모든 적이 피해를 받음. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Mage")
	float ExplosionRadius = 250.0f;

	/** 캐릭터 앞쪽으로 발사체를 스폰하는 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Mage")
	float MuzzleOffset = 80.0f;

	/** 발사체 스폰 높이 보정(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Mage")
	float MuzzleHeight = 50.0f;

	virtual void Attack() override;

protected:
	/** 전방으로 폭발 마법 발사체 1발 발사 */
	void CastSpell();
};
