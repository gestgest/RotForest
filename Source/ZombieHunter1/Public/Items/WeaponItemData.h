// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Jobs/JobTypes.h"
#include "WeaponItemData.generated.h"

class USkeletalMesh;

// 무기 한 자루의 데이터. 값 타입이라 복사해서 들고 다닌다.
// 직업 컴포넌트(장착 무기), 픽업 액터(바닥에 떨어진 것), 상점/드랍이 전부 이 타입을 공유한다.
USTRUCT(BlueprintType)
struct FWeaponItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 WeaponPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	USkeletalMesh* Mesh = nullptr;

	// 이 무기를 쓸 수 있는 직업. 다른 직업은 장착할 수 없다(검사가 활을 못 듦).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EJobType JobType = EJobType::Warrior;
};
