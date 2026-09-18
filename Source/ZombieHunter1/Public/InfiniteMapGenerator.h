// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POIState.h" 
#include "InfiniteMapGenerator.generated.h"

class UStaticMesh;
class UMaterialInterface;
class AStaticMeshActor;
class APawn;
class ANavMeshBoundsVolume;
class ACompanion;
class AVillager;

USTRUCT()
struct FMapChunk // 한 청크가 스폰한 액터 묶음 
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
};

// POI(특수 지역) 종류
enum class EPOIType : uint8
{
	Village,		// 마을
	ZombieVillage,	// 역병마을
};

// 리전 하나의 POI 정보
struct FPOIInfo
{
	bool bHasPOI = false;
	FIntPoint CenterChunk = FIntPoint::ZeroValue;	// POI 중심 청크 좌표 (리전 로컬 아님, 전역 청크 좌표)
	EPOIType Type = EPOIType::Village;
	bool bIsCenter = false;	  //질의한 청크가 POI 중심 청크인지
};

// 플레이어를 따라다니며 주변 청크를 동적으로 생성/제거하는 무한 맵 생성기.
UCLASS()
class ZOMBIEHUNTER1_API AInfiniteMapGenerator : public AActor
{
	GENERATED_BODY()

public:
	AInfiniteMapGenerator();
	virtual void Tick(float DeltaTime) override;


private:
	void UpdateChunks(const FIntPoint& Center);
	void GenerateChunk(const FIntPoint& Coord); //핵심
	void SpawnObstacles(FRandomStream& Stream, FMapChunk& Chunk, FVector Origin, bool bIsPOIChunk);
	void UnloadChunk(const FIntPoint& Coord);

protected:
	virtual void BeginPlay() override;

	//[Generate 함수]
	//청크가 생성될때 실행된다.
	void SetupFloor(const FVector& Center, FMapChunk& Chunk, FPOIInfo & POI, bool bIsPOIChunk);
	void SpawnFog(const FVector& Center, FMapChunk& Chunk);
	void SetupVillage(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream);
	void SpawnVillageGuards(const FVector& Center, FMapChunk& Chunk);
	void SpawnVillagers(const FVector& Center, FMapChunk& Chunk);
	void SetupZombieVillage(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream);

	// 마을 외곽 구조물 배치 
	void SpawnVillageStructures(const FVector& Center, FMapChunk& Chunk, FRandomStream& Stream);
	AStaticMeshActor* SpawnObstacleMesh(UStaticMesh* Mesh, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat);

	// [청크]
	// 청크 한 변의 길이(cm) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float ChunkSize = 2000.f;
	
	// 플레이어 기준 몇 청크까지 유지할지 (반경 R → (2R+1)^2 개 로드) =>3이라면 7x7
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 ViewRadiusInChunks = 3;

	// 청크 갱신 주기(초). 매 프레임이 아니라 이 간격으로만 검사
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 GlobalSeed = 1337;

	// [바닥]
	// 바닥 타일 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UStaticMesh> FloorMesh;

	// 바닥에 덮어씌울 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	// FloorMesh의 원본 XY 한 변 길이(cm). 엔진 큐브 계열은 보통 100
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorMeshBaseSize = 100.f;

	// 바닥 두께(cm). 윗면이 Z=0에 오도록 배치됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorThickness = 20.f;

	// [장애물]
	// 장애물 후보 메시들 (랜덤으로 골라 배치) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	TArray<TObjectPtr<UStaticMesh>> ObstacleMeshes;

	// 장애물 메시의 원본 한 변 길이(cm). 바닥 위에 앉히기 위한 높이 계산용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMeshBaseSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	int32 MinObstaclesPerChunk = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	int32 MaxObstaclesPerChunk = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMinScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ObstacleMaxScale = 2.0f;

	// 장애물이 청크 가장자리에서 떨어질 여백(cm) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	float ChunkEdgeMargin = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Fog")
	TSubclassOf<AActor> FogClass;

	//안개 높이 cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Fog")
	float FogSizeHeight = 10.f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Fog")
	float FogBaseSize = 100.f;

	// [POI] 특수지역
	// 리전 한 변의 청크 수. 리전마다 최대 1개의 POI가 배치된다.  => 자세한 내용은 노션 개발문서 참고
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "2"))
	int32 RegionSizeInChunks = 8;

	// POI 한 변의 청크 수 (예: 3이면 3×3 청크 = 60×60m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "1"))
	int32 POISizeInChunks = 3;

	// 리전에 POI가 생길 확률(0~1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float POIChance = 0.8f;

	// POI가 마을일 확률 (나머지는 좀비마을).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VillageRatio = 0.8f;

	// 마을 청크의 바닥 머티리얼. 기본값: MI_Solid_Blue (파란 바닥 = 마을)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> VillageFloorMaterial;

	// 좀비마을 청크의 바닥 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> ZombieVillageFloorMaterial;

	//마을 중심 청크에 스폰할 발판 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AActor> VillagePadClass;

	//마을 경비병 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<ACompanion> VillageGuardClass;

	// 마을 중심 청크에 배치할 경비병 수.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "4"))
	int32 VillageGuardCount = 2;

	// 마을 주민(비전투 NPC) 클래스.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AVillager> VillagerClass;

	// 마을 중심 청크에 배치할 주민 수.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "6"))
	int32 VillagerCount = 3;

	// 마을 외곽 링에 놓을 건축 구조물 후보 (랜덤)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TArray<TObjectPtr<UStaticMesh>> VillageStructureMeshes;

	// 외곽 링 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "6"))
	int32 VillageStructureCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AActor> BossClass;

	// [디버깅 변수]
	// 켜면 POI 청크 생성 시 경계 박스(마을=초록, 좀비마을=빨강)와 로그를 남긴다. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI|Debug")
	bool bDebugDrawPOI = true;

	//켜면 청크 생성/갱신을 전부 멈춤. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Debug")
	bool debugDisableGeneration = false;

private:
	
	UPROPERTY()
	TMap<FIntPoint, FMapChunk> LoadedChunks; // 현재 로드된 청크들 (좌표 → 스폰 액터들)

	UPROPERTY()
	TObjectPtr<APawn> TrackedPawn;

	UPROPERTY() 
	TObjectPtr<ANavMeshBoundsVolume> NavBoundsVolume; // 따라다닐 NavMeshBoundsVolume (BeginPlay에서 레벨에서 찾아 캐시)

	FIntPoint LastPlayerChunk = FIntPoint(MAX_int32, MAX_int32);
	float TimeSinceUpdate = 0.f;
	bool bHasGenerated = false;


	// NavMeshBoundsVolume를 플레이어 위치로 옮기고 내비 시스템에 갱신을 통지
	void UpdateNavBoundsToPlayer();

	FIntPoint WorldToChunk(const FVector& WorldLocation) const;

	// 청크 좌표 → 소속 리전 좌표 (음수 좌표도 올바르게 내림 나눗셈)
	FIntPoint ChunkToRegion(const FIntPoint& ChunkCoord) const;

	// 이 리전의 POI 정보를 시드 해시로 계산한다. 스폰/검사 없음 — 순수 계산이라 항상 같은 답.
	FPOIInfo GetPOIForRegion(const FIntPoint& RegionCoord) const;

	// 이 청크가 POI 발자국(중심 ± 반경) 안이면 true를 반환하고 OutInfo를 채운다.
	bool GetPOIAtChunk(const FIntPoint& ChunkCoord, FPOIInfo& OutInfo) const;

	// POI 발자국 반경(청크 수). 발자국 한 변 = 2R+1. 리전을 벗어나지 않게 제한된 값
	int32 GetPOIRadiusInChunks() const;

	/** 발판 상태가 기본값과 다르면 POIStates에 저장 (청크가 죽는 유일한 출구인 UnloadChunk에서 호출).
	 *  기본값 그대로면 저장 스킵 — 시드가 재생성하는 값은 기억할 필요 없다(delta만 저장). */
	void SavePadStateIfChanged(const FIntPoint& Coord, class AMoneyPadZone* Pad);

	/** POI 중심 청크 좌표 → 이번 판의 상태. 청크 언로드 시 저장하고 재생성 시 복원한다.
	 *  기본값과 같은 상태는 저장하지 않으므로 "플레이어가 손댄 POI"만큼만 자란다. */
	UPROPERTY()
	FPOIStateStore POIStateStore;


public:
	// 월드 좌표가 "마을" 발자국 안인지 (적 스폰 제외 등 외부 질의용. 좀비마을은 해당 안 됨)
	bool IsLocationInVillage(const FVector& WorldLocation) const;

	// 좀비마을 보스가 죽었음을 기록한다  => ABoss::OnDeath가 호출
	void MarkBossKilled(const FIntPoint& CenterChunk);

	// 런타임에 생긴 액터(보스 전리품 등)를 청크 소유물로 등록한다.
	bool RegisterChunkActor(const FIntPoint& ChunkCoord, AActor* Actor);
};
