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

	enum class EState : uint8 { Following, Fighting };
	EState State = EState::Following;

	// 빙의된 AI 컨트롤러
	UPROPERTY()
	AAIController* AICon = nullptr;


	// 캐시된 교전 대상 — DecisionInterval마다 FindNearestEnemy로 갱신.
	UPROPERTY()
	AEnemy* CurrentTarget = nullptr;


	// AI 의사결정(적 스캔·이동 명령) 갱신 간격(초). 매 프레임 전체 적 스캔을 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|AI")
	float DecisionInterval = 0.2f;

	// 죽은 뒤 시체가 사라지기까지의 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally")
	float CorpseLifeSpan = 3.0f;

	// 적 탐지 반경(cm). 이 안에 적이 있으면 교전 상태로 전환.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally")
	float DetectRadius = 900.0f;


	// 판단 => DecisionInterval마다 호출
	void UpdateDecision();

	// 싸울 적이 없는 경우
	virtual void UpdateIdle();


	// DetectRadius 안에서 가장 가까운 "살아있는" 적을 찾는다(없으면 nullptr).
	AEnemy* FindNearestEnemy() const;

	// 의사결정 누적 시간. BeginPlay에서 랜덤 오프셋으로 초기화해 동료들끼리 같은 프레임에 몰리지 않게 분산.
	float TimeSinceDecision = 0.0f;


	// 대상 액터를 바라보도록 yaw 회전(공격 방향을 적에게 맞춤).
	void FaceActor(AActor* Target);
	
public:

	// 멈춰서 공격할 거리(cm) => 직업 사거리
	float GetEngageRange() const;
};
