// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PartyComponent.h"
#include "Characters/Companion.h"
#include "Characters/CombatCharacter.h"
#include "Characters/MyPlayer.h" //획득 문구 표시(ShowOnItemText)
#include "Jobs/JobComponent.h"
#include "Items/ItemDataAsset.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h" //동료 스폰 시 캡슐 반높이
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h" //GEngine 화면 디버그

UPartyComponent::UPartyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// [섭외]
void UPartyComponent::RecruitCompanion(TSubclassOf<UJobComponent> JobComponent)
{
	UWorld* World = GetWorld();
	if (!CanRecruit(World))
	{
		return;
	}

	const FTransform SpawnTM = MakeSpawnTransform(World);

	// 지연 스폰: BeginPlay가 돌기 전에 Leader/직업을 세팅해야
	ACompanion* Companion = World->SpawnActorDeferred<ACompanion>(
		CompanionClass, SpawnTM, GetOwner(), nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Companion)
	{
		return;
	}

	Companion->Leader = GetOwner();

	Companion->DefaultJobClass = JobComponent; //직업 셋팅

	UGameplayStatics::FinishSpawningActor(Companion, SpawnTM);

	Companions.Add(Companion);
}

bool UPartyComponent::CanRecruit(UWorld* World)
{
	if (!CompanionClass)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
				TEXT("[Party] CompanionClass가 비어있음 - BP_MyPlayer의 Party 컴포넌트에서 BP_Companion 지정"));
		}
		return false;
	}

	if (!World || !GetOwner())
	{
		return false;
	}

	// 죽어서 사라진 동료는 목록에서 제거한 뒤 인원 체크.
	Companions.RemoveAll([](const ACompanion* C) { return !IsValid(C); });
	if (Companions.Num() >= MaxCompanions)
	{
		return false;
	}
	return true;
}

FTransform UPartyComponent::MakeSpawnTransform(UWorld* World) const
{
	// 소유자 회전 기준으로 오프셋을 적용해 옆/뒤쪽에 스폰(여러 명이면 살짝씩 벌어지게).
	const AActor* Owner = GetOwner();
	const FRotator SpawnRot = Owner->GetActorRotation();
	FVector Offset = CompanionSpawnOffset;
	Offset.Y += Companions.Num() * 80.0f; // 두 번째부터는 옆으로 더 벌려 겹침 방지
	FVector SpawnLoc = Owner->GetActorLocation() + SpawnRot.RotateVector(Offset);

	// 바닥에 맞춰 스폰 — 아래로 트레이스해 지면을 찾고 그 위에 캡슐 반높이만큼 띄운다.
	const FVector TraceStart = SpawnLoc + FVector(0, 0, 200.0f);
	const FVector TraceEnd = SpawnLoc - FVector(0, 0, 1000.0f);
	FHitResult Hit;
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(Owner);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Q))
	{
		const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
		const float HalfHeight = OwnerCharacter ? OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f;
		SpawnLoc = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 2.0f);
	}

	FTransform SpawnTM(SpawnRot, SpawnLoc);
	return SpawnTM;
}


// [장비]
// 소유자(플레이어)가 먼저 판단하고, 안 쓰면 동료 중 한 명에게 넘긴다.
ACombatCharacter* UPartyComponent::TryDistributeWeapon(UWeaponDataAsset* Item)
{
	if (!Item)
	{
		return nullptr;
	}

	ACombatCharacter* OwnerCharacter = Cast<ACombatCharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->WantsWeaponItem(Item) && OwnerCharacter->EquipWeaponItem(Item))
	{
		NotifyWeaponTaken(Item, OwnerCharacter);
		return OwnerCharacter;
	}

	// 쓸 수 있는 동료 중 지금 무기가 가장 약한 한 명 => 파티 전체 전투력 상승폭이 가장 크다
	ACompanion* Best = nullptr;
	for (ACompanion* Companion : Companions)
	{
		if (!IsValid(Companion) || Companion->GetIsDead() || !Companion->WantsWeaponItem(Item))
		{
			continue;
		}

		if (!Best || Companion->GetEquippedWeaponPower() < Best->GetEquippedWeaponPower())
		{
			Best = Companion;
		}
	}

	if (Best && Best->EquipWeaponItem(Item))
	{
		NotifyWeaponTaken(Item, Best);
		return Best;
	}

	return nullptr;
}

void UPartyComponent::NotifyWeaponTaken(UWeaponDataAsset* Item, ACombatCharacter* Receiver) const
{
	AMyPlayer* Player = Cast<AMyPlayer>(GetOwner());
	if (!Player || !Receiver || !Item)
	{
		return;
	}

	FText Msg;
	if (Receiver == Player)
	{
		Msg = FText::Format(FText::FromString(TEXT("{0}을 획득했습니다.")), Item->Name);
	}
	else
	{
		// 직업 이름은 EJobType의 UMETA(DisplayName)에서 가져온다 (=>"궁수")
		const UJobComponent* Job = Receiver->GetJobComponent();
		const FText JobName = Job
			? StaticEnum<EJobType>()->GetDisplayNameTextByValue(static_cast<int64>(Job->JobType))
			: FText::FromString(TEXT("동료"));

		Msg = FText::Format(FText::FromString(TEXT("{0} 동료가 {1}을 획득했습니다.")), JobName, Item->Name);
	}

	Player->ShowOnItemText(Msg);
}
