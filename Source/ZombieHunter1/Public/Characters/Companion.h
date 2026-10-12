// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/AllyCharacter.h"
#include "Companion.generated.h"

// 아군 AI 동료
UCLASS()
class ZOMBIEHUNTER1_API ACompanion : public AAllyCharacter
{
	GENERATED_BODY()

protected:
	virtual void UpdateIdle() override;

	// [변수]
	// 따라다닐 대상(보통 플레이어). 섭외 시 RecruitCompanion에서 설정.
	UPROPERTY(BlueprintReadWrite, Category = "Companion")
	AActor* Leader = nullptr;

	// 리더와 유지할 거리(cm). 이보다 멀어지면 따라간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float FollowDistance = 250.0f;

	// [동료 복귀 변수]
	UPROPERTY(EditAnywhere, Category="Companion|Return")
	float SoftTrackingDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Return")
	float HardTrackingDistance = 1800.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Return")
	float TeleportDistance = 3000.0f;

public:
	void SetLeader(AActor* NewLeader) { Leader = NewLeader; }
};
