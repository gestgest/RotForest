// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryComponent.h"
#include "Items/ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


bool UInventoryComponent::TryAddItem(UItemDataAsset* Item)
{
	if (!Item || !HasRoomFor(Item))
	{
		return false;
	}

	Items.Add(Item);
	return true;
}


bool UInventoryComponent::HasRoomFor(UItemDataAsset* Item) const
{
	if (!Item)
	{
		return false;
	}

	return GetCurrentWeight() + Item->Weight <= MaxWeight;
}


float UInventoryComponent::GetCurrentWeight() const
{
	float Total = 0.0f;
	for (const UItemDataAsset* Item : Items)
	{
		if (Item)
		{
			Total += Item->Weight;
		}
	}

	return Total;
}


int32 UInventoryComponent::GetTotalSellPrice() const
{
	int32 Total = 0;
	for (const UItemDataAsset* Item : Items)
	{
		if (Item)
		{
			Total += Item->Price;
		}
	}

	return Total;
}


void UInventoryComponent::ClearAll()
{
	Items.Empty();
}

//너무 자주 Pop으로 메모리 줄어드는 거 호출하면 언리얼도 부담부담
UItemDataAsset* UInventoryComponent::PopItem()
{
	if (0 < Items.Num())
	{
		UItemDataAsset* Item = Items.Pop(EAllowShrinking::No);
		if (Item)
		{
			return Item;
		}
	}

	return nullptr;
}
