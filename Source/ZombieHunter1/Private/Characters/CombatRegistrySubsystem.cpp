// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/CombatRegistrySubsystem.h"


//ACombatCharacter가 BeginPlay/EndPlay에서 스스로 등록/해제한다.
void UCombatRegistrySubsystem::Deinitialize()
{
	Characters.Empty();
	Super::Deinitialize();
}

bool UCombatRegistrySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return EWorldType::Game == WorldType || EWorldType::PIE == WorldType;
}

void UCombatRegistrySubsystem::Register(ACombatCharacter* Character)
{
    if (!IsValid(Character))
    {
        return;
    }

    // AddUnique — 중복 등록 방어. BeginPlay가 두 번 불릴 일은 없지만,
    // 있어도 조용히 무시되는 편이 목록에 같은 놈이 두 번 들어가는 것보다 낫다.
    Characters.AddUnique(Character);
}

void UCombatRegistrySubsystem::Unregister(ACombatCharacter* Character)
{
    if (!Character)
    {
        return;
    }

    // RemoveSingleSwap — 순서를 지킬 필요가 없으니 마지막 원소를 끌어와 덮는다.
    // Remove()의 O(n) 밀어내기보다 빠르다.
    Characters.RemoveSingleSwap(Character);
}


ACombatCharacter* UCombatRegistrySubsystem::FindNearestOfEnemy(const FVector& From, ETeam Team, float MaxDistance) const
{
    ACombatCharacter* Best = nullptr;

    // 제곱 거리로 비교한다 — sqrt를 한 번도 안 부르려고.
    // MaxDistance <= 0이면 "거리 제한 없음"으로 취급.
    float BestDistSq = (MaxDistance > 0.f)
        ? MaxDistance * MaxDistance
        : TNumericLimits<float>::Max();


    //탐색
    for (const TWeakObjectPtr<ACombatCharacter>& Weak : Characters)
    {
        ACombatCharacter* Candidate = Weak.Get();

        // IsValid  : 파괴됐거나 GC 대기 중
        // IsHidden : 풀에서 대기 중인 적 (EnterPoolDormancy로 숨겨진 상태)
        // GetIsDead: 죽었지만 아직 시체가 남아있는 상태
        if (!IsValid(Candidate) || Candidate->IsHidden() || Candidate->GetIsDead())
        {
            continue;
        }

        //찾는 팀이 아니라면 
        if (Candidate->TeamType != Team)
        {
            continue;
        }

        // 2D 거리 — 높이 차이는 무시. 지금 게임이 평면이라 Z를 넣으면
        // 언덕 위 적이 부당하게 멀게 계산된다.
        const float DistSq = FVector::DistSquared2D(From, Candidate->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }
    return Best;
}
