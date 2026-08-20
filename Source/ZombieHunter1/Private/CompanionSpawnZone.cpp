// Fill out your copyright notice in the Description page of Project Settings.

#include "CompanionSpawnZone.h"
#include "Characters/MyPlayer.h"
#include "Engine/Engine.h"

void ACompanionSpawnZone::HandleZoneFilled(AMyPlayer* Player)
{
	Player->RecruitCompanion(GetJobComponent());
}

//ACompanionSpawnZone::
void ACompanionSpawnZone::SetIconMeshComponent(USkeletalMeshComponent* Comp)
{
	IconMeshComp = Comp;
}

void ACompanionSpawnZone::ChangeIconMesh()
{
	TSubclassOf<UJobComponent> JobClass = GetJobComponent();
	if (JobClass && IconMeshComp)
	{
		IconMeshComp->SetSkeletalMeshAsset(JobClass.GetDefaultObject()->GetWeaponMesh());
	}
}

//나의 잡 컴포넌트 : 여기서 주를 이룬다.
TSubclassOf<UJobComponent> ACompanionSpawnZone::GetJobComponent()
{
	TSubclassOf<UJobComponent>* Found = JobComponents.Find(JobType);
	return Found ? *Found : nullptr; //value값을 줘라
}
