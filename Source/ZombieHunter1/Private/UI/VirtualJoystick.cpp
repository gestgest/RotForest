// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/VirtualJoystick.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"

void UVirtualJoystick::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 안 잡는 경우 휴식을 준다.
	if (bFloating && ActivePointerIndex == -1)
	{
		const FVector2D Rest = GetRestOffset(MyGeometry);
		if (!BaseOffset.Equals(Rest, 0.5f))
		{
			BaseOffset = Rest;
			ApplyVisuals(FVector2d::ZeroVector); // 원래 자리
		}
	}
}

TSharedRef<SWidget> UVirtualJoystick::RebuildWidget()
{
	// 위젯 블루프린트 없이 C++에서 트리를 직접 구성 (RootWidget이 비어있을 때만)
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		//캔버스 생성
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
		WidgetTree->RootWidget = RootOverlay;

		//배경 만들기
		Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JoystickBackground"));

		//중앙에 넣어라
		if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(Background))
		{
			//layout
			BgSlot->SetHorizontalAlignment(HAlign_Center);
			BgSlot->SetVerticalAlignment(VAlign_Center);
		}

		//핸들링 만들기
		Handle = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JoystickHandle"));

		//중앙에 넣어라
		if (UOverlaySlot* HandleSlot = RootOverlay->AddChildToOverlay(Handle))
		{
			//layout
			HandleSlot->SetHorizontalAlignment(HAlign_Center); 
			HandleSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UVirtualJoystick::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 전체가 터치/마우스를 받도록
	SetVisibility(ESlateVisibility::Visible);
}

float UVirtualJoystick::GetEffectiveRange() const
{
	if (HandleRange > 0.f)
	{
		return HandleRange;
	}
	// 자동: 베이스 반지름에서 손잡이 반지름을 뺀 값 (손잡이가 베이스 안에 머물도록)
	return FMath::Max(1.f, (BackgroundSize.X - HandleSize.X) * 0.5f);
}

FReply UVirtualJoystick::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return HandlePressed(InGeometry, InGestureEvent);
}

FReply UVirtualJoystick::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	// 내 손가락이 맞을 때만 반응 (멀티터치: 다른 조이스틱 손가락 무시)
	if (InGestureEvent.GetPointerIndex() == ActivePointerIndex)
	{
		HandleMoved(InGeometry, InGestureEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystick::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return HandleReleased(InGestureEvent);
}

FReply UVirtualJoystick::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandlePressed(InGeometry, InMouseEvent);
}

FReply UVirtualJoystick::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetPointerIndex() == ActivePointerIndex)
	{
		HandleMoved(InGeometry, InMouseEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystick::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleReleased(InMouseEvent);
}

//조이스틱을 놓친다면
void UVirtualJoystick::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);

	// 어떤 경로로든 입력 캡처가 끝나면 무조건 손잡이를 중앙으로 되돌리고 (0,0)을 보냄
	if (ActivePointerIndex != -1)
	{
		ActivePointerIndex = -1;
		ResetHandle();
	}
}

FReply UVirtualJoystick::HandlePressed(const FGeometry& InGeometry, const FPointerEvent& InEvent)
{
	// 이미 다른 손가락이 잡고 있으면 무시
	if (ActivePointerIndex != -1)
	{
		return FReply::Unhandled();
	}

	ActivePointerIndex = InEvent.GetPointerIndex();

	// 조이싙ㄱ 건드리고 있다면
	if (bFloating)
	{
		// px기준 어디 눌렀는지
		const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
		BaseOffset = ClampBase(InGeometry, LocalPos - InGeometry.GetLocalSize() * 0.5f);
	}

	UpdateFromScreenPosition(InGeometry, InEvent.GetScreenSpacePosition());

	// 이 포인터의 이후 Move/Up 이벤트를 이 위젯이 받도록 캡처
	return FReply::Handled().CaptureMouse(TakeWidget());
}

void UVirtualJoystick::HandleMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent)
{
	UpdateFromScreenPosition(InGeometry, InEvent.GetScreenSpacePosition());
}

FReply UVirtualJoystick::HandleReleased(const FPointerEvent& InEvent)
{
	if (InEvent.GetPointerIndex() != ActivePointerIndex)
	{
		return FReply::Unhandled();
	}

	ActivePointerIndex = -1;
	ResetHandle();
	return FReply::Handled().ReleaseMouseCapture();
}

void UVirtualJoystick::UpdateFromScreenPosition(const FGeometry& InGeometry, const FVector2D& ScreenPos)
{
	// 위젯 로컬 좌표로 변환 후 중심 기준 오프셋 계산
	const FVector2D LocalPos = InGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D WidgetCenter = InGeometry.GetLocalSize() * 0.5f;
	FVector2D Offset = LocalPos - (WidgetCenter + BaseOffset);

	// 손잡이가 벗어나지 않도록 반경 제한
	const float Range = GetEffectiveRange();
	if (Offset.Size() > Range)
	{
		// 넘친 만큼 배경을 끌고 옴
		if (bFloating && bFollowFinger)
		{
			BaseOffset = ClampBase(InGeometry, BaseOffset + Offset - Offset.GetSafeNormal() * Range);
			Offset = LocalPos - (WidgetCenter + BaseOffset);
		}

		if (Offset.Size() > Range)
		{
			Offset = Offset.GetSafeNormal() * Range;
		}
	}
	ApplyVisuals(Offset);

	// 정규화: 화면 Y는 아래가 +이므로 Y를 뒤집어 "위 = +" 로 맞춤 (게임패드 스틱과 동일)
	FVector2D Value(Offset.X / Range, -Offset.Y / Range);

	// 데드존 적용
	if (Value.Size() < DeadZone)
	{
		Value = FVector2D::ZeroVector;
	}

	OnJoystickMoved.Broadcast(Value);
}

void UVirtualJoystick::ResetHandle()
{
	ApplyVisuals(FVector2D::ZeroVector);
	OnJoystickMoved.Broadcast(FVector2D::ZeroVector);
}

FVector2D UVirtualJoystick::ClampBase(const FGeometry& InGeometry, const FVector2D& InOffset) const
{
	const FVector2D Half = InGeometry.GetLocalSize() * 0.5f - BackgroundSize * 0.5f;

	// 영역 전개
	const float MaxX = FMath::Max(0.f, Half.X);
	const float MaxY = FMath::Max(0.f, Half.Y);

	return FVector2D(
		FMath::Clamp(InOffset.X, -MaxX, MaxX),
		FMath::Clamp(InOffset.Y, -MaxY, MaxY)
	);
}

FVector2D UVirtualJoystick::GetRestOffset(const FGeometry& InGeometry) const
{
	const FVector2D Rest = (RestPosition - FVector2D(0.5f, 0.5f)) * InGeometry.GetLocalSize();
	return ClampBase(InGeometry, Rest);
}


// 위치로 이동
void UVirtualJoystick::ApplyVisuals(const FVector2D& HandleOffset) 
{
	if (Background)
	{
		Background->SetRenderTranslation(BaseOffset);
	}
	if (Handle)
	{
		Handle->SetRenderTranslation(BaseOffset + HandleOffset);
	}
}
