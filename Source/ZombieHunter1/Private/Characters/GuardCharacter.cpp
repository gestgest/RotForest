// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/GuardCharacter.h"
#include "AIController.h"

void AGuardCharacter::UpdateIdle()
{
	// 자기 자리(HomeLocation)로 복귀 — 교전으로 자리를 벗어나도 다시 돌아온다.
	State = EState::Following;

	if (!AICon)
	{
		return;
	}

	const float Dist = FVector::Dist2D(GetActorLocation(), HomeLocation);
	if (Dist > HomeAcceptRadius)
	{
		AICon->MoveToLocation(HomeLocation, HomeAcceptRadius * 0.5f);
	}
	else 
	{
		AICon->StopMovement(); // 자리에 도착 — 대기
	}
}
