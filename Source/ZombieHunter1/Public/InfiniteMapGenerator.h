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

/** 한 청크가 스폰한 액터 묶음 (언로드 시 한 번에 Destroy) */
USTRUCT()
struct FMapChunk
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
};

/** POI(특수 지역) 종류 */
enum class EPOIType : uint8
{
	Village,		// 마을 (발판/NPC 등 — 후속 단계에서 채움)
	ZombieVillage,	// 좀비마을 (적 밀집 + 보상 — 후속 단계에서 채움)
};

/** 리전 하나의 POI 정보. 시드 해시로만 결정되므로 어느 청크에서 계산해도 항상 같은 답이 나온다. */
struct FPOIInfo
{
	bool bHasPOI = false;
	FIntPoint CenterChunk = FIntPoint::ZeroValue;	// POI 중심 청크 좌표 (리전 로컬 아님, 전역 청크 좌표)
	EPOIType Type = EPOIType::Village;
	bool bIsCenter = false;	// GetPOIAtChunk가 채움: 질의한 청크가 POI 중심 청크인지 (건물/NPC 스폰은 중심에서 한 번만)
};

// 플레이어를 따라다니며 주변 청크를 동적으로 생성/제거하는 무한 맵 생성기.
// 레벨에 하나 배치하고 Details 패널에서 바닥/장애물 메시를 지정해 사용한다.

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

	//////////////////////////////////////////////////////////////////////////┐
	//Generate 함수
	//청크가 생성될때 실행된다.
	void SetupFloor(const FVector& Center, FMapChunk& Chunk, FPOIInfo & POI, bool bIsPOIChunk);
	void SpawnFog(const FVector& Center, FMapChunk& Chunk);
	void SetupVillege(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream);
	void SpawnVillageGuards(const FVector& Center, FMapChunk& Chunk);
	void SpawnVillagers(const FVector& Center, FMapChunk& Chunk);
	void SetupZombieVillege(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream);

	// 마을 외곽 링(고정 슬롯 6곳)에 구조물 배치 — 스폰존 방향과 그 반대쪽은 비워서 길처럼 보이게 함.
	//  슬롯 좌표는 고정이고, 슬롯당 메시 선택/스케일만 청크 시드로 살짝 흔든다.
	void SpawnVillageStructures(const FVector& Center, FMapChunk& Chunk, FRandomStream& Stream);
	AStaticMeshActor* SpawnObstacleMesh(UStaticMesh* Mesh, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat);
	//////////////////////////////////////////////////////////////////////////┘














	//////////////////////////////////////////////////////////////////////////┐
	// 청크 기본 설정
	// 청크 한 변의 길이(cm) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float ChunkSize = 2000.f;

	//7 7?
	/** 플레이어 기준 몇 청크까지 유지할지 (반경 R → (2R+1)^2 개 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 ViewRadiusInChunks = 3;

	/** 청크 갱신 주기(초). 매 프레임이 아니라 이 간격으로만 검사 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float UpdateInterval = 0.25f;

	/** 전역 시드. 같은 시드+같은 청크 좌표면 항상 동일하게 생성됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 GlobalSeed = 1337;
	//////////////////////////////////////////////////////////////////////////┘





	//////////////////////////////////////////////////////////////////////////┐
	// 바닥
	// 바닥 타일 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UStaticMesh> FloorMesh;

	// 바닥에 덮어씌울 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	/** FloorMesh의 원본 XY 한 변 길이(cm). 엔진 큐브 계열은 보통 100 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorMeshBaseSize = 100.f;

	/** 바닥 두께(cm). 윗면이 Z=0에 오도록 배치됨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Floor")
	float FloorThickness = 20.f;
	//////////////////////////////////////////////////////////////////////////┘





	//////////////////////////////////////////////////////////////////////////┐
	// 장애물
	// 장애물 후보 메시들 (랜덤으로 골라 배치) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Obstacles")
	TArray<TObjectPtr<UStaticMesh>> ObstacleMeshes;

	/** 장애물 메시의 원본 한 변 길이(cm). 바닥 위에 앉히기 위한 높이 계산용 */
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
	//////////////////////////////////////////////////////////////////////////┘






	//////////////////////////////////////////////////////////////////////////┐
	// POI (마을/좀비마을 — 특수 지역)
	// 청크를 리전(RegionSizeInChunks²) 단위로 묶고, 리전마다 시드 해시로 최대 1개의 POI를 정한다.

	//그러니까 8x8청크에 하나의 리전 존재
	/** 리전 한 변의 청크 수. 리전마다 최대 1개의 POI가 배치된다. (8 × ChunkSize 2000 = 160m마다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "2"))
	int32 RegionSizeInChunks = 8;

	/** POI 한 변의 청크 수 (예: 3이면 3×3 청크 = 60×60m). 홀수 권장 — 짝수는 아래 홀수로 내림.
		*  리전보다 크게 잡으면 리전에 들어가는 최대 크기로 자동 축소된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "1"))
	int32 POISizeInChunks = 3;

	/** 리전에 POI가 생길 확률(0~1). 시작 리전(0,0)은 이 값과 무관하게 항상 마을이 보장된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float POIChance = 0.8f;

	/** POI가 마을일 확률(나머지는 좀비마을). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VillageRatio = 0.8f;

	//여기서 부턴 오브젝트
	/** 마을 청크의 바닥 머티리얼. 기본값: MI_Solid_Blue (파란 바닥 = 마을) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> VillageFloorMaterial;

	/** 좀비마을 청크의 바닥 머티리얼. 기본값: MI_PrototypeGrid_TopDark (어두운 바닥 = 좀비마을) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|POI")
	TObjectPtr<UMaterialInterface> ZombieVillageFloorMaterial;

	/** 마을 중심 청크에 스폰할 발판 클래스 (기본: BP_CompanionSpawnZone — 랜덤 직업 동료 소환). 비우면 발판 없이 바닥만 깐다.
	 *  청크 액터 묶음에 들어가므로 언로드 시 함께 제거되지만, 게이지 진행도는 FPOIStateStore로 보존된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AActor> VillagePadClass;

	/** 마을 경비병 클래스 (기본: BP_Companion — 경비 모드로 스폰됨). 비우면 경비병 없음.
	 *  언로드 시 제거되고 재방문 시 풀피로 재생성된다 (경비병 생사 영속은 후속 과제). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<ACompanion> VillageGuardClass;

	/** 마을 중심 청크에 배치할 경비병 수. 발판 주변 고정 자리에 선다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "4"))
	int32 VillageGuardCount = 2;

	/** 마을 주민(비전투 NPC) 클래스. BP_Villager(부모: Villager)를 만들어 지정 — 비우면 주민 없음.
	 *  마을 중심 주변을 배회하며, 언로드 시 제거되고 재방문 시 재생성된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AVillager> VillagerClass;

	/** 마을 중심 청크에 배치할 주민 수. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "6"))
	int32 VillagerCount = 3;

	/** 마을 외곽 링에 놓을 건축 구조물 후보("집" 대용 — 네크로폴리스 팩의 납골당/예배당류 추천: SM_crypt_small_01/02, SM_chapel_01).
	 *  슬롯마다 이 중 하나를 랜덤으로 골라 배치. 비어있으면 구조물 없이 스킵. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TArray<TObjectPtr<UStaticMesh>> VillageStructureMeshes;

	/** 외곽 링 슬롯 6개 중 앞에서부터 몇 개나 채울지. 낮추면 구조물 빈도가 줄어든다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI", meta = (ClampMin = "0", ClampMax = "6"))
	int32 VillageStructureCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI")
	TSubclassOf<AActor> BossClass;
	//////////////////////////////////////////////////////////////////////////┘



	//////////////////////////////////////////////////////////////////////////┐
	// 디버깅
	// 켜면 POI 청크 생성 시 경계 박스(마을=초록, 좀비마을=빨강)와 로그를 남긴다. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|POI|Debug")
	bool bDebugDrawPOI = true;

	//켜면 청크 생성/갱신을 전부 멈춤. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Debug")
	bool debugDisableGeneration = false;
	//////////////////////////////////////////////////////////////////////////┘



private:
	// 현재 로드된 청크들 (좌표 → 스폰 액터들)
	UPROPERTY()
	TMap<FIntPoint, FMapChunk> LoadedChunks;

	UPROPERTY()
	TObjectPtr<APawn> TrackedPawn;

	/** 따라다닐 NavMeshBoundsVolume (BeginPlay에서 레벨에서 찾아 캐시) */
	UPROPERTY()
	TObjectPtr<ANavMeshBoundsVolume> NavBoundsVolume;

	float TimeSinceUpdate = 0.f;
	bool bHasGenerated = false;
	FIntPoint LastPlayerChunk = FIntPoint(MAX_int32, MAX_int32);

	/** NavMeshBoundsVolume를 플레이어 위치로 옮기고 내비 시스템에 갱신을 통지 */
	void UpdateNavBoundsToPlayer();

	FIntPoint WorldToChunk(const FVector& WorldLocation) const;






	/** 청크 좌표 → 소속 리전 좌표 (음수 좌표도 올바르게 내림 나눗셈) */
	FIntPoint ChunkToRegion(const FIntPoint& ChunkCoord) const;

	/** 이 리전의 POI 정보를 시드 해시로 계산한다. 스폰/검사 없음 — 순수 계산이라 항상 같은 답. */
	FPOIInfo GetPOIForRegion(const FIntPoint& RegionCoord) const;

	/** 이 청크가 POI 발자국(중심 ± 반경) 안이면 true를 반환하고 OutInfo를 채운다. */
	bool GetPOIAtChunk(const FIntPoint& ChunkCoord, FPOIInfo& OutInfo) const;

	/** POI 발자국 반경(청크 수). 발자국 한 변 = 2R+1. 리전을 벗어나지 않게 제한된 값 */
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

	/** 좀비마을 보스가 죽었음을 기록한다 (ABoss::OnDeath가 호출).
	 *  청크가 언로드됐다 재생성돼도 이 마을에는 보스가 다시 서지 않는다. */
	void MarkBossKilled(const FIntPoint& CenterChunk);
};
