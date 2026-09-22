#include "Zones/MoneyPadZone.h"
#include "Characters/MyPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

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
		UGameplayStatics::PlaySoundAtLocation(this, InsufficientFundsSound, GetActorLocation());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(7001, 1.0f, FColor::Red,
				FString::Printf(TEXT("[Zone] 돈 부족! (%d원 필요)"), MoneyPerPayment));
		}
		return false;
	}

	OutAmount = Payment;
	return true;
}
