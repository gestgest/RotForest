// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/TopDownInputComponent.h"
#include "Components/InputComponent.h"

#include "UI/VirtualJoystick.h"
#include "UI/MyCanvas.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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
	PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightY, this, &UTopDownInputComponent::OnAimY);

	// 마우스
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UTopDownInputComponent::OnLeftMousePressed);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &UTopDownInputComponent::OnLeftMouseReleased);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UTopDownInputComponent::OnRightMousePressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &UTopDownInputComponent::OnRightMouseReleased);
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

static FVector2D Larger(const FVector2D& A, const FVector2D & B)
{
	return (A.SizeSquared() >= B.SizeSquared()) ? A : B;
}


FTopDownInput UTopDownInputComponent::Gather()
{
	FTopDownInput Input;

	FVector2D MouseMove = FVector2D::ZeroVector;
	FVector2D MouseAim = FVector2D::ZeroVector;
	MouseInput(MouseMove, MouseAim);
	
	// 터치 / 게임패드 / 마우스 중 가장 큰 입력
	Input.Move = ApplyDeadzone(Larger(Larger(TouchMove, GamepadMove), MouseMove));
	Input.Aim = ApplyDeadzone(Larger(GamepadAim, MouseAim));
	Input.bAttackHeld = bAttackButtonHeld;

	// 커서가 찍은 땅의 정확한 위치
	if (bLeftMouseHeld && bHasLastCursorPoint)
	{
		Input.bHasCursorAimPoint = true;
		Input.CursorAimPoint = LastCursorPoint;
	}

	return Input;
}

// 모든 잡는 변수 false : 죽은 경우에 활용 
void UTopDownInputComponent::ReleaseAll()
{
	bAttackButtonHeld = false;
	bLeftMouseHeld = false;
	bRightMouseHeld = false;
}

// 커서 방향 => 스틱 포맷
void UTopDownInputComponent::MouseInput(FVector2D& OutMove, FVector2D& OutAim)
{
	// 둘다 안 눌렀다면
	if(!bLeftMouseHeld && !bRightMouseHeld)
	{
		return;
	}

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	FVector MoveDir = FVector::ZeroVector;
	FVector Cursor;

	if (GetCursorGroundLocation(Cursor))
	{
		LastCursorPoint = Cursor;
		bHasLastCursorPoint = true;

		FVector ToCursor = Cursor - OwnerLocation;
		ToCursor.Z = 0.0f;

		if (ToCursor.SizeSquared() > CursorStopRadius * CursorStopRadius && ToCursor.Normalize())
		{
			LastCursorDir = ToCursor;
			MoveDir = ToCursor;
		}
	}
	else
	{
		MoveDir = LastCursorDir;
	}

	// 이동 => 
	if (!MoveDir.IsNearlyZero() && bRightMouseHeld)
	{
		// 언리얼 평균 입력 시스템 수준... 조이스틱이면 X Y 바꿔야함.
		OutMove = FVector2D(MoveDir.Y, MoveDir.X);
	}

	// 조준
	if (bLeftMouseHeld && bHasLastCursorPoint)
	{
		FVector ToAim = LastCursorPoint - OwnerLocation;
		ToAim.Z = 0.0f;
		ToAim.Normalize();

		// 언리얼 평균 입력 시스템 수준... 조이스틱이면 X Y 바꿔야함.
		OutAim = FVector2D(ToAim.Y, ToAim.X);
	}


}

bool UTopDownInputComponent::GetCursorGroundLocation(FVector& OutLocation) const 
{
	const APawn* Pawn = GetOwner<APawn>();
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	FVector WorldOrigin, WorldDirection;

	// 2d 마우스 좌표를 3d 월드 광선으로 바꾸는 코드 => WorldDirection
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	// WorldDirection가 0이면 아예 수직이니 == 자기자리 무시해라.
	// 자기 그 자리를 우클릭한거
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	// 플레이어와 우클릭한 좌표 Z 비교
	const float T = (Pawn->GetActorLocation().Z - WorldOrigin.Z) / WorldDirection.Z;

	// 평면이 카메라 뒷쪽 => 거의 안 생긴다.
	if (T < 0.0f)
	{
		return false;
	}

	OutLocation = WorldOrigin + WorldDirection * T;
	return true;
}

FVector2D UTopDownInputComponent::ApplyDeadzone(const FVector2D& Value) const
{
	return (Value.SizeSquared() > InputDeadzone * InputDeadzone) ? Value : FVector2D::ZeroVector;
}


void UTopDownInputComponent::OnMoveX(float Value) { GamepadMove.X = Value; }
void UTopDownInputComponent::OnMoveY(float Value) { GamepadMove.Y = Value; }
void UTopDownInputComponent::OnAimX(float Value) { GamepadAim.X = Value; }
void UTopDownInputComponent::OnAimY(float Value) { GamepadAim.Y = Value; }

void UTopDownInputComponent::OnLeftMousePressed() { bLeftMouseHeld = true; }
void UTopDownInputComponent::OnLeftMouseReleased() { bLeftMouseHeld = false; }
void UTopDownInputComponent::OnRightMousePressed() { bRightMouseHeld = true; }
void UTopDownInputComponent::OnRightMouseReleased() { bRightMouseHeld = false; }

void UTopDownInputComponent::OnMoveJoystickMoved(FVector2D Value) { TouchMove = Value; }
void UTopDownInputComponent::OnAttackButtonPressed() { bAttackButtonHeld = true; }
void UTopDownInputComponent::OnAttackButtonReleased() { bAttackButtonHeld = false; }
