// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponPickup.h"

// Sets default values
AWeaponPickup::AWeaponPickup()
{
	// 픽업은 매 프레임 할 일이 없다. 무한 맵에 픽업이 쌓이면 그대로 비용이 되므로 꺼둔다.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

