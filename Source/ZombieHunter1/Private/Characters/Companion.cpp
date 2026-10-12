// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Companion.h"
#include "AIController.h"

void ACompanion::UpdateIdle()
{
	if (!Leader)
	{
		Super::UpdateIdle();
		return;
	}

	// === 따라다님 ===
	State = EState::Following;
	if (!AICon)
	{
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), Leader->GetActorLocation());
	if (Dist > FollowDistance)
	{
		AICon->MoveToActor(Leader, FollowDistance);
	}
	else
	{
		AICon->StopMovement(); // 충분히 가까우면 멈춤(졸졸 따라가다 정지)
	}
}
