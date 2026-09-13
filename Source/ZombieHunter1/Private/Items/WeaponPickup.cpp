// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponPickup.h"
#include "Components/BoxComponent.h"

// Sets default values
AWeaponPickup::AWeaponPickup()
{
	// 픽업은 매 프레임 할 일이 없다. 무한 맵에 픽업이 쌓이면 그대로 비용이 되므로 꺼둔다.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnTriggerBeginOverlap);
	}

}

// Called every frame
void AWeaponPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AWeaponPickup::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/) 
{

}
