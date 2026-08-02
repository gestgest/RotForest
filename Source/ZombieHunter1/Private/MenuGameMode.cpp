// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"

AMenuGameMode::AMenuGameMode()
{
	// 메뉴 씬엔 걸어다닐 캐릭터가 필요 없다. None으로 두면 스폰 실패 에러가 찍히므로
	// 보이지 않는 관전자 폰을 쓴다 (입력이 UI 전용이라 움직이지도 않음).
	DefaultPawnClass = ASpectatorPawn::StaticClass();
}

void AMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 메뉴 위젯 생성 + 표시
	if (MenuWidgetClass)
	{
		MenuWidget = CreateWidget<UUserWidget>(GetWorld(), MenuWidgetClass);
		if (MenuWidget)
		{
			MenuWidget->SetIsFocusable(true);   // ← AddToViewport 전에 호출해야 먹는다
			MenuWidget->AddToViewport();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Menu] MenuWidgetClass가 비어 있음 — 이 맵의 게임모드 BP에서 띄울 위젯을 지정하세요."));
	}

	// 마우스 커서 표시 + UI 전용 입력 (버튼만 조작 가능)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		if (MenuWidget)
		{
			InputMode.SetWidgetToFocus(MenuWidget->TakeWidget()); // 게임패드/키보드 포커스
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}
