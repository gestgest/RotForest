// Fill out your copyright notice in the Description page of Project Settings.


#include "Zones/ItemSellZone.h"
#include "Component/InventoryComponent.h"
#include "Items/ItemDataAsset.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h"

AItemSellZone::AItemSellZone()
{
	Cooldown = 0.0f;
}

// 여기서 OutAmount는 판 갯수
bool AItemSellZone::TryFillOnce(AMyPlayer* Player, int32& OutAmount)
{
	UInventoryComponent* Bag = Player ? Player->GetBag() : nullptr;

	if (!Bag)
	{
		return false;
	}

	//아이템
	UItemDataAsset* Item = Bag->PopItem();
	if (!Item)
	{
		return false;
	}

	//돈
	int32 Money = Item->Price;
	Player->GainMoney(Money);

	//sold 값
	SoldCount++;
	SoldMoney += Money;

	//사운드
	UGameplayStatics::PlaySoundAtLocation(this, SellSound, GetActorLocation());

	OutAmount = 1;
	return true;
}

void AItemSellZone::HandleZoneFilled(AMyPlayer* Player)
{
	ShowSoldSummary(Player);
}

void AItemSellZone::OnPlayerEntered(AMyPlayer* Player)
{
	// Player가 null이면 nullptr
	UInventoryComponent* Inventory = Player ? Player->GetBag() : nullptr;

	RequiredAmount = Inventory ? FMath::Max(1, Inventory->GetItems().Num()) : 1;

	//초기화
	FilledAmount = 0;
	Progress = 0.0f;

	SoldCount = 0;
	SoldMoney = 0;

	// 시간을 쿨타임만큼 추가하면 바로 판매 가능
	Fill_Timer = Fill_Interval;
}

//문구만 뜨고
void AItemSellZone::OnPlayerExited(AMyPlayer* Player)
{
	ShowSoldSummary(Player);
}

void AItemSellZone::ShowSoldSummary(AMyPlayer * Player)
{
	//플레이어가 
	if (SoldCount <= 0 || !IsValid(Player))
	{
		SoldCount = 0;
		SoldMoney = 0;
		return;
	}

	// 메세지 보여주고
	FText Msg = FText::Format(FText::FromString(TEXT("아이템 {0}개를 팔아 {1}원을 받았습니다.")), SoldCount, SoldMoney);
	Player->ShowOnItemText(Msg, EItemNotifyType::Gain);

	// 초기화
	SoldCount = 0;
	SoldMoney = 0;
}
