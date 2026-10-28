// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "VirtualJoystick.generated.h"

class UImage;

/** 조이스틱 값이 바뀔 때마다(드래그 중 매 입력) 브로드캐스트. Value는 정규화된 (-1~1), Y는 위가 + */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoystickMoved, FVector2D, Value);

/**
 * 모바일 터치 가상 조이스틱 위젯.
 * 위젯 트리(배경 + 손잡이 이미지)를 C++에서 직접 생성하므로 별도의 위젯 블루프린트가 필요 없다.
 * CreateWidget<UVirtualJoystick>(...) 로 바로 만들어 AddToViewport 하면 된다.
 * 드래그 입력은 OnJoystickMoved 델리게이트로 내보낸다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UVirtualJoystick : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 드래그할 때마다 정규화 벡터(-1~1, Y 위가 +)를 전달. 손가락을 떼면 (0,0)을 한 번 보냄 */
	UPROPERTY(BlueprintAssignable, Category = "Joystick")
	FOnJoystickMoved OnJoystickMoved;

	/** 베이스 크기(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	FVector2D BackgroundSize = FVector2D(220.f, 220.f);

	/** 손잡이 크기(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	FVector2D HandleSize = FVector2D(100.f, 100.f);

	/** 손잡이가 중심에서 움직일 수 있는 최대 반경(px). 0 이하면 베이스 크기에서 자동 계산 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	float HandleRange = 0.f;

	/** 데드존(정규화 기준). 이보다 작은 입력은 (0,0)으로 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	float DeadZone = 0.1f;


protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	// 데스크탑(마우스)에서도 테스트할 수 있도록 마우스 입력도 동일하게 처리
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스/터치 캡처를 잃으면(손가락 떼거나 포커스 이탈) 반드시 손잡이를 0으로 리셋.
	// (Up 이벤트가 위젯에 안 들어와도 마지막 입력이 남아 계속 움직이는 것 방지)
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

	// 누른 자리 배경옮김
	UPROPERTY(EditAnywhere, Category = "Joystick")
	bool bFloating = true;

	// 반경 밖으로 나가면 배경이 끌러옴
	UPROPERTY(EditAnywhere, Category = "Joystick")
	bool bFollowFinger = true;

	// 조이스틱 아무 것도 안하면 def
	UPROPERTY(EditAnywhere, Category = "Joystick")
	FVector2D RestPosition = FVector2D(0.3f, 0.6f);


private:
	/** 위젯 BP에서 같은 이름의 Image를 두면 자동 바인딩됨. 없으면 C++가 직접 생성 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Background;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Handle;

	/** 현재 이 조이스틱을 잡고 있는 포인터(손가락) 번호. -1이면 비활성 */
	int32 ActivePointerIndex = -1;

	// 위젯 default 가운데 기준 배경이 옮긴 거리(px)
	FVector2D BaseOffset = FVector2D::ZeroVector;


	/** 실제 사용할 손잡이 반경 */
	float GetEffectiveRange() const;

	FReply HandlePressed(const FGeometry& InGeometry, const FPointerEvent& InEvent);
	void HandleMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent);
	FReply HandleReleased(const FPointerEvent& InEvent);

	void UpdateFromScreenPosition(const FGeometry& InGeometry, const FVector2D& ScreenPos);
	void ResetHandle();

	// 터치 영역 바깥으로 나가지 말도록
	FVector2D ClampBase(const FGeometry& InGeometry, const FVector2D& InOffset) const;
	FVector2D GetRestOffset(const FGeometry& InGeometry) const;
	void ApplyVisuals(const FVector2D& HandleOffset);
};
