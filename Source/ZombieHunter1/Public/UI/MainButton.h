#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMainButtonClicked);

// 메뉴 버튼 공용 위젯 — WBP_Button의 부모 클래스
UCLASS()
class ZOMBIEHUNTER1_API UMainButton : public UUserWidget
{
	GENERATED_BODY()

public:
	// 버튼에 표시할 글자
	UPROPERTY(EditAnywhere, Category = "RFButton")
	FText Label;

	UPROPERTY(BlueprintAssignable, Category = "RFButton")
	FOnMainButtonClicked OnClicked;

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* InnerButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Label;

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

private:
	UFUNCTION()
	void HandleClicked();
};
