// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AutoAimComponent.generated.h"

class ACombatCharacter;
class UMaterialInteface;

// 자동공격 관련 클래스
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIEHUNTER1_API UAutoAimComponent : public UActorComponent
{
	GENERATED_BODY()


public:	
	// Sets default values for this component's properties
	UAutoAimComponent();
protected:
	virtual void BeginPlay() override;
public:	
	void UpdateAutoTarget(float Range);
	void ClearTarget();
	ACombatCharacter* GetTarget() const { return Target.Get(); }

private:
	void SetTarget(ACombatCharacter* NewTarget);



	// 타겟 메시에 덮을 림 라이트
	UPROPERTY(EditAnywhere, Category = "AutoAim")
	UMaterialInterface* HighlightMaterial = nullptr;

	// 자동조준 타겟
	TWeakObjectPtr<ACombatCharacter> Target;
	// 자기 자신
	TWeakObjectPtr<ACombatCharacter> OwnerCharacter; 
};
