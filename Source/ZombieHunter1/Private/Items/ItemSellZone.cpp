// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemSellZone.h"
#include "Component/InventoryComponent.h"
#include "Characters/MyPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"


AItemSellZone::AItemSellZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// 판정은 TriggerBox 하나만 담당한다.
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AItemSellZone::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AItemSellZone::OnTriggerBeginOverlap);
	}
}


void AItemSellZone::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	AMyPlayer* MyPlayer = Cast<AMyPlayer>(OtherActor);
	if (!MyPlayer)
	{
		return;
	}

	UInventoryComponent* Bag = MyPlayer->GetBag();
	if (!Bag)
	{
		return;
	}

	const int32 Total = Bag->GetTotalSellPrice();
	if (Total <= 0)
	{
		return;
	}

	const int32 Count = Bag->GetItems().Num();

	Bag->ClearAll();
	MyPlayer->GainMoney(Total);

	FText Msg = FText::Format(FText::FromString(TEXT("아이템 {0}개를 팔아 {1}원을 받았습니다.")), Count, Total);
	MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);
}
