// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CombatAnimInstance.generated.h"

class ACombatCharacter;
class AMyPlayer;

/**
 * 전투 캐릭터 공용 AnimInstance 베이스 — 플레이어/동료/적 AnimBP가 부모로 쓴다.
 *
 * 매 프레임 NativeUpdateAnimation(C++)이 소유 캐릭터에서 상태를 읽어 아래 변수들을 채운다.
 * → AnimBP 이벤트 그래프에서 캐스트/계산 노드가 전부 필요 없어지고, 애님 그래프는
 *   변수 읽기만 남아 FastPath(워커 스레드 업데이트) 최적화를 탈 수 있다.
 *
 * 에디터 적용법: 각 AnimBP의 Class Settings → Parent Class를 CombatAnimInstance로 변경 후,
 * 기존 BP 변수(GroundSpeed 등 이름이 겹치는 것)를 지우고 상속된 변수를 읽으면 된다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UCombatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** 수평 이동 속도(cm/s). idle/walk/run 블렌드스페이스 입력용. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	float GroundSpeed = 0.0f;

	/** 공중에 떠 있는지(점프/낙하). */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	bool bIsFalling = false;

	/** 무장 상태 — 캐릭터의 bArmed 미러. 무장/비무장 블렌드스페이스 선택용. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	bool bArmed = true;

	/** 죽었는지 — 캐릭터의 IsDead 미러. 죽음 상태 전이용. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	bool bIsDead = false;

	/** 하체를 이동 방향으로 돌리는 yaw 오프셋(도). 플레이어만 계산하며 그 외 캐릭터는 항상 0.
	 *  AnimBP가 pelvis(+)·spine_01(-) 회전에 쓴다. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	float LegYawOffset = 0.0f;

	// 공격 몽타주 동안 몸 전체를 돌리는 yaw(도). AnimBP가 pelvis 회전에 더한다
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	float AttackYawOffset = 0.0f;

	// AttackYawOffset이 따라가는 속도. 기본값: 20
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackYawInterpSpeed = 20.0f;


	/** 소유 캐릭터(캐시). AnimBP에서 추가 정보가 필요할 때 캐스트 없이 쓸 수 있다. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
	ACombatCharacter* OwnerCombatCharacter = nullptr;

private:
	/** 소유자가 플레이어일 때만 유효(LegYawOffset 읽기 용). */
	UPROPERTY(Transient)
	AMyPlayer* OwnerPlayer = nullptr;

	/** TryGetPawnOwner를 캐스트해 OwnerCombatCharacter/OwnerPlayer를 캐시. 실패하면 false. */
	bool CacheOwner();
};
