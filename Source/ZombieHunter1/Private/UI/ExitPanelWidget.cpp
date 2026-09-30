// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ExitPanelWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"



void UExitPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		// 블루프린트 함수는 Dynamic을 씀
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UExitPanelWidget::OnContinueClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddUniqueDynamic(this, &UExitPanelWidget::OnExitClicked);
	}
}

void UExitPanelWidget::OnContinueClicked()
{
	UGameplayStatics::SetGamePaused(this, false); // 멈춤 풀기
	SetVisibility(ESlateVisibility::Collapsed); // 아예 버튼 안 보이게
}

void UExitPanelWidget::OnExitClicked()
{
	UGameplayStatics::SetGamePaused(this, false);

	// map 로드
	if (!MainMenuLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel);
	}
}
