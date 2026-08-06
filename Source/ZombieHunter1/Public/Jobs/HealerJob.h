// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobComponent.h"
#include "HealerJob.generated.h"

class ACombatCharacter;

/**
 * 힐러(지원) 직업.
 * "공격"이 곧 힐 시전이다. 조준하면 전방으로 힐을 날려, 닿은 아군(자신/동료)을 회복한다.
 * 적에게는 아무 효과가 없다. (동료 시스템이 들어오면 전방의 동료까지 회복 — 로스트아크식 서포터)
 */
UCLASS()
class ZOMBIEHUNTER1_API UHealerJob : public UJobComponent
{
	GENERATED_BODY()

public:
	UHealerJob();

	virtual void OnAttackNotify(FName NotifyName) override ;
	void Heal();

	// 한 번에 회복하는 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	int32 HealAmount = 1;

	// 회복 상한은 대상 캐릭터 자신의 MaxHP를 쓴다(힐러가 따로 들고 있지 않는다).

	/** 전방 힐 사거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	float HealRange = 400.0f;

	/** 힐 범위 반경(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job|Healer")
	float HealRadius = 500.0f;

	/** 공격 대신 힐을 시전한다 */
	virtual void Attack() override;

protected:
	virtual void TickJob(float DeltaTime) override;

	/** 대상 아군(플레이어/동료)을 MaxHP까지 회복 */
	void HealCharacter(ACombatCharacter* Target);
};
