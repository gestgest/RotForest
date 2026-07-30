// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/CombatAnimInstance.h"
#include "Characters/CombatCharacter.h"
#include "Characters/MyPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCombatAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwner();
}

void UCombatAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 프리뷰/스폰 직후엔 폰이 아직 없을 수 있어 매 갱신마다 재시도한다.
	if (!OwnerCombatCharacter && !CacheOwner())
	{
		return;
	}

	// 이동 상태
	GroundSpeed = OwnerCombatCharacter->GetVelocity().Size2D();
	if (const UCharacterMovementComponent* Move = OwnerCombatCharacter->GetCharacterMovement())
	{
		bIsFalling = Move->IsFalling();
	}

	// 전투 상태 (캐릭터 값 미러)
	bArmed = OwnerCombatCharacter->bArmed;
	bIsDead = OwnerCombatCharacter->GetIsDead();

	// 하체 yaw 오프셋 — 플레이어만 계산한다(Tick의 UpdateLegYawOffset). 그 외엔 0.
	LegYawOffset = OwnerPlayer ? OwnerPlayer->GetLegYawOffset() : 0.0f;
}

bool UCombatAnimInstance::CacheOwner()
{
	OwnerCombatCharacter = Cast<ACombatCharacter>(TryGetPawnOwner());
	OwnerPlayer = Cast<AMyPlayer>(OwnerCombatCharacter);
	return OwnerCombatCharacter != nullptr;
}
