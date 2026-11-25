// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuDirector.h"

#include "MenuGameMode.h"
#include "Blueprint/UserWidget.h"

#include "Camera/CameraActor.h"

#include "GameFramework/PlayerController.h"

#include "Kismet/GameplayStatics.h"


// Sets default values
AMenuDirector::AMenuDirector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMenuDirector::BeginPlay()
{
	Super::BeginPlay();
	
	if (JobSelectWidgetClass)
	{
		JobSelectWidget = CreateWidget<UUserWidget>(GetWorld(), JobSelectWidgetClass);
		if (JobSelectWidget)
		{
			JobSelectWidget->SetIsFocusable(true);
		}
	}
}

// 스위치
void AMenuDirector::SwitchTo(ACameraActor* Camera, UUserWidget* Show, UUserWidget* Hide)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	if (Hide)
	{
		// 부모에게서 떼어냄 => 입력도 못받은? 한 마디로 Collapsed임
		// 그러니까 합법적으로 메모리 드는 느낌?
		Hide->RemoveFromParent();
	}

	if (Camera)
	{
		// Camera한테 BlendTime초 동안 어떤 곡선으로 가는지.
		PC->SetViewTargetWithBlend(Camera, BlendTime, VTBlend_Cubic);
	}

	if (Show)
	{
		// 위젯을 게임 화면 위에 표시
		Show->AddToViewport();

		FInputModeUIOnly InputMode;

		//Show->TakeWidget() => slate 변환기 => slate란 코
		InputMode.SetWidgetToFocus(Show->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 혹시 몰라서 넣은겨
		PC->SetInputMode(InputMode);
	}
}


void AMenuDirector::ShowJobSelect()
{
	// 메인 메뉴 => 직업 선택 UI
	SwitchTo(JobSelectCamera, JobSelectWidget, GetMainMenuWidget());
}

void AMenuDirector::ShowMainMenu()
{
	// 직업 선택 UI => 메인 메뉴
	SwitchTo(MainMenuCamera, GetMainMenuWidget(), JobSelectWidget);
}

UUserWidget* AMenuDirector::GetMainMenuWidget() const
{
	const AMenuGameMode* GameMode = GetWorld()->GetAuthGameMode<AMenuGameMode>();
	return GameMode ? GameMode->GetMenuWidget() : nullptr;
}
