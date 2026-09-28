// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LoadingWidget.h" // 자기자신 헤더
#include "LevelLoaderSubsystem.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


//MyGeometry 화면 정보?
void ULoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UGameInstance* GI = GetGameInstance();
	const ULevelLoaderSubsystem* Loader = GI ? GI->GetSubsystem<ULevelLoaderSubsystem>() : nullptr;
	if (!Loader)
	{
		return;
	}

	const float Progress = Loader->GetProgress();

	if (LoadingBar)
	{
		LoadingBar->SetPercent(Progress);
	}
	if (PercentText)
	{
		PercentText->SetText(FText::AsPercent(Progress));   // => 0.42 → "42%"
	}

}
