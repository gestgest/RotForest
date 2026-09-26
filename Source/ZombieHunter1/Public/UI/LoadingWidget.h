#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;


UCLASS()
class ZOMBIEHUNTER1_API ULoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* LoadingBar;

	// 없어도 괜찮다
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional) )
	UTextBlock* PercentText;

protected:
	// UI의 Tick
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
