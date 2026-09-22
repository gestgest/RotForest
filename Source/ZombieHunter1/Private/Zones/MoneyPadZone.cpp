// Fill out your copyright notice in the Description page of Project Settings.

#include "Zones/MoneyPadZone.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

AMoneyPadZone::AMoneyPadZone()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMoneyPadZone::BeginPlay()
{
	Super::BeginPlay();

}

void AMoneyPadZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

