#include "POIState.h"

// POI 상태 저장소 구현.
// 규칙 하나만 지킨다: 맵 두 개(Villages / ZombieVillages)는 이 파일 밖으로 새어나가지 않는다.
// 바깥(생성기, 보스)은 아래 네 함수만 보고, 저장 구조가 바뀌어도 호출부는 그대로다.

void FPOIStateStore::SavePad(const FIntPoint& Center, int32 Paid, int32 Max, bool bConsumed)
{
	FVillageState& State = Villages.FindOrAdd(Center);

	// MaxMoney는 게이지 계산에서 분모로 쓰인다(Progress = Paid / Max) — 0이 들어오면 복원 시 0으로 나눈다.
	State.MaxMoney = FMath::Max(1, Max);
	State.PaidMoney = FMath::Clamp(Paid, 0, State.MaxMoney);
	State.bConsumed = bConsumed;
}

bool FPOIStateStore::TryGetPad(const FIntPoint& Center, FVillageState& Out) const
{
	if (const FVillageState* Found = Villages.Find(Center))
	{
		Out = *Found;
		return true;
	}

	return false;
}

void FPOIStateStore::MarkBossKilled(const FIntPoint& Center)
{
	ZombieVillages.FindOrAdd(Center).bBossKilled = true;
}

bool FPOIStateStore::IsBossKilled(const FIntPoint& Center) const
{
	const FZombieVillageState* Found = ZombieVillages.Find(Center);

	// 기록이 아예 없으면 = 아직 안 잡음. Find 결과가 null인 것과 false인 것을 같게 취급한다.
	return Found != nullptr && Found->bBossKilled;
}
