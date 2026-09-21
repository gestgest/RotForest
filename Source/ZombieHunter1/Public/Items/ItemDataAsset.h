// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Jobs/JobTypes.h"
#include "ItemDataAsset.generated.h"

class USkeletalMesh;


// 아이템 한 종류의 정의. 에셋 하나를 모두가 공유하므로 런타임에 값을 바꾸면 안 된다.
UCLASS(BlueprintType)
class ZOMBIEHUNTER1_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Name;

	// 가방 무게. 기본값: 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;

	// 판매 발판에서 받는 금액
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 Price = 0;
};


// 무기 한 종류의 정의.
UCLASS(BlueprintType)
class ZOMBIEHUNTER1_API UWeaponDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 WeaponPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMesh* Mesh = nullptr;

	// 이 무기를 쓸 수 있는 직업. 다른 직업은 장착할 수 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EJobType JobType = EJobType::Warrior;
};
