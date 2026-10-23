#include "Zones/MoneyPadZone.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h"

bool AMoneyPadZone::TryFillOnce(AMyPlayer* Player, int32& OutAmount)
{
	// 돈 소비량(default : 1) 과 남은 돈
	const int32 Payment = FMath::Min(MoneyPerPayment, GetRemainingAmount());
	
	// 게이지가 꽉 찬 경우. GetRemainingAmount가 0
	if (Payment <= 0)
	{
		return false;
	}

	// 돈 소비
	if (!Player->TrySpendMoney(Payment))
	{
		// 실패
		// 실패 사운드

		const FText Msg = FText::FromString(FString::Printf
			(TEXT("돈이 부족합니다. (%d 금화 필요)"), MoneyPerPayment)
		);

		// 실패 메세지가 나온다면
		if (NotifyFailOnce(Player, Msg))
		{
			UGameplayStatics::PlaySoundAtLocation(this, InsufficientFundsSound, GetActorLocation());
		}
		
		return false;
	}

	OutAmount = Payment;
	return true;
}
