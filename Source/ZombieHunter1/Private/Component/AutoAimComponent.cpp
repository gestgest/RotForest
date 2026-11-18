// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/AutoAimComponent.h"

#include "Characters/CombatRegistrySubsystem.h"

// Sets default values for this component's properties
UAutoAimComponent::UAutoAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAutoAimComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<ACombatCharacter>(GetOwner());
}

// 자동으로 해서 림라이팅
void UAutoAimComponent::UpdateAutoTarget(float Range)
{
	UCombatRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatRegistrySubsystem>();

	// 사거리만큼 적 탐색
	ACombatCharacter* Nearest = Registry ? 
        Registry->FindNearestOfEnemy(GetOwner()->GetActorLocation(), ETeam::Enemy, Range) : nullptr;

	SetTarget(Nearest);
}

void UAutoAimComponent::ClearTarget()
{
    SetTarget(nullptr);
}


// 모듈형 적(머리·투구·갑옷...)의 모든 스켈레탈 메시에 오버레이를 건다
static void SetOverlayOnAllMeshes(AActor* Actor, UMaterialInterface* Material)
{
    TArray<USkeletalMeshComponent*> Meshes;

    // 모든 스켈레톤 매시 가져와라
    Actor->GetComponents<USkeletalMeshComponent>(Meshes);

    for (USkeletalMeshComponent* Mesh : Meshes)
    {
        Mesh->SetOverlayMaterial(Material);
    }
}

void UAutoAimComponent::SetTarget(ACombatCharacter* NewTarget)
{
    if (Target.Get() == NewTarget)
    {
        return;
    }

    if (ACombatCharacter* Old = Target.Get())
    {
        SetOverlayOnAllMeshes(Old, nullptr);
    }

    Target = NewTarget;

    if (NewTarget && HighlightMaterial)
    {
        SetOverlayOnAllMeshes(NewTarget, HighlightMaterial);
    }
}
