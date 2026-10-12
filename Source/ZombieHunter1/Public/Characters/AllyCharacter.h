// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CombatCharacter.h"
#include "AllyCharacter.generated.h"

class AAIController;
class AEnemy;

// 캐릭터
// ㄴ Companion 
// ㄴ Guard

UCLASS(Abstract)
class ZOMBIEHUNTER1_API AAllyCharacter : public ACombatCharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAllyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	// 죽음 전환 — AI/공격을 끊고 잠시 후 시체를 제거한다. 
	virtual void OnDeath() override;

	enum class EState : uint8 { Following, Fighting, Returning };
	EState State = EState::Following;

	// 빙의된 AI 컨트롤러
	UPROPERTY()
	AAIController* AICon = nullptr;


	// 캐시된 교전 대상 — DecisionInterval마다 FindNearestEnemy로 갱신.
	UPROPERTY()
	AEnemy* CurrentTargetEnemy = nullptr;


	// AI 의사결정(적 스캔·이동 명령) 갱신 간격(초). 매 프레임 전체 적 스캔을 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|AI")
	float DecisionInterval = 0.2f;

	// 죽은 뒤 시체가 사라지기까지의 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally")
	float CorpseLifeSpan = 3.0f;

	// 적 탐지 반경(cm). 이 안에 적이 있으면 교전 상태로 전환.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally")
	float DetectRadius = 900.0f;

	// [동료 복귀 변수]
	UPROPERTY(EditAnywhere, Category = "Ally|Return")
	float ResumeTrackingDistance = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Ally|Return")
	float SoftTrackingDistance = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Ally|Return")
	float HardTrackingDistance = 1800.0f;

	// 판단 => DecisionInterval마다 호출
	void UpdateDecision();

	// 싸울 적, 복귀할 경우
	virtual void MoveToAnchor();

	// 복귀 기준점 => 동료: 리더, 경비병: 자리. 없으면 false
	virtual bool GetAnchorLocation(FVector& OutLocation) const { return false; }
	virtual bool TryReturnTeleport(float AnchorDist) { return false; }  // 일단 false

	// DetectRadius 안에서 가장 가까운 적을 찾는다
	AEnemy* FindNearestEnemy(const FVector& Origin) const;

	// 의사결정 누적 시간. BeginPlay에서 랜덤 오프셋으로 초기화해 동료들끼리 같은 프레임에 몰리지 않게 분산.
	float TimeSinceDecision = 0.0f;


	// 대상 액터를 바라보도록 yaw 회전
	void FaceActor(AActor* Target);
	
public:

	// 멈춰서 공격할 거리(cm) => 직업 사거리
	float GetEngageRange() const;
};
