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

    /** 팀이 일치하는 등록 캐릭터를 전부 모은다(Out은 덮어쓴다).
     *  죽음/숨김은 거르지 않는다 — 그 판단은 호출자가 한다.
     *  풀에 없는 적(생성기가 스폰한 보스 등)까지 한 번에 훑어야 할 때 쓴다. */
    void GetAllOfTeam(ETeam Team, TArray<ACombatCharacter*>& Out) const;

    void Register(ACombatCharacter* Character);
    void Unregister(ACombatCharacter* Character);

    int32 GetRegisteredCount() const { return Characters.Num(); }

private:
    /** TWeakObjectPtr인 이유: 액터가 파괴돼도 이 배열이 그걸 살려두지 않는다.
     *  UPROPERTY()로 강참조하면 죽은 적이 GC되지 않고 계속 남는다. */
    TArray<TWeakObjectPtr<ACombatCharacter>> Characters;
};
