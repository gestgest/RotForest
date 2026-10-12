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
	virtual void MoveToAnchor() override;
	virtual bool TryReturnTeleport(float AnchorDist) override;

	// [변수]
	// 따라다닐 대상(보통 플레이어). 섭외 시 RecruitCompanion에서 설정.
	UPROPERTY(BlueprintReadWrite, Category = "Companion")
	AActor* Leader = nullptr;

	// 리더와 유지할 거리(cm). 이보다 멀어지면 따라간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float FollowDistance = 250.0f;


	UPROPERTY(EditAnywhere, Category = "Companion|Return")
	float TeleportDistance = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Companion|Return")
	float TeleportRadius = 400.0f;
	virtual bool GetAnchorLocation(FVector& OutLocation) const override;
public:
	void SetLeader(AActor* NewLeader) { Leader = NewLeader; }
};
