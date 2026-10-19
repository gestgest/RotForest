// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ZoneGaugeWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UZoneGaugeWidget::SetPercent(float Percent)
{
	if (GaugeBar)
	{
		GaugeBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UZoneGaugeWidget::SetTitle(const FText& Title)
{
	if (NameText)
	{
		NameText->SetText(Title);
	}

}
void UZoneGaugeWidget::SetCost(int32 Cost)
{
	if (CostText)
	{
		CostText->SetText(FText::AsNumber(Cost));
	}
}
