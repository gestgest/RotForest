// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DeathPanelWidget.h"
#include "UI/MainButton.h"
#include "Kismet/GameplayStatics.h"

void UDeathPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddUniqueDynamic(this, &UDeathPanelWidget::OnMainMenuClicked);
	}
}

// 메인메뉴로 — 레벨을 통째로 다시 열므로 적/코인/플레이어 상태는 자동으로 정리된다.
void UDeathPanelWidget::OnMainMenuClicked()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
