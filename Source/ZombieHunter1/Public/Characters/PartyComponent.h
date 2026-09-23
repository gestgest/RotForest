// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PartyComponent.generated.h"

class ACompanion;
class ACombatCharacter;
class UJobComponent;
class UWeaponDataAsset;

// 플레이어가 데리고 다니는 동료 파티 — 섭외/명단/장비 배분을 담당한다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEHUNTER1_API UPartyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPartyComponent();

	// 동료 섭외 — 소유자 옆에 동료를 스폰해 따라다니며 싸우게 한다. 동료 소환 발판(ACompanionSpawnZone)이 호출.
	void RecruitCompanion(TSubclassOf<UJobComponent> JobComponent);

	// 무기 한 자루를 파티에 배분한다. 받은 캐릭터를 반환하고, 아무도 못 쓰면 nullptr.
	// OutReplaced: 받은 캐릭터가 원래 들고 있던 무기 (없으면 nullptr)
	ACombatCharacter* TryDistributeWeapon(UWeaponDataAsset* Item, UWeaponDataAsset*& OutReplaced);

	// 현재 섭외해 둔 동료 목록(읽기 전용)
	FORCEINLINE const TArray<ACompanion*>& GetCompanions() const { return Companions; }

	// 섭외 가능한 상태인지 검사하고, 죽은 동료를 명단에서 정리한다.
	bool CanRecruit(UWorld* World);

private:
	// [명단]
	// 스폰할 동료 클래스(BP_Companion 지정). 비우면 섭외 안 됨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACompanion> CompanionClass;

	// 최대 동료 수. 이 인원에 도달하면 더 섭외하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party", meta = (AllowPrivateAccess = "true"))
	int32 MaxCompanions = 3;

	// 동료를 소유자 기준 어디에 스폰할지 오프셋(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party", meta = (AllowPrivateAccess = "true"))
	FVector CompanionSpawnOffset = FVector(-120.0f, 120.0f, 0.0f);

	// 현재 섭외해 둔 동료들(런타임). 죽으면 정리된다.
	UPROPERTY(BlueprintReadOnly, Category = "Party", meta = (AllowPrivateAccess = "true"))
	TArray<ACompanion*> Companions;

	// [섭외]

	// 소유자 옆 자리를 잡아 스폰 트랜스폼을 만든다.
	FTransform MakeSpawnTransform(UWorld* World) const;

	// [장비]
	// 무기를 누가 가져갔는지 플레이어 UI에 알린다.
	void NotifyWeaponTaken(UWeaponDataAsset* Item, ACombatCharacter* Receiver) const;
};
