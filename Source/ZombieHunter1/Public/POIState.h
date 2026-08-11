// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "POIState.generated.h"

USTRUCT()
struct FVillageState
{
    GENERATED_BODY()


    UPROPERTY()
	int32 PaidMoney = 0;

    //발판 완성 비용
    UPROPERTY()
	int32 MaxMoney = 5;

    UPROPERTY()
	bool bConsumed = false;
};

USTRUCT()
struct FZombieVillageState 
{
    GENERATED_BODY()

    UPROPERTY()
	bool bBossKilled = false;
};

USTRUCT()
struct FPOIStateStore
{
    GENERATED_BODY()

public:
    // 창구는 그대로 넷 — 바깥은 맵이 둘인 걸 모른다.
    // 좌표는 항상 POI의 "중심 청크" 좌표다 (FPOIInfo::CenterChunk).

    /** 발판 상태를 덮어쓴다. 기본값이면 저장을 건너뛸지는 호출자(생성기)가 판단한다 —
     *  기본값 기준은 발판 BP의 CDO라 여기선 알 수 없기 때문. */
    void SavePad(const FIntPoint& Center, int32 Paid, int32 Max, bool bConsumed);

    /** 저장된 발판 상태가 있으면 Out에 채우고 true. 없으면 Out은 건드리지 않는다. */
    bool TryGetPad(const FIntPoint& Center, FVillageState& Out) const;

    void MarkBossKilled(const FIntPoint& Center);
    bool IsBossKilled(const FIntPoint& Center) const;
private:
    UPROPERTY() TMap<FIntPoint, FVillageState>       Villages;
    UPROPERTY() TMap<FIntPoint, FZombieVillageState> ZombieVillages;
};
