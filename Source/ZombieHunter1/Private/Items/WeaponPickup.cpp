// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponPickup.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Characters/MyPlayer.h"

// Sets default values
AWeaponPickup::AWeaponPickup()
{
	// 픽업은 매 프레임 할 일이 없다. 무한 맵에 픽업이 쌓이면 그대로 비용이 되므로 꺼둔다.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);


	// 땅에 놓인 무기 외형. 실제 메시는 WeaponItemData.Mesh에서 꽂는다.
	// 충돌은 끔 — 판정은 TriggerBox 하나만 담당한다(메시까지 켜면 오버랩이 두 번 들어온다).
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(TriggerBox);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnTriggerBeginOverlap);
	}
	if (WeaponItemData.Mesh)
	{
		WeaponMesh->SetSkeletalMeshAsset(WeaponItemData.Mesh);
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
	AMyPlayer* MyPlayer = Cast<AMyPlayer>(OtherActor);
	if (!MyPlayer)
	{
		return;
	}


	//줍는 데미지가 약하다면
	if (WeaponItemData.WeaponPower <= MyPlayer->GetEquippedWeapon().WeaponPower)
	{
		return;
	}

	//장착
	if (!MyPlayer->EquipWeaponItem(WeaponItemData))
	{
		return;
	}

	//중복 오버랩 — 한 번 주운 뒤 이벤트가 또 들어오는 경우를 어떻게 막을지
	TriggerBox->SetGenerateOverlapEvents(false);
	Destroy();
}
