// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathPanelWidget.generated.h"

class UMainButton;

/**
 * 사망 패널(디아블로식 "YOU DIED" 화면)의 C++ 베이스 — WBP_DeathPanel의 부모 클래스.
 * 패널의 생김새(어두운 배경/문구/레이아웃)는 WBP 디자이너에서 만들고,
 * 로직(메인메뉴 버튼 클릭 → 맵 이동)은 여기가 담당한다.
 *
 * 에디터 셋업:
 *  1. WBP_DeathPanel의 Class Settings → Parent Class = DeathPanelWidget
 *  2. 안의 버튼 이름을 MainMenuButton으로 (BindWidget이라 이름이 틀리면 WBP 컴파일 에러로 바로 잡힌다)
 *  3. BP_Canvas에 배치한 이 위젯 인스턴스의 이름은 DeathPanel (UMyCanvas가 그 이름으로 찾아 켜고 끈다)
 */
UCLASS()
class ZOMBIEHUNTER1_API UDeathPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 누르면 메인메뉴 맵으로 돌아가는 버튼. 필수(BindWidget) — 없으면 WBP 컴파일 에러. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UMainButton* MainMenuButton;

	/** MainMenuButton이 여는 맵 이름. WBP 디폴트에서 바꿀 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FName MainMenuLevelName = TEXT("MainMenu");

protected:
	virtual void NativeConstruct() override;

private:
	/** MainMenuButton 클릭 → 메인메뉴 맵으로. (OnClicked 동적 바인딩용 — UFUNCTION 필수) */
	UFUNCTION()
	void OnMainMenuClicked();
};
