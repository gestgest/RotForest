// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponPickup.generated.h"

class UBoxComponent;
class UWeaponDataAsset;

// 바닥에 떨어져 있는 무기 한 자루. 플레이어가 밟으면 장착되고 자신은 사라진다.
UCLASS()
class ZOMBIEHUNTER1_API AWeaponPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponPickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	UWeaponDataAsset* WeaponItemData = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider")
	USkeletalMeshComponent* WeaponMesh;


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider")
	UBoxComponent* TriggerBox;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

private:
	void EnablePickup();

	// 아무도 못 쓰는 무기라는 안내는 한 번만 — 오버랩이 반복 들어와도 도배되지 않게.
	bool bNoTakerNotified = false;

	FTimerHandle PickupDelayHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category="Pickup")
	float PickupDelay = 0.5f;
};
