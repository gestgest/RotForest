// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MenuDirector.generated.h"

class ACameraActor;
class UUserWidget;

UCLASS()
class ZOMBIEHUNTER1_API AMenuDirector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMenuDirector();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Menu")
	TObjectPtr<ACameraActor> MainMenuCamera;

	UPROPERTY(EditAnywhere, Category = "Menu")
	TObjectPtr<ACameraActor> JobSelectCamera;

	UPROPERTY(EditAnywhere, Category = "Menu")
	TSubclassOf<UUserWidget> JobSelectWidgetClass;

	UPROPERTY(EditAnywhere, Category="Menu")
	float BlendTime = 2.0f;

	void SwitchTo(ACameraActor* Camera, UUserWidget* Show, UUserWidget* Hide);

	UUserWidget* GetMainMenuWidget() const;

	UPROPERTY()
	TObjectPtr<UUserWidget> JobSelectWidget;

public:	
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowJobSelect();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowMainMenu();
};
