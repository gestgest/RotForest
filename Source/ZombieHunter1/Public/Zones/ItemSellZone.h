// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zones/GaugeZone.h"
#include "ItemSellZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;


// 밟으면 가방을 통째로 팔아 돈으로 바꾸는 발판.
UCLASS()
class ZOMBIEHUNTER1_API AItemSellZone : public AGaugeZone
{
	GENERATED_BODY()

public:
	AItemSellZone();

	// 한 개 팔릴 때마다 재생
	UPROPERTY(EditAnywhere, Category = "SellZone")
	USoundBase* SellSound = nullptr;
protected:
	// 여기서 OutAmount는 판 갯수, 그냥 1임
	virtual bool TryFillOnce(AMyPlayer* Player, int32& OutAmount) override;
	virtual void HandleZoneFilled(AMyPlayer* Player) override;
	virtual void OnPlayerEntered(AMyPlayer* Player) override;
	virtual void OnPlayerExited(AMyPlayer* Player) override;

private:
	// 다 완료하면 판매 문구
	void ShowSoldSummary(AMyPlayer * Player); 

	int32 SoldCount = 0;
	int32 SoldMoney = 0;
};
