// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/TopDownInputComponent.h"
#include "Components/InputComponent.h"

#include "GameFramework/PlayerController.h"
#include "UI/VirtualJoystick.h"
#include "UI/MyCanvas.h"

#include "InputCoreTypes.h"

// Sets default values for this component's properties
UTopDownInputComponent::UTopDownInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}
void UTopDownInputComponent::BindKeys(UInputComponent* PlayerInputComponent)
{
	// 게임패드
	PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftX, this, &UTopDownInputComponent::OnMoveX);
	PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftY, this, &UTopDownInputComponent::OnMoveY);
	PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightX, this, &UTopDownInputComponent::OnAimX);
	PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightX, this, &UTopDownInputComponent::OnAimY);

	// 마우스
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UTopDownInputComponent::OnLeftMousePressed);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &UTopDownInputComponent::OnLeftMouseReleased);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UTopDownInputComponent::OnRightMousePressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UTopDownInputComponent::OnRightMouseReleased);
}



void UTopDownInputComponent::BindCanvas(UMyCanvas* Canvas)
{
	if (!Canvas)
	{
		return;
	}

	if (UVirtualJoystick* MoveStick = Canvas->GetMoveJoystick())
	{
		MoveStick->OnJoystickMoved.AddUniqueDynamic(this, &UTopDownInputComponent::OnMoveJoystickMoved);
	}
	if (UButton* AttackBtn = Canvas->GetAttackButton())
	{
		AttackBtn->OnPressed.AddUniqueDynamic(this, &UTopDownInputComponent::OnAttackButtonPressed);
		AttackBtn->OnReleased.AddUniqueDynamic(this, &UTopDownInputComponent::OnAttackButtonReleased);
	}
}

// Larger
