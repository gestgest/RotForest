// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSellZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;


// 밟으면 가방을 통째로 팔아 돈으로 바꾸는 발판.
UCLASS()
class ZOMBIEHUNTER1_API AItemSellZone : public AActor
{
	GENERATED_BODY()

public:
	AItemSellZone();

protected:
	virtual void BeginPlay() override;

	// 밟는 영역(트리거)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SellZone")
	UBoxComponent* TriggerBox;

	// 발판 바닥 메시(선택). BP에서 지정.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SellZone")
	UStaticMeshComponent* PadMesh;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
