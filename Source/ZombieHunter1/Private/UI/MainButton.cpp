#include "UI/MainButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMainButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (InnerButton)
	{
		InnerButton->OnClicked.AddUniqueDynamic(this, &UMainButton::HandleClicked);
	}
}

void UMainButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
}

void UMainButton::HandleClicked()
{
	OnClicked.Broadcast();
}
