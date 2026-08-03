// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CombatCharacter.h"
#include "Companion.generated.h"

class UJobComponent;
class AAIController;
class AEnemy;

/**
 * 아군 동료 AI 캐릭터.
 * 평소엔 리더(플레이어)를 졸졸 따라다니고(Following), 근처에 적이 나타나면
 * 그 적에게 다가가 직업(UJobComponent)으로 공격한다(Fighting).
 * 전투/이동 방식은 플레이어와 동일한 UJobComponent를 재활용한다 — 입력 대신 AI가 구동.
 *
 * 체력(HP)/데미지/죽음/공격몽타주 Notify 배선은 베이스(ACombatCharacter)가 제공한다.
 */
UCLASS()
class ZOMBIEHUNTER1_API ACompanion : public ACombatCharacter
{
	GENERATED_BODY()

public:
	ACompanion();

	/** 따라다닐 대상(보통 플레이어). 섭외 시 RecruitCompanion에서 설정. */
	UPROPERTY(BlueprintReadWrite, Category = "Companion")
	AActor* Leader = nullptr;

	// 직업(DefaultJobClass/CurrentJob)과 자동 공격 간격(AttackInterval)은 베이스(ACombatCharacter)에 있다.
	// 플레이어와 완전히 같은 배선이라 한곳으로 모았다.

	/** 적 탐지 반경(cm). 이 안에 적이 있으면 교전 상태로 전환. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float DetectRadius = 900.0f;

	/** 리더와 유지할 거리(cm). 이보다 멀어지면 따라간다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float FollowDistance = 250.0f;

	/** 죽은 뒤 시체가 사라지기까지의 시간(초). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	float CorpseLifeSpan = 3.0f;

	//////////////////////////////////////////////////////////////////////////
	// 경비 모드 (마을 경비병용) — 리더 대신 "자리"를 지킨다

	/** 켜면 리더 추종 대신 HomeLocation을 지킨다. 적과 교전 후 할 일이 없으면 자기 자리로 복귀.
	 *  (마을 경비병: 생성기가 스폰 직후 켜고 HomeLocation을 지정한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Guard")
	bool bGuardHome = false;

	/** 지킬 위치(월드). bGuardHome일 때만 사용. */
	UPROPERTY(BlueprintReadWrite, Category = "Companion|Guard")
	FVector HomeLocation = FVector::ZeroVector;

	/** 집에서 이 거리(cm) 안이면 "자리에 있음"으로 보고 정지. 벗어난 채 한가하면 복귀한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Guard")
	float HomeAcceptRadius = 150.0f;

	/** AI 의사결정(적 스캔·이동 명령) 갱신 간격(초). 매 프레임 전체 적 스캔을 막는다.
	 *  조준/공격 타이밍은 매 프레임 유지되므로 반응성엔 영향 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|AI")
	float DecisionInterval = 0.2f;

	// 디버그 토글은 베이스(ACombatCharacter::bDebugCombat)로 올림 — 플레이어/적과 공용.

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	/** 빙의된 AI 컨트롤러 */
	UPROPERTY()
	AAIController* AICon = nullptr;

	// 공격 타이머(TimeSinceLastAttack)는 베이스(ACombatCharacter::TickAttack)가 관리한다.

	/** 의사결정 누적 시간. BeginPlay에서 랜덤 오프셋으로 초기화해 동료들끼리 같은 프레임에 몰리지 않게 분산. */
	float TimeSinceDecision = 0.0f;

	/** 캐시된 교전 대상 — DecisionInterval마다 FindNearestEnemy로 갱신. */
	UPROPERTY()
	AEnemy* CurrentTarget = nullptr;

	/** 동료 상태 — 따라다님 / 교전 */
	enum class EState : uint8 { Following, Fighting };
	EState State = EState::Following;

	/** DecisionInterval마다 호출 — 타겟 재탐색 + 이동 명령(추격/정지/리더 추종). */
	void UpdateDecision();

	/** DetectRadius 안에서 가장 가까운 "살아있는" 적을 찾는다(없으면 nullptr). */
	AEnemy* FindNearestEnemy() const;

	/** 대상 액터를 바라보도록 yaw 회전(공격 방향을 적에게 맞춤). */
	void FaceActor(AActor* Target);

	/** 멈춰서 공격할 거리(cm) — 직업의 교전 사거리가 있으면 그것, 없으면 동료 AttackRange.
	 *  Tick(조준/공격)과 UpdateDecision(이동 명령)이 반드시 같은 값을 써야 한다.
	 *  둘이 어긋나면 사거리 밖에서 멈춰 영영 공격을 안 하는 버그가 생긴다. */
	float GetEngageRange() const;

	// 공격 Notify → 직업의 OnAttackNotify 전달은 베이스(ACombatCharacter)의 기본 구현이 처리한다.

	/** 죽음 전환 — AI/공격을 끊고 잠시 후 시체를 제거한다. */
	virtual void OnDeath() override;
};
