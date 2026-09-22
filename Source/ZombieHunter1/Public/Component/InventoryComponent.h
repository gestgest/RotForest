// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemDataAsset;


// 아이템 보관 가방. 인벤토리 창은 없고 판매 발판이 전량을 비운다.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIEHUNTER1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 무게가 넘치면 안 넣고 false
	bool TryAddItem(UItemDataAsset* Item);

	// 상태를 바꾸지 않는 질문용. 넣을 때는 TryAddItem을 쓴다.
	bool HasRoomFor(UItemDataAsset* Item) const;

	float GetCurrentWeight() const;

	// 가방 전체를 팔았을 때 받는 금액
	int32 GetTotalSellPrice() const;

	void ClearAll();
	UItemDataAsset * PopItem(); //스택식 pop

	FORCEINLINE const TArray<UItemDataAsset*>& GetItems() const { return Items; }
	FORCEINLINE float GetMaxWeight() const { return MaxWeight; }

private:
	// 담을 수 있는 총 무게
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"), Category = "Inventory")
	float MaxWeight = 20.0f;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Inventory")
	TArray<UItemDataAsset*> Items;
};
