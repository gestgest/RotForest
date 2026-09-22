// Fill out your copyright notice in the Description page of Project Settings.

#include "Zones/WeaponUpgradeZone.h"
#include "Characters/MyPlayer.h"
#include "Engine/Engine.h"

void AWeaponUpgradeZone::HandleZoneFilled(AMyPlayer* Payer)
{
	Payer->UpgradeWeapon();

	// 다음 강화는 더 비싸게 — CompleteZone이 게이지를 이미 리셋한 뒤라 안전하다.
	RequiredAmount += CostGrowth;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
			FString::Printf(TEXT("[UpgradeZone] 무기 강화 완료! (다음 비용 %d원)"), RequiredAmount));
	}
}
