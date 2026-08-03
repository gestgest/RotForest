// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Characters/CombatCharacter.h" 
#include "CombatRegistrySubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIEHUNTER1_API UCombatRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//UCombatRegistrySubsystem(); => 여기선 Initialize를 호출해야 한다.
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    ACombatCharacter* FindNearestOfEnemy(const FVector& From, ETeam Team, float MaxDistance) const;

    void Register(ACombatCharacter* Character);
    void Unregister(ACombatCharacter* Character);

    int32 GetRegisteredCount() const { return Characters.Num(); }

private:
    /** TWeakObjectPtr인 이유: 액터가 파괴돼도 이 배열이 그걸 살려두지 않는다.
     *  UPROPERTY()로 강참조하면 죽은 적이 GC되지 않고 계속 남는다. */
    TArray<TWeakObjectPtr<ACombatCharacter>> Characters;
};
