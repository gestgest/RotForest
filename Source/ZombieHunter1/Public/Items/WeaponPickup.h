// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WeaponItemData.h"
#include "WeaponPickup.generated.h"

class UBoxComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider")
	UBoxComponent* TriggerBox;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Data")
	FWeaponItemData WeaponItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider")
	USkeletalMeshComponent* WeaponMesh;

private:
};
