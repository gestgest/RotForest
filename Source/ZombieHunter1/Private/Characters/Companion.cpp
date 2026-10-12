// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/Companion.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"

void ACompanion::MoveToAnchor()
{
	if (!Leader)
	{
		Super::MoveToAnchor();
		return;
	}

	// === 따라다님 ===
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

// 순간이동 하는 함수
bool ACompanion::TryReturnTeleport(float AnchorDist)
{
	// 리더가 없거나 텔포할 거리정도는 아니라면
	if (!Leader || AnchorDist <= TeleportDistance)
	{
		return false;
	}

	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav)
	{
		return false;
	}

	const FVector LeaderLoc = Leader->GetActorLocation();
	const FVector Behind = LeaderLoc - Leader->GetActorForwardVector() * TeleportRadius;

	FNavLocation NavLoc;
	// 순간이동할 땅 찾기 => 리더 뒤, 없으면 리더 주변. 둘 다 없으면 포기
	if (!Nav->ProjectPointToNavigation(Behind, NavLoc, FVector(200.0f, 200.0f, 500.0f)) &&
		!Nav->GetRandomReachablePointInRadius(LeaderLoc, TeleportRadius, NavLoc))
	{
		return false;
	}

	FVector Dest = NavLoc.Location;
	Dest.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// 그 회전상태로 텔포
	if (!TeleportTo(Dest, GetActorRotation()))
	{
		return false;
	}

	if (AICon)
	{
		AICon->StopMovement();
	}
	return true;
}


bool ACompanion::GetAnchorLocation(FVector& OutLocation) const
{
	if (!Leader)
	{
		return false;
	}
	OutLocation = Leader->GetActorLocation();
	return true;

}
