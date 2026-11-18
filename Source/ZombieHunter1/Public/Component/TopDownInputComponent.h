// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TopDownInputComponent.generated.h"

class UInputComponent;
class UMyCanvas;

struct FTopDownInput
{
	FVector2D Move = FVector2D::ZeroVector;
	FVector2D Aim = FVector2D::ZeroVector;
	bool bAttackHeld = false;

	bool bHasCursorAimPoint = false;
	FVector CursorAimPoint = FVector::ZeroVector;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIEHUNTER1_API UTopDownInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTopDownInputComponent();

protected:

public:	

	// input에서 호출
	void BindKeys(UInputComponent* PlayerInputComponent);

	// SetCanvasWidgets에서 호출
	void BindCanvas(UMyCanvas* Canvas);

	// tick : 매 프레임 호출
	FTopDownInput Gather();

	// 사망 시 눌린 상태 해체
	void ReleaseAll();

private:
	UPROPERTY(EditAnywhere, Category="Input")
	float InputDeadzone = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Input")
	float CursorStopRadius = 60.0f;

	FVector2D GamePadMove = FVector2D::ZeroVector;
	FVector2D GamePadAim = FVector2D::ZeroVector;
	FVector2D TouchMove = FVector2D::ZeroVector;

	bool bLeftMouseHeld = false;
	bool bRightMouseHeld = false;
	bool bAttackButtonHeld = false;

	FVector LastCursorDir = FVector::ForwardVector;
	FVector LastCursorPoint = FVector::ZeroVector;
	bool bHasLastCursorPoint = false;

	void MouseInput(FVector2D& OutMove, FVector2D& OutAim);
	bool GetCursorGroundLocation(FVector& OutLocation) const;
	FVector2D ApplyDeadZone(const FVector2D& Value) const;

	// [게임패드]
	void OnMoveX(float Value);
	void OnMoveY(float Value);
	void OnAimX(float Value);
	void OnAimY(float Value);

	// [마우스]
	void OnLeftMousePressed();
	void OnLeftMouseReleased();
	void OnRightMousePressed();
	void OnRightMouseReleased();

	// [터치]
	UFUNCTION()
	void OnMoveJoystickMoved(FVector2D Value);

	UFUNCTION()
	void OnAttackButtonPressed();

	UFUNCTION()
	void OnAttackButtonReleased();
		
};
