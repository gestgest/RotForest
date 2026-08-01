// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobComponent.h"
#include "Characters/CombatCharacter.h"          
#include "Components/SkeletalMeshComponent.h"    
#include "Animation/AnimSingleNodeInstance.h"
#include "ArcherJob.generated.h"

class AProjectile;

/**
 * 궁수(원거리) 직업.
 * 공격 시 전방으로 화살(AProjectile)을 발사한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UArcherJob : public UJobComponent
{
	GENERATED_BODY()

public:
	UArcherJob();

	 void OnAttackMontageEnded() override;

	/** 발사할 화살 액터 클래스. BP에서 메시 달린 BP_Arrow를 지정하면 보임. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Archer")
	TSubclassOf<AProjectile> ProjectileClass;

	/** 화살 발사 속도(cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Archer")
	float ProjectileSpeed = 2000.0f;

	/** 캐릭터 앞쪽으로 화살을 스폰하는 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Archer")
	float MuzzleOffset = 80.0f;

	/** 화살 스폰 높이 보정(cm) — 발 밑이 아닌 가슴 높이에서 발사 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Archer")
	float MuzzleHeight = 50.0f;

	virtual void Attack() override;

protected:
	/** 전방으로 화살 1발 발사 */
	void FireArrow();
	void PlayBowDraw();



	// UArcherJob — 미세조정용. 1.0보다 크면 살짝 먼저 다 당겨진다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job|Archer")
	float BowDrawRateScale = 1.05f;
private:
	void ResetBow();

	// 활 시위 당김 애니(활 스켈레톤용). 무기 컴포넌트에서 재생된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Job|Archer")
	UAnimSequence* BowDrawAnim = nullptr;

};
