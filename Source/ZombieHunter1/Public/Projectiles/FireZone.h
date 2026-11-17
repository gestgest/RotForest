// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireZone.generated.h"

class ACombatCharacter;
class UNiagaraComponent;

UCLASS()
class ZOMBIEHUNTER1_API AFireZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AFireZone();

	void Init(int NewDamage, ACombatCharacter* NewOwner);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void DealDamage();


	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* Effect;


	UPROPERTY(EditAnywhere, Category = "FireZone")
	float Radius = 300.f;

	UPROPERTY(EditAnywhere, Category = "FireZone")
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, Category = "FireZone")
	float TickInterval = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "FireZone")
	bool bDrawDebug = false;


private:
	FTimerHandle DamageTimer;
	TWeakObjectPtr<ACombatCharacter> OwnerCharacter;

	float Damage;

};
