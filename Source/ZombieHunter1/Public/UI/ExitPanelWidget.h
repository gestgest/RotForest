
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExitPanelWidget.generated.h"

class UButton;

//나가기 버튼
UCLASS()
class ZOMBIEHUNTER1_API UExitPanelWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly, meta= (BindWidget))
	UButton *ContinueButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* ExitButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit")
	TSoftObjectPtr<UWorld> MainMenuLevel;

protected:
	virtual void NativeConstruct() override;
private:
	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnExitClicked();
};
