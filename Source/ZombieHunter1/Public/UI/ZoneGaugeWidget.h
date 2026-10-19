// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZoneGaugeWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class ZOMBIEHUNTER1_API UZoneGaugeWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* GaugeBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CostText;

public:
	void SetPercent(float Percent);
	void SetTitle(const FText& Title);
	void SetCost(int32 Cost);
};
