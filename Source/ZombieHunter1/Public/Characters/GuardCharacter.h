// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/AllyCharacter.h"
#include "GuardCharacter.generated.h"

UCLASS()
class ZOMBIEHUNTER1_API AGuardCharacter : public AAllyCharacter
{
	GENERATED_BODY()

protected:
	virtual void UpdateIdle() override;


	// 지킬 위치(월드).
	UPROPERTY(BlueprintReadWrite, Category = "uard")
	FVector HomeLocation = FVector::ZeroVector;

	// 집에서 이 거리(cm) 안이면 "자리에 있음"으로 보고 정지. 벗어난 채 한가하면 복귀한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float HomeAcceptRadius = 150.0f;

public:
	void SetHomeLocation(const FVector& NewHomeLocation) { HomeLocation = NewHomeLocation; }
};
