// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/WeaponPickup.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Characters/MyPlayer.h"
#include "Characters/PartyComponent.h"
#include "Characters/CombatCharacter.h"
#include "Component/InventoryComponent.h"
#include "Items/ItemDataAsset.h"

// Sets default values
AWeaponPickup::AWeaponPickup()
{
	// 픽업은 매 프레임 할 일이 없다. 무한 맵에 픽업이 쌓이면 그대로 비용이 되므로 꺼둔다.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(false);


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
	if (WeaponItemData && WeaponItemData->Mesh)
	{
		WeaponMesh->SetSkeletalMeshAsset(WeaponItemData->Mesh);
	}

	//딜레이
	if (PickupDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(PickupDelayHandle, this, &AWeaponPickup::EnablePickup, PickupDelay);
	}
	else //딜레이 값이 없으면 그냥 바로 실행
	{
		EnablePickup();
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

	UPartyComponent* Party = MyPlayer->GetParty();
	if (!Party)
	{
		return;
	}

	if (!WeaponItemData)
	{
		return;
	}

	// 누가 가져갈지(플레이어 우선, 없으면 동료)는 파티가 정한다. 획득 문구도 파티가 띄운다.
	UWeaponDataAsset* Replaced = nullptr;
	const ACombatCharacter* Receiver = Party->TryDistributeWeapon(WeaponItemData, Replaced);
	UInventoryComponent* Bag = MyPlayer->GetBag();

	// 받는 사람이 없다면
	if (!Receiver)
	{
		// 아무도 못 쓰면 가방으로. 무게가 넘치면 바닥에 그대로 남는다.
		if (!Bag || !Bag->TryAddItem(WeaponItemData))
		{
			if (!bNoTakerNotified)
			{
				bNoTakerNotified = true;
				FText Msg = FText::Format(FText::FromString(TEXT("가방이 가득 차 {0}을 넣을 수 없습니다.")), (WeaponItemData->Name));
				MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Blocked);
			}
			return;
		}

		FText Msg = FText::Format(FText::FromString(TEXT("{0}을 가방에 넣었습니다.")), (WeaponItemData->Name));
		MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);
	}
	// 대체 됐다면
	else if (Replaced)
	{
		if (!Bag || !Bag->TryAddItem(Replaced))
		{
			bNoTakerNotified = true;
			FText Msg = FText::Format(FText::FromString(TEXT("가방이 가득 차 {0}을 바닥에 내려놓았습니다.")), (Replaced->Name));
			MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Blocked);

			BecomeWeapon(Replaced);
			return;
		}
		// else 가방안에 들어감
		FText Msg = FText::Format(FText::FromString(TEXT("{0}을 가방에 넣었습니다.")), (Replaced->Name));
		MyPlayer->ShowOnItemText(Msg, EItemNotifyType::Gain);
	}

	// 중복 오버랩 — 한 번 주운 뒤 이벤트가 또 들어오는 경우를 어떻게 막을지
	TriggerBox->SetGenerateOverlapEvents(false);
	Destroy();
}

void AWeaponPickup::EnablePickup()
{
	if (!TriggerBox)
	{
		return;
	}

	
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->UpdateOverlaps();
}

void AWeaponPickup::BecomeWeapon(UWeaponDataAsset * NewItem)
{
	WeaponItemData = NewItem;
	WeaponMesh->SetSkeletalMeshAsset(NewItem ? NewItem->Mesh : nullptr);

	//바로 주울 수 있으니 접촉 비활성화
	TriggerBox->SetGenerateOverlapEvents(false);
	GetWorldTimerManager().SetTimer(PickupDelayHandle, this,
		&AWeaponPickup::EnablePickup, FMath::Max(PickupDelay, 0.5f));
}
