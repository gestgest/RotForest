// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

class UUserWidget;

/**
 * 메뉴 계열 맵(메인메뉴/직업선택 등) 공용 게임모드.
 * 맵이 시작되면 지정된 위젯을 자동으로 띄우고 마우스 커서 + UI 전용 입력으로 전환한다.
 * → 레벨 블루프린트에서 CreateWidget/AddToViewport/SetInputMode를 손으로 연결할 필요가 없다.
 *
 * 사용법(맵 하나당):
 *  1. 이 클래스의 BP 자식 생성 (예: BP_JobSelectGameMode) → MenuWidgetClass에 띄울 WBP 지정
 *  2. 빈 레벨 저장 → World Settings → GameMode Override = 그 BP
 */
UCLASS()
class ZOMBIEHUNTER1_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();

protected:
	virtual void BeginPlay() override;

private:
	// 맵 시작 시 자동으로 띄울 메뉴 위젯 (WBP_MainMenu / WBP_JobSelect 등)
	UPROPERTY(EditAnywhere, Category = "Menu")
	TSubclassOf<UUserWidget> MenuWidgetClass;

	// 띄운 메뉴 위젯 (GC 방지용 참조)
	UPROPERTY()
	UUserWidget* MenuWidget = nullptr;

public: //get set
	UUserWidget* GetMenuWidget() const { return MenuWidget; }
};
