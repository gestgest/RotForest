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
