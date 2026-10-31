#include "InfiniteMapGenerator.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"

#include "Kismet/GameplayStatics.h"

#include "Materials/MaterialInterface.h"

#include "UObject/ConstructorHelpers.h"

#include "NavigationSystem.h" // 런타임 스폰한 바닥을 NavMesh에 반영

#include "NavMesh/NavMeshBoundsVolume.h" // 플레이어 따라 옮길 나비 경계 볼륨

#include "DrawDebugHelpers.h" // POI 청크 디버그 박스

#include "Zones/CompanionSpawnZone.h"
#include "Zones/MoneyPadZone.h" // 마을 발판 상태 저장/복원

#include "Characters/GuardCharacter.h" // 마을 경비병
#include "Characters/Villager.h" // 마을 주민 (비전투 배회 NPC)
#include "Characters/Boss.h" 

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"

//초기
AInfiniteMapGenerator::AInfiniteMapGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));


	// 바닥 머티리얼(있으면): 프로젝트의 프로토타입 그리드
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMatFinder(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
	if (GridMatFinder.Succeeded())
	{
		FloorMaterial = GridMatFinder.Object;
	}

	// POI 바닥 머티리얼: 마을=파랑, 좀비마을=어두운 그리드 (에디터 작업 없이 바로 구분되게 기본 지정) => 스킨 가져오기
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VillageMatFinder(TEXT("/Game/LevelPrototyping/Materials/MI_Solid_Blue.MI_Solid_Blue"));
	if (VillageMatFinder.Succeeded())
	{
		VillageFloorMaterial = VillageMatFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ZombieVillageMatFinder(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));
	if (ZombieVillageMatFinder.Succeeded())
	{
		ZombieVillageFloorMaterial = ZombieVillageMatFinder.Object;
	}
}

void AInfiniteMapGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	if (GlobalSeed == 0)
	{
		GlobalSeed = FMath::RandRange(1, MAX_int32);
	}

	UE_LOG(LogTemp, Log, TEXT("[Map] GlobalSeed = %d"), GlobalSeed);
	// [디버그] 켜져 있으면 초기 생성도 안 함 (맵 없이 시작해 핑 비교용)
	if (debugDisableGeneration)
	{
		return;
	}
	SpawnFog();

	//플레이어를 가져와라
	TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (TrackedPawn)
	{
		const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation()); //벡터값 가져오기

		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;
		UpdateFogToPlayer(Center);
	}

	//플레이어 추격
	// NavMeshBoundsVolume를 레벨에서 찾아 캐시하고, 플레이어 위치로 한 번 맞춰둔다.
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, ANavMeshBoundsVolume::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		NavBoundsVolume = Cast<ANavMeshBoundsVolume>(Found[0]);

		// 런타임에 SetActorLocation으로 옮기려면 브러시(루트) 컴포넌트가 Movable이어야 한다.
		// (기본은 Static이라 "무버블이어야 합니다" 에러가 나고 이동이 무시됨)
		if (NavBoundsVolume && NavBoundsVolume->GetRootComponent())
		{
			NavBoundsVolume->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		}
	}
	UpdateNavBoundsToPlayer();
}

void AInfiniteMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// [디버그] 맵 생성이 핑 원인인지 격리: 켜면 청크 갱신을 통째로 건너뜀
	if (debugDisableGeneration)
	{
		return;
	}

	TimeSinceUpdate += DeltaTime;
	if (TimeSinceUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceUpdate = 0.f;

	//BeginPlay보다 빠르게 Tick 함수가 호출되는 경우 대비
	if (!TrackedPawn)
	{
		TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!TrackedPawn)
		{
			return;
		}
	}

	const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation());
	if (!bHasGenerated || Center != LastPlayerChunk) //전 청크와 내 청크가 다르다면
	{
		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;

		// 청크가 바뀐 시점에만(=플레이어가 한 청크 이동) NavMesh 경계도 플레이어로 따라 옮긴다.
		// 매 프레임이 아니라 이 시점에만 해서 내비 리빌드 부담을 줄인다.
		UpdateNavBoundsToPlayer();
		UpdateFogToPlayer(Center);
	}
}




//플레이어를 추격하는 nav mesh
void AInfiniteMapGenerator::UpdateNavBoundsToPlayer()
{
	if (!NavBoundsVolume || !TrackedPawn)
	{
		return;
	}

	// 볼륨을 플레이어 XY로 옮긴다(높이는 유지 — 점프해도 경계가 위아래로 안 흔들리게).
	FVector Loc = TrackedPawn->GetActorLocation();
	Loc.Z = NavBoundsVolume->GetActorLocation().Z;
	NavBoundsVolume->SetActorLocation(Loc);

	// 핵심: 위치만 바꾸면 NavMesh가 새 위치에 안 깔린다. 내비 시스템에 "경계 바뀜"을 통지해야
	// 옮긴 영역에 NavMesh가 (인보커 기준으로) 다시 생성된다.
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->OnNavigationBoundsUpdated(NavBoundsVolume);
	}
}

//내 위치의 청크 반환
FIntPoint AInfiniteMapGenerator::WorldToChunk(const FVector& WorldLocation) const
{
	const float Size = FMath::Max(1.f, ChunkSize);
	return FIntPoint(FMath::FloorToInt(WorldLocation.X / Size),
		FMath::FloorToInt(WorldLocation.Y / Size));
}

void AInfiniteMapGenerator::UpdateChunks(const FIntPoint& Center)
{
	// 1) 필요한 청크 집합 계산 7x7 
	TSet<FIntPoint> needed;
	needed.Reserve((2 * ViewRadiusInChunks + 1) * (2 * ViewRadiusInChunks + 1));
	for (int32 dx = -ViewRadiusInChunks; dx <= ViewRadiusInChunks; ++dx)
	{
		for (int32 dy = -ViewRadiusInChunks; dy <= ViewRadiusInChunks; ++dy)
		{
			needed.Add(FIntPoint(Center.X + dx, Center.Y + dy));
		}
	}

	// 2) 범위를 벗어난 청크 언로드
	TArray<FIntPoint> ToUnload;
	for (const TPair<FIntPoint, FMapChunk>& Pair : LoadedChunks)
	{
		if (!needed.Contains(Pair.Key))
		{
			ToUnload.Add(Pair.Key);
		}
	}
	for (const FIntPoint& Coord : ToUnload)
	{
		UnloadChunk(Coord);
	}

	// 3) 새로 들어온 청크 생성
	for (const FIntPoint& Coord : needed)
	{
		if (!LoadedChunks.Contains(Coord))
		{
			GenerateChunk(Coord);
		}
	}
}

//언로드. Destroy로 제거한다.
void AInfiniteMapGenerator::UnloadChunk(const FIntPoint& Coord)
{
	if (FMapChunk* Chunk = LoadedChunks.Find(Coord))
	{
		for (AActor* Actor : Chunk->SpawnedActors)
		{
			// 발판은 Destroy로 상태(넣은 돈)가 사라지므로, 죽기 직전에 POIStates로 대피시킨다
			if (AMoneyPadZone* Pad = Cast<AMoneyPadZone>(Actor))
			{
				SavePadStateIfChanged(Coord, Pad);
			}

			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		LoadedChunks.Remove(Coord);
	}
}

//여기서 장애물, 마을, 좀비마을 
void AInfiniteMapGenerator::GenerateChunk(const FIntPoint& Coord)
{
	FMapChunk Chunk;

	const FVector Origin(Coord.X * ChunkSize, Coord.Y * ChunkSize, 0.f);
	const FVector Center = Origin + FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 0.f);

	// 청크 좌표 + 전역 시드로 결정론적 난수 (같은 청크는 항상 동일하게 재생성)
	const uint32 Hash = HashCombine(GetTypeHash(Coord), static_cast<uint32>(GlobalSeed));
	FRandomStream Stream(static_cast<int32>(Hash));

	// POI 판정: 물리 검사가 아니라 시드 해시 계산이므로 로드 순서와 무관하게 항상 같은 답
	FPOIInfo POI;
	const bool bIsPOIChunk = GetPOIAtChunk(Coord, POI);

	if (!GetWorld())
	{
		return;
	}
	
	SetupFloor(Center, Chunk, POI, bIsPOIChunk);
	SetupVillage(bIsPOIChunk, POI, Center, Chunk, Stream);
	SetupPlagueVillage(bIsPOIChunk, POI, Center, Chunk, Stream);

	//POI : 마을이나 좀비마을 와이어 박스 만듬
	if (bIsPOIChunk && bDebugDrawPOI && POI.bIsCenter)
	{
		const bool bVillage = (POI.Type == EPOIType::Village);
		const int32 FootprintSize = GetPOIRadiusInChunks() * 2 + 1; //사이즈 할당
		const float HalfExtent = ChunkSize * FootprintSize * 0.5f;
		DrawDebugBox(GetWorld(), Center + FVector(0.f, 0.f, 100.f),
			FVector(HalfExtent, HalfExtent, 100.f),
			bVillage ? FColor::Green : FColor::Red, false, 120.f, 0, 8.f);
		UE_LOG(LogTemp, Log, TEXT("[POI] %s generated at chunk (%d, %d), size %dx%d chunks"),
			bVillage ? TEXT("Village") : TEXT("ZombieVillage"), Coord.X, Coord.Y, FootprintSize, FootprintSize);
	}

	TArray<FRoadSegment> RoadSegments;
	GetRoadSegmentsInChunk(Coord, RoadSegments); //여기서 segments 위치 설정
	SpawnRoad(RoadSegments, Chunk);

	SpawnObstacles(Stream, Chunk, Origin, bIsPOIChunk, RoadSegments);
	if (!bIsPOIChunk)
	{
		SpawnGrass(Coord, Chunk, Origin, RoadSegments);
	}
	LoadedChunks.Add(Coord, MoveTemp(Chunk));
}




// 장애물 생성 : 청크 안에 랜덤 배치 — POI 청크는 비워둔다 (마을 콘텐츠 자리)
void AInfiniteMapGenerator::SpawnObstacles(FRandomStream & Stream, FMapChunk & Chunk, FVector Origin, bool bIsPOIChunk, const TArray<FRoadSegment> & RoadSegments)
{
	if (!bIsPOIChunk && ObstacleMeshes.Num() > 0)
	{
		const int32 Count = Stream.RandRange(MinObstaclesPerChunk, FMath::Max(MinObstaclesPerChunk, MaxObstaclesPerChunk));
		for (int32 i = 0; i < Count; ++i)
		{
			UStaticMesh* Mesh = ObstacleMeshes[Stream.RandRange(0, ObstacleMeshes.Num() - 1)];
			//여기에 if문 안하는 이유. 난수 꼬여서 지정된 시드값이어도 장애물이 다를까봐 
			if (!Mesh)
			{
				continue;
			}
			const float LocalX = Stream.FRandRange(ChunkEdgeMargin, ChunkSize - ChunkEdgeMargin);
			const float LocalY = Stream.FRandRange(ChunkEdgeMargin, ChunkSize - ChunkEdgeMargin);
			const float Scale = Stream.FRandRange(ObstacleMinScale, ObstacleMaxScale);
			const float Yaw = Stream.FRandRange(0.f, 360.f);

			// 중심 피벗 메시를 바닥 위에 앉히기 위한 Z 오프셋
			//const float HalfHeight = (ObstacleMeshBaseSize * Scale) * 0.5f;
			const FVector Loc = Origin + FVector(LocalX, LocalY, 0);

			const float RoadClear = RoadWidth * 0.5f + ObstacleMeshBaseSize * Scale * 0.5f;
			bool bOnRoad = false;

			for (const FRoadSegment& RoadSegment : RoadSegments)
			{
				if (FMath::PointDistToSegment(Loc, RoadSegment.Start, RoadSegment.End) < RoadClear)
				{
					bOnRoad = true;
					break;
				}
			}

			if (bOnRoad)
			{
				continue;
			}

			if (AStaticMeshActor* Obstacle = SpawnObstacleMesh(Mesh, Loc, FRotator(0.f, Yaw, 0.f), FVector(Scale), nullptr))
			{
				Chunk.SpawnedActors.Add(Obstacle);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//POI chunk
//////////////////////////////////////////////////////////////////////////////////////////////

/*
*   Tick (0.25초마다)
   └─ UpdateChunks          "플레이어 주변 7×7 청크 유지"
	   └─ GenerateChunk
		   └─ GetPOIAtChunk       "이 청크가 POI 자리야?"
			   ├─ ChunkToRegion       "이 청크는 어느 리전 소속?"
			   └─ GetPOIForRegion     "그 리전의 POI는 어디에 뭐가 있지?"
*/

//static. 단, 이 파일에서만 사용하는 느낌
//멤버 함수가 아니라서 "사용하는 지점보다 위"에 정의돼 있어야 함 (컴파일러는 위→아래 한 번만 읽음)
namespace
{
	// 음수에서도 내림으로 떨어지는 정수 나눗셈 (C++ 기본 나눗셈은 0 방향 절삭이라 -1/8 = 0이 됨)
	int32 FloorDiv(int32 A, int32 B)
	{
		int32 Q = A / B;
		if ((A % B != 0) && ((A < 0) != (B < 0)))
		{
			--Q;
		}
		return Q;
	}
}

//POI 영역 반경(청크). 한 변이 region 크기보다 압도적으로 큰 경우를 막음
//= > 혹시모르는 try문이라고 생각하면 됨
int32 AInfiniteMapGenerator::GetPOIRadiusInChunks() const
{
	const int32 RegionSize = FMath::Max(2, RegionSizeInChunks); //음수나 0 나누기 방지용
	const int32 Radius = (FMath::Max(1, POISizeInChunks) - 1) / 2; // 짝수 크기는 홀수(2R+1)로 내림: 4 → 3×3
	return FMath::Min(Radius, (RegionSize - 1) / 2); //만약 크기를 무식
}

//이 청크가 POI 영역 안에 있는지
bool AInfiniteMapGenerator::GetPOIAtChunk(const FIntPoint& ChunkCoord, FPOIInfo& OutInfo) const
{
	// 중심을 리전 안쪽으로 클램프해 영역이 리전을 못 벗어나므로, 자기 리전만 검사하면 된다
	const FPOIInfo Info = GetPOIForRegion(ChunkToRegion(ChunkCoord));
	
	//POIChance로 안 나올경우
	if (!Info.bHasPOI)
	{
		return false;
	}

	//아무리 리전이 있다해도 내 시야에 안 보일 경우 => 여기서 판별
	const int32 Radius = GetPOIRadiusInChunks();
	if (FMath::Abs(ChunkCoord.X - Info.CenterChunk.X) <= Radius &&
		FMath::Abs(ChunkCoord.Y - Info.CenterChunk.Y) <= Radius)
	{
		OutInfo = Info;
		OutInfo.bIsCenter = (ChunkCoord == Info.CenterChunk);
		return true;
	}
	return false;
}

//월드 좌표가 "마을" 영역 안인지 — 적 스폰 링 등 외부에서 질의 (좀비마을은 적 지역이라 해당 없음)
bool AInfiniteMapGenerator::IsLocationInVillage(const FVector& WorldLocation) const
{
	FPOIInfo Info;
	if (GetPOIAtChunk(WorldToChunk(WorldLocation), Info))
	{
		return Info.Type == EPOIType::Village;
	}
	return false;
}

//청크 => 리전 포지션으로 변환 (15 15 청크를 1 1 region으로 변환)
FIntPoint AInfiniteMapGenerator::ChunkToRegion(const FIntPoint& ChunkCoord) const
{
	const int32 Size = FMath::Max(2, RegionSizeInChunks);
	return FIntPoint(FloorDiv(ChunkCoord.X, Size), FloorDiv(ChunkCoord.Y, Size));
}


//리전
FPOIInfo AInfiniteMapGenerator::GetPOIForRegion(const FIntPoint& RegionCoord) const
{
	const int32 Size = FMath::Max(2, RegionSizeInChunks);

	// 청크용 해시와 값이 겹치지 않게 리전 전용 소금을 섞는다 => 혹시 모르기 때문
	const uint32 Hash = HashCombine(
		HashCombine(GetTypeHash(RegionCoord), static_cast<uint32>(GlobalSeed)), 0x9E3779B9u);
	FRandomStream Stream(static_cast<int32>(Hash));

	FPOIInfo Info;

	// 시작 리전(0,0)은 확률과 무관하게 항상 마을 — 첫 마을 발견을 보장
	const bool bStartRegion = (RegionCoord == FIntPoint::ZeroValue);
	if (!bStartRegion && Stream.FRand() >= POIChance) //시작 리전이 아니고 poi 구역 리전이 아닌지. 
	{
		return Info; // 이 리전엔 POI 없음
	}
	Info.bHasPOI = true;


	// 리전 내 POI 중심 위치. 영역(2R+1)²이 리전 밖으로 삐져나가지 않게 중심을 [R, Size-1-R]에서만 뽑는다.
	// (삐져나가면 옆 리전 청크가 자기 리전만 검사하는 GetPOIAtChunk에서 잘려버림)
	const int32 Radius = GetPOIRadiusInChunks();
	const int32 MaxCell = Size - 1 - Radius;

	//리전 위치 뽑는 코드
	// 시작 리전은 영역 가장자리(중심-R)가 플레이어 시작 청크(0,0)에서 최소 2청크 떨어지게.
	// 영역이 커서 불가능하면 리전에 들어가는 한 최대한 밀어낸다.
	const int32 MinCell = FMath::Min(bStartRegion ? (Radius + 2) : Radius, MaxCell);
	const int32 LocalX = Stream.RandRange(MinCell, MaxCell); 
	const int32 LocalY = Stream.RandRange(MinCell, MaxCell);


	Info.CenterChunk = FIntPoint(RegionCoord.X * Size + LocalX, RegionCoord.Y * Size + LocalY);

	Info.Type = (bStartRegion || Stream.FRand() < VillageRatio)
		? EPOIType::Village : EPOIType::PlagueVillage;
	return Info;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//Village
// 마을 v1: 마을 안의 오브젝트 생성 (무기고, npc 같은거)
void AInfiniteMapGenerator::SetupVillage(bool bIsPOIChunk, FPOIInfo & POI, const FVector Center, FMapChunk & Chunk, FRandomStream & Stream)
{
	if (bIsPOIChunk && POI.bIsCenter && POI.Type == EPOIType::Village && VillagePadClass)
	{
		FActorSpawnParameters PadParams;
		PadParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PadParams.Owner = this;

		// 발판 루트(트리거 박스, 반높이 100)가 바닥 위 Z 0~200을 덮도록 +100에 스폰 => 무기고 같은
		const FVector PadLoc = Center + FVector(0.f, 0.f, 100.f);
		if (AActor* Pad = GetWorld()->SpawnActor<AActor>(VillagePadClass, PadLoc, FRotator::ZeroRotator, PadParams))
		{
#if WITH_EDITOR
			Pad->SetFolderPath(TEXT("Spawned/Map"));
#endif
			Chunk.SpawnedActors.Add(Pad);

			if (ACompanionSpawnZone* CompanionZone = Cast<ACompanionSpawnZone>(Pad))
			{
				const int32 JobIndex = Stream.RandRange(0, static_cast<int32>(EJobType::Healer));
				CompanionZone->SetJobType(static_cast<EJobType>(JobIndex));
			}

			// 상태 복원: 이 마을에 저장된 상태(언로드 때 대피시킨 게이지/비용)가 있으면 새 발판에 주입.
			// 없으면 그대로 둠 — 처음 발견한 마을과 동일한 기본값.
			if (AMoneyPadZone* PadZone = Cast<AMoneyPadZone>(Pad))
			{
				FVillageState Saved;
				if (POIStateStore.TryGetPad(POI.CenterChunk, Saved))
				{
					PadZone->RestorePadState(Saved.PaidMoney, Saved.MaxMoney, Saved.bConsumed);
					UE_LOG(LogTemp, Log, TEXT("[POIState] 복원: 청크(%d, %d) Paid %d / Max %d"),
						POI.CenterChunk.X, POI.CenterChunk.Y, Saved.PaidMoney, Saved.MaxMoney);
				}
			}
		}

		SpawnVillageGuards(Center, Chunk);
		SpawnVillagers(Center, Chunk);
		SpawnVillageStructures(Center, Chunk, Stream);

		// 사운드
		if (AActor* Audio = SpawnDecoActor(VillageAudioClass, Center, FRotator::ZeroRotator))
		{
			Chunk.SpawnedActors.Add(Audio);
		}
	}
}

// 경비병: 발판 주변 고정 자리에 배치. 랜덤 산포가 아니라 고정 레이아웃 — 재생성돼도 항상 같은 그림.
// 리더 없이 스폰하고 경비 모드를 켜서 "자리를 지키다 → 적 오면 교전 → 끝나면 복귀"로 동작.
void AInfiniteMapGenerator::SpawnVillageGuards(const FVector& Center, FMapChunk& Chunk)
{
	if (!VillageGuardClass || VillageGuardCount <= 0)
	{
		return;
	}

	// 발판 대각선 네 모서리 자리 (VillageGuardCount만큼 앞에서부터 사용)
	static const FVector2D GuardPosts[4] = {
		FVector2D(450.f, 450.f), FVector2D(-450.f, -450.f),
		FVector2D(450.f, -450.f), FVector2D(-450.f, 450.f),
	};

	FActorSpawnParameters GuardParams;
	GuardParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GuardParams.Owner = this;

	const int32 Count = FMath::Min(VillageGuardCount, 4);

	for (int32 i = 0; i < Count; i++)
	{
		// 캡슐 반높이(~90)만큼 띄워 바닥 위에 스폰
		const FVector GuardLoc = Center + FVector(GuardPosts[i].X, GuardPosts[i].Y, 92.f);
		AGuardCharacter* Guard = GetWorld()->
			SpawnActor<AGuardCharacter>(VillageGuardClass, GuardLoc, FRotator::ZeroRotator, GuardParams);

		if (!Guard)
		{
			continue;
		}

		Guard->SetHomeLocation(GuardLoc);
#if WITH_EDITOR
		Guard->SetFolderPath(TEXT("Spawned/Map"));
#endif
		Chunk.SpawnedActors.Add(Guard); // 청크와 함께 언로드/재생성
	}
}

// 주민(비전투): 발판과 겹치지 않는 고정 자리에서 시작해 마을 중심 주변을 배회.
// 스폰 위치는 고정이지만 이후엔 각자 알아서 돌아다녀서 마을이 살아 보인다.
void AInfiniteMapGenerator::SpawnVillagers(const FVector& Center, FMapChunk& Chunk)
{
	if (!VillagerClass || VillagerCount <= 0)
	{
		return;
	}

	static const FVector2D VillagerPosts[6] = {
		FVector2D(0.f, 650.f), FVector2D(650.f, 0.f), FVector2D(0.f, -650.f),
		FVector2D(-650.f, 0.f), FVector2D(325.f, -325.f), FVector2D(-325.f, 325.f),
	};

	FActorSpawnParameters VillagerParams;
	VillagerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	VillagerParams.Owner = this;

	const int32 Count = FMath::Min(VillagerCount, 6);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector VillagerLoc = Center + FVector(VillagerPosts[i].X, VillagerPosts[i].Y, 92.f);
		AVillager* Villager = GetWorld()->SpawnActor<AVillager>(
			VillagerClass, VillagerLoc, FRotator::ZeroRotator, VillagerParams);
		if (!Villager)
		{
			continue;
		}

		// 배회 중심은 스폰 지점이 아니라 마을 중심 — 주민들이 마을 전체를 고루 돌아다닌다
		Villager->HomeLocation = Center;
#if WITH_EDITOR
		Villager->SetFolderPath(TEXT("Spawned/Map"));
#endif
		Chunk.SpawnedActors.Add(Villager);
	}
}

// 마을 외곽 링(반경 2400 — 영역 절반 3000 안쪽)에 구조물 6개.
// 스폰존이 있는 북쪽(0°)과 그 반대쪽 남쪽(180°)은 비워서 마을 안팎으로 길이 뚫린 것처럼 보이게 한다.
// 좌표는 경비병/주민처럼 고정 슬롯 — 슬롯당 메시 선택/스케일만 청크 시드로 살짝 흔든다(재생성해도 같은 그림).
void AInfiniteMapGenerator::SpawnVillageStructures(const FVector& Center, FMapChunk& Chunk, FRandomStream& Stream)
{
	if (VillageStructureMeshes.Num() == 0 || VillageStructureCount <= 0)
	{
		return;
	}

	struct FStructureSlot
	{
		FVector2D Pos;
		float FacingYaw; // 중심(발판)을 바라보는 방향
	};
	static const FStructureSlot Slots[6] = {
		{ FVector2D(1697.f,  1697.f), 225.f },
		{ FVector2D(2400.f,     0.f), 180.f },
		{ FVector2D(1697.f, -1697.f), 135.f },
		{ FVector2D(-1697.f,-1697.f),  45.f },
		{ FVector2D(-2400.f,    0.f),   0.f },
		{ FVector2D(-1697.f, 1697.f), 315.f },
	};

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	const int32 Count = FMath::Min(VillageStructureCount, 6);
	for (int32 i = 0; i < Count; ++i)
	{
		const FStructureSlot& Slot = Slots[i];
		UStaticMesh* Mesh = VillageStructureMeshes[Stream.RandRange(0, VillageStructureMeshes.Num() - 1)];
		if (!Mesh)
		{
			continue;
		}

		const float Scale = Stream.FRandRange(0.9f, 1.1f);
		// 건축 프롭은 보통 피벗이 바닥(밑동)에 있다고 가정 — 장애물(나무)과 같은 전제, Z 오프셋 없이 그대로 지면에 심는다
		const FVector Loc = Center + FVector(Slot.Pos.X, Slot.Pos.Y, 0.f);

		if (AStaticMeshActor* Structure = SpawnObstacleMesh(Mesh, Loc, FRotator(0.f, Slot.FacingYaw, 0.f), FVector(Scale), nullptr))
		{
			Chunk.SpawnedActors.Add(Structure);
		}
	}
}


void AInfiniteMapGenerator::SpawnGrass(const FIntPoint& Coord, FMapChunk& Chunk, const FVector& Origin, const TArray<FRoadSegment>& RoadSegments)
{
	UWorld* World = GetWorld();
	if (GrassMeshes.Num() <= 0 || GrassPerChunk <= 0 || !World)
	{
		return;
	}
	
	//GetTypeHash는 뭐 좌표를 큰 숫자로 변환하는 함수
	const uint32 Hash = HashCombine(GetTypeHash(Coord), static_cast<uint32>(GlobalSeed) ^ 0x6A55u);
	FRandomStream Stream(static_cast<int32>(Hash));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AActor* GrassActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!GrassActor)
	{
		return;
	}

	// 유니티의 Transform 같은 설정
	USceneComponent* Root = NewObject<USceneComponent>(GrassActor, TEXT("Root"));
	GrassActor->SetRootComponent(Root);
	Root->RegisterComponent();

#if WITH_EDITOR
	GrassActor->SetFolderPath("Spawned/Grass");
#endif

	// HISM => mesh 덩어리
	TArray<UHierarchicalInstancedStaticMeshComponent*> Hisms;
	for (UStaticMesh* Mesh : GrassMeshes)
	{
		UHierarchicalInstancedStaticMeshComponent* Hism = nullptr;
		if (Mesh)
		{
			Hism = NewObject<UHierarchicalInstancedStaticMeshComponent>(GrassActor);

			Hism->SetMobility(EComponentMobility::Movable);
			Hism->SetStaticMesh(Mesh);
			Hism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Hism->SetCanEverAffectNavigation(false);
			Hism->SetCastShadow(false);

			// GrassCullDistance 만큼 멀어지면 안 그린다.
			Hism->SetCullDistances(
				static_cast<int32>(GrassCullDistance * 0.8f),
				static_cast<int32>(GrassCullDistance));

			Hism->SetupAttachment(Root);
			Hism->RegisterComponent();
		}
		Hisms.Add(Hism);
	}

	// 풀1 { 풀1의 오브젝트, }, 풀2 { 풀2의 오브젝트1, 풀2의 오브젝트2}
	TArray<TArray<FTransform>> Transforms;
	Transforms.SetNum(GrassMeshes.Num());

	const float GrassZ = FloorThickness * 0.5f;
	
	for (int32 i = 0; i < GrassPerChunk; i++)
	{
		const int32 MeshIndex = Stream.RandRange(0, GrassMeshes.Num() - 1);

		const float LocalX = Stream.FRandRange(0.f, ChunkSize);
		const float LocalY = Stream.FRandRange(0.f, ChunkSize);

		const float Scale = Stream.FRandRange(GrassMinScale,GrassMaxScale);
		const float Yaw = Stream.FRandRange(0.f, 360.f); // 회전값

		if (!Hisms[MeshIndex])
		{
			continue;
		}
		const FVector Loc = Origin + FVector(LocalX, LocalY, GrassZ);

		// 도로 판단 변수
		bool bOnRoad = false; 
		for (const FRoadSegment & Seg : RoadSegments)
		{
			// PointDistToSegment
			// 풀과 도로의 수직
			// ㄴ 만약 수직 내렸는데 바깥 선분이면 그냥 Start와의 거리
			// 그 어차피 선 기준으로 도로 폭보다 가까우면 도로안이니까 true
			if (FMath::PointDistToSegment(Loc, Seg.Start, Seg.End) < GrassRoadClear)
			{
				bOnRoad = true;
				break;
			}
		}
		if (bOnRoad)
		{
			continue;
		}
		Transforms[MeshIndex].Add(FTransform(FRotator(0.f, Yaw, 0.f), Loc, FVector(Scale)));
	}
	

	for (int32 i = 0; i < Hisms.Num(); i++)
	{
		if (Hisms[i] && Transforms[i].Num() > 0)
		{
			Hisms[i]->AddInstances(Transforms[i], false);
		}
	}
	Chunk.SpawnedActors.Add(GrassActor);
}
void AInfiniteMapGenerator::SpawnRoad(const TArray<FRoadSegment>& Segments, FMapChunk& Chunk)
{

	// null 체크
	if (!RoadMesh || !GetWorld())
	{
		return;
	}

	// 도로지역 아니면 안그림
	bool bHasOwned = false;
	for (const FRoadSegment& Seg : Segments)
	{
		if (Seg.bOwned)
		{
			bHasOwned = true;
			break;
		}
	}
	if (!bHasOwned)
	{
		return;
	}

	// 소환
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AActor* RoadActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!RoadActor)
	{
		return;
	}

	USceneComponent* Root = NewObject<USceneComponent>(RoadActor, TEXT("Root"));
	RoadActor->SetRootComponent(Root);
	Root->RegisterComponent();

#if WITH_EDITOR
	RoadActor->SetFolderPath(TEXT("Spawned/Road"));
#endif

	const float WidthScale = RoadWidth / FMath::Max(1.f, RoadMeshBaseSize);

	for (const FRoadSegment& Seg : Segments)
	{
		if (!Seg.bOwned)
		{
			continue;
		}

		// 설정
		USplineMeshComponent* Piece = NewObject<USplineMeshComponent>(RoadActor);
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetStaticMesh(RoadMesh);
		if (RoadMaterial)
		{
			Piece->SetMaterial(0, RoadMaterial);
		}

		// 구부리기
		Piece->SetForwardAxis(ESplineMeshAxis::X, false);
		Piece->SetStartAndEnd(Seg.Start, Seg.StartTangent, Seg.End, Seg.EndTangent, false);
		Piece->SetStartScale(FVector2D(WidthScale, 1.f), false); // 폭 설정
		Piece->SetEndScale(FVector2D(WidthScale, 1.f), false); // 폭 설정

		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCanEverAffectNavigation(false);
		Piece->SetCastShadow(false);

		Piece->SetupAttachment(Root); // 부모 설정
		Piece->RegisterComponent(); // 월드에 등록(랜더링)
		Piece->UpdateMesh();
	}

	Chunk.SpawnedActors.Add(RoadActor);
}



// 발판 상태를 POIStates에 저장 (청크가 죽는 유일한 출구인 UnloadChunk에서 호출).
// 기본값(CDO)과 같으면 저장하지 않는다 — 시드가 어차피 그대로 재생성하는 값이라 기억할 필요 없음.
// 덕분에 저장소는 탐색량이 아니라 "플레이어가 실제로 손댄 발판 수"만큼만 자란다.
void AInfiniteMapGenerator::SavePadStateIfChanged(const FIntPoint& Coord, AMoneyPadZone* Pad)
{
	if (!IsValid(Pad))
	{
		return;
	}

	const AMoneyPadZone* Defaults = Pad->GetClass()->GetDefaultObject<AMoneyPadZone>();


	//만약 안 바뀌었으면 그냥 무시.
	const bool bIsDefault =
		Pad->GetFilledAmount() == Defaults->GetFilledAmount() &&
		Pad->GetRequiredAmount() == Defaults->GetRequiredAmount() &&
		Pad->IsConsumed() == Defaults->IsConsumed();

	FVillageState Existing;
	// 기본값이고 기존 저장분도 없으면 스킵. (기존 엔트리가 있으면 갱신해서 낡은 값이 남지 않게 한다)
	if (bIsDefault && !POIStateStore.TryGetPad(Coord, Existing))
	{
		return;
	}


	//todo POIStateStore.SavePad ...?
	// 발판 세 필드만 덮어쓴다 — 같은 엔트리에 들어있는 bBossKilled는 건드리지 않는다.
	// (마을과 좀비마을은 서로 다른 중심 청크라 실제로 섞일 일은 없지만, 그래도 남의 필드는 안 만진다)
	POIStateStore.SavePad(Coord, Pad->GetFilledAmount(), Pad->GetRequiredAmount(), Pad->IsConsumed());

	UE_LOG(LogTemp, Log, TEXT("[POIState] 저장: 청크(%d, %d) Filled %d / Required %d"),
		Coord.X, Coord.Y, Pad->GetFilledAmount(), Pad->GetRequiredAmount());
}

// 보스 처치 기록 — 보스가 죽는 순간 자기 소속 마을 좌표를 들고 여기로 온다.
void AInfiniteMapGenerator::MarkBossKilled(const FIntPoint& CenterChunk)
{
	POIStateStore.MarkBossKilled(CenterChunk);

	UE_LOG(LogTemp, Log, TEXT("[POIState] 좀비마을(%d, %d) 보스 처치 기록 — 재방문해도 다시 스폰되지 않는다"),
		CenterChunk.X, CenterChunk.Y);
}

/** GenerateChunk가 만든 액터가 아니면 SpawnedActors에 안 들어간다.
 *  그런 액터는 여기서 직접 등록해줘야 청크와 함께 정리된다 — 안 하면 아무도 안 치우는 미아가 된다. */
bool AInfiniteMapGenerator::RegisterChunkActor(const FIntPoint& ChunkCoord, AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FMapChunk* Chunk = LoadedChunks.Find(ChunkCoord);
	if (!Chunk)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Chunk] (%d, %d) 청크가 로드돼 있지 않아 %s 등록 실패"),
			ChunkCoord.X, ChunkCoord.Y, *Actor->GetName());
		return false;
	}

	Chunk->SpawnedActors.Add(Actor);
	return true;
}




void AInfiniteMapGenerator::SetupFloor(const FVector & Center, FMapChunk & Chunk, FPOIInfo & POI, bool bIsPOIChunk)
{
	// POI 청크는 전용 머티리얼로 구분 (마을=파랑, 좀비마을=어두움)
	UMaterialInterface* FloorMat = FloorMaterial;
	if (bIsPOIChunk)
	{
		UMaterialInterface* POIMat = (POI.Type == EPOIType::Village) ? VillageFloorMaterial : ZombieVillageFloorMaterial;
		if (POIMat)
		{
			FloorMat = POIMat;
		}
	}
	if (FloorMesh)
	{
		const float Base = FMath::Max(1.f, FloorMeshBaseSize);
		const float XYScale = ChunkSize / Base;
		const float ZScale = FloorThickness / Base;
		const FVector FloorScale(XYScale, XYScale, ZScale);
		const FVector FloorLoc = Center - FVector(0.f, 0.f, FloorThickness * 0.5f);

		//바닥 깔기
		if (AStaticMeshActor* Floor = SpawnObstacleMesh(FloorMesh, FloorLoc, FRotator::ZeroRotator, FloorScale, FloorMat))
		{
			Chunk.SpawnedActors.Add(Floor);
		}

	}
}

void AInfiniteMapGenerator::SpawnFog()
{
	if (bDebugOffFog || !FogClass)
	{
		return;
	}

	FActorSpawnParameters FogParams;
	FogParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FogParams.Owner = this;

	FogActor = GetWorld()->SpawnActor<AActor>(FogClass, FTransform::Identity, FogParams);
	if (!FogActor)
	{
		return;
	}

#if WITH_EDITOR
	FogActor->SetFolderPath(TEXT("Spawned/Map"));
#endif
	if (USceneComponent* FogRoot = FogActor->GetRootComponent())
	{
		FogRoot->SetMobility(EComponentMobility::Movable);
	}

	const float Base = FMath::Max(1.f, FogBaseSize);
	const float CoverSize = ChunkSize * (2 * ViewRadiusInChunks + 1); // 7 x 7
	FogActor->SetActorScale3D(FVector(CoverSize / Base, CoverSize / Base, FogSizeHeight / Base));
}
void AInfiniteMapGenerator::UpdateFogToPlayer(const FIntPoint& CenterChunk)
{
	if (!FogActor)
	{
		return;
	}

	const FVector FogLoc(
		(CenterChunk.X + 0.5f) * ChunkSize,
		(CenterChunk.Y + 0.5f) * ChunkSize,
		FogSizeHeight / 2);
	FogActor->SetActorLocation(FogLoc);
}

//보스 나오는 구역(역병마을) 생성
void AInfiniteMapGenerator::SetupPlagueVillage(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream)
{
	// POI 구역과 역병마을이 아니라면
	if (!bIsPOIChunk || POI.Type != EPOIType::PlagueVillage)
	{
		return;
	}

	SpawnPlagueVillageObjects(Center, Chunk, Stream, POI.bIsCenter);

	// 가운데
	if (POI.bIsCenter)
	{
		SpawnPlagueCandle(Center, Chunk);

		if (AActor* Audio = SpawnDecoActor(PlagueVillageAudioClass, Center, FRotator::ZeroRotator))
		{
			Chunk.SpawnedActors.Add(Audio);
		}
	}
	if (!POI.bIsCenter || !BossClass)
	{
		return;
	}
	
	// 이미 이번 판에 클리어한 좀비마을이면 보스를 다시 세우지 않는다.
	// (없으면 보스를 죽이고 멀리 갔다 오는 것만으로 풀피 보스가 부활한다)
	if (POIStateStore.IsBossKilled(POI.CenterChunk))
	{
		return;
	}

	FActorSpawnParameters VillagerParams;
	VillagerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	VillagerParams.Owner = this;

	// 캡슐 중심 기준이라 반높이만큼 띄워야 바닥에 선다
	float HalfHeight = 100.f;
	if (const ACharacter* BossCDO = BossClass->GetDefaultObject<ACharacter>())
	{
		HalfHeight = BossCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	const FVector SpawnLoc = Center + FVector(0.f, 0.f, HalfHeight + 5.f);

	ABoss* Boss = GetWorld()->SpawnActor<ABoss>(BossClass, SpawnLoc, FRotator::ZeroRotator, VillagerParams);
	if (!Boss)
	{
		// BossClass가 ABoss 계열이 아니거나 스폰이 막힌 경우 — 아래에서 널 역참조로 죽지 않게 막는다
		UE_LOG(LogTemp, Warning, TEXT("[POI] 좀비마을(%d, %d) 보스 스폰 실패 — BossClass 확인 필요"),
			POI.CenterChunk.X, POI.CenterChunk.Y);
		return;
	}

#if WITH_EDITOR
	Boss->SetFolderPath(TEXT("Spawned/Enemies"));
#endif

	// 죽을 때 "어느 마을을 클리어했는지" 스스로 기록할 수 있게 소속을 알려준다
	Boss->SetHome(this, POI.CenterChunk);

	//플레이어 추적 기능.
	Boss->OnAIController();

	Chunk.SpawnedActors.Add(Boss); // 청크와 함께 언로드/재생성
}	

// 소품 생성
void AInfiniteMapGenerator::SpawnPlagueVillageObjects(const FVector& Center, FMapChunk & Chunk, FRandomStream & Stream, bool bIsCenter)
{


	// 작은, 큰 소품
	// SpawnPlagueMeshes(false, Stream, Center, bIsCenter, Chunk); 
	// SpawnPlagueMeshes(true, Stream, Center, bIsCenter, Chunk);
	
	// BP
	//int32 Size = PlagueVillageActorClasses.Num();
	//if (Size == 0)
	//{
	//	return;
	//}

	//// BP 소품 라인
	//for (int32 i = 0; i < PlagueVillageActorPerChunk; i++)
	//{
	//	TSubclassOf<AActor> ActorClass = PlagueVillageActorClasses[Stream.RandRange(0, Size - 1)];
	//	FVector Loc;
	//	if (!PickPlagueLocation(Stream, Center, bIsCenter, Loc))
	//	{
	//		continue;
	//	}
	//	const FRotator Rot(0.f, Stream.FRandRange(0.f, 360.f), 0.f);

	//	if (AActor* Actor = SpawnDecoActor(ActorClass, Loc, Rot))
	//	{
	//		Chunk.SpawnedActors.Add(Actor);
	//	}
	//}
}


// 중간 역병마을 spawn 함수
//void AInfiniteMapGenerator::SpawnPlagueMeshes(bool bBlocking, FRandomStream& Stream, const FVector& Center, bool bIsCenter, FMapChunk& Chunk)
//{
//	// bBlocking 변수로 어떤 배열을 가져올지 판단
//
//	const TArray<TObjectPtr<UStaticMesh>>& Meshes = bBlocking ? PlagueVillagePropMeshes : PlagueVillageDecoMeshes;
//	const int32 Count = bBlocking ? PlagueVillagePropPerChunk : PlagueVillageDecoPerChunk;
//
//	if (Meshes.Num() == 0)
//	{
//		return;
//	}
//
//	for (int32 i = 0; i < Count; i++)
//	{
//		UStaticMesh* Mesh = Meshes[Stream.RandRange(0, Meshes.Num() - 1)];
//
//		FVector Loc;
//		// 불가능
//		if (!PickPlagueLocation(Stream, Center, bIsCenter, Loc))
//		{
//			continue;
//		}
//
//		const FRotator Rot(0.f, Stream.FRandRange(0.f, 360.f), 0.f);
//
//		AStaticMeshActor* Actor = bBlocking ? 
//			SpawnObstacleMesh(Mesh, Loc, Rot, FVector(1.f), nullptr) :
//			SpawnDecoMesh(Mesh, Loc, Rot, FVector(Stream.FRandRange(0.8f, 1.2f)));
//
//		if (Actor)
//		{
//			Chunk.SpawnedActors.Add(Actor);
//		}
//	}
//}



// 중심 청크는 보스 스폰 때문에 피하자.
//bool AInfiniteMapGenerator::PickPlagueLocation(FRandomStream& Stream, const FVector& Center, bool bIsCenter, FVector& OutLoc) const
//{
//	const float Half = ChunkSize * 0.5f - ChunkEdgeMargin;
//
//	// 최대 5번 시도
//	for (int32 Try = 0; Try < 5; Try++)
//	{
//		const FVector Offset(Stream.FRandRange(-Half, Half), Stream.FRandRange(-Half, Half), 0.f);
//
//		// center인데 반지름안이면 무시
//		if (bIsCenter && Offset.Size2D() < PlagueVillageClearRadius)
//		{
//			continue;
//		}
//		// center 청크지만 보스 스폰 범위 바깥이면 노상관
//
//		OutLoc = Center + Offset; //아웃풋 주기
//		return true;
//	}
//	return false;
//}


//AStaticMeshActor* AInfiniteMapGenerator::SpawnDecoMesh(UStaticMesh * Mesh, const FVector & Location, const FRotator & Rotation, const FVector& Scale)
//{
//	if (!Mesh || !GetWorld())
//	{
//		return nullptr;
//	}
//
//	FActorSpawnParameters Params;
//	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	Params.Owner = this;
//
//	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, Params);
//	if (!Actor)
//	{
//		return Actor;
//	}
//
//#if WITH_EDITOR
//	Actor->SetFolderPath(TEXT("Spawned/Deco"));
//#endif
//	if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
//	{
//		Comp->SetMobility(EComponentMobility::Movable);
//		Actor->SetActorScale3D(Scale);
//		Comp->SetStaticMesh(Mesh);
//		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//		Comp->SetCanEverAffectNavigation(false);
//	}
//
//	return Actor;
//}

//오브젝트 배치라고 생각하면 됨
AStaticMeshActor* AInfiniteMapGenerator::SpawnObstacleMesh(UStaticMesh* Mesh, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale, UMaterialInterface* OverrideMat)
{
	if (!Mesh || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), Location, Rotation, Params);
	if (!Actor)
	{
		return nullptr;
	}

#if WITH_EDITOR
	// 아웃라이너 정리용 폴더 (에디터/PIE 전용 — 패키징 빌드에선 컴파일 제외)
	Actor->SetFolderPath(TEXT("Spawned/Map"));
#endif

	if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
	{
		// 런타임에 변형/메시 설정을 하려면 Movable 이어야 함 (기본은 Static)
		Comp->SetMobility(EComponentMobility::Movable);
		Actor->SetActorScale3D(Scale);
		Comp->SetStaticMesh(Mesh);
		if (OverrideMat)
		{
			Comp->SetMaterial(0, OverrideMat);
		}
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));

		// NavMesh 반영 — 런타임 스폰 시 컴포넌트는 "메시 없는 상태"로 내비에 등록되므로,
		// 메시/충돌을 다 세팅한 뒤 내비 관련성을 켜고 옥트리를 갱신해 줘야 이 바닥 위에 NavMesh가 깔린다.
		// (이게 없으면 동적 NavMesh가 런타임 바닥을 못 잡아서 적/동료가 길찾기를 못 함)
		Comp->SetCanEverAffectNavigation(true);
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->UpdateComponentInNavOctree(*Comp);
		}
	}

	return Actor;
}

// BP 생성
AActor* AInfiniteMapGenerator::SpawnDecoActor(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator & Rotation)
{
	if (!ActorClass || !GetWorld())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;

	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AActor* Actor = GetWorld()->SpawnActor<AActor>(ActorClass, Location, Rotation, Params);

#if WITH_EDITOR
	if (Actor)
	{
		Actor->SetFolderPath(TEXT("Spawned/Deco"));
	}
#endif

	return Actor;
}


// cos sin을 이용한 촛불 배치
void AInfiniteMapGenerator::SpawnPlagueCandle(const FVector& Center, FMapChunk& Chunk)
{
	// 촛불이 없다면
	if (PlagueVillageCandleClass == nullptr || PlagueVillageCandleCount <= 0)
	{
		return;
	}

	const float Step = 360.f / PlagueVillageCandleCount;

	for (int32 i = 0; i < PlagueVillageCandleCount; i++)
	{
		//라디안으로 바꿔야 한다.
		float AngleDeg = Step * i; 
		float AngleRad = FMath::DegreesToRadians(AngleDeg);

		const FVector Offset(
			FMath::Cos(AngleRad) * PlagueVillageCandleRadius,
			FMath::Sin(AngleRad) * PlagueVillageCandleRadius,
			0.f
		);

		const FVector Loc = Center + Offset;

		const FRotator Rot(0.f, AngleDeg + 180.f, 0.f);

		if (AActor* Candle = SpawnDecoActor(PlagueVillageCandleClass, Loc, Rot))
		{
			Chunk.SpawnedActors.Add(Candle);
		}

	}
}

// 맴버변수 없으면 그냥 이렇게
namespace
{
	// P는 지나가야 할 점(앞 현재 뒤) => 마을, 역병마을
	// T는 그 P1 P2의 비율

	// P(T) = a + b·T + c·T² + d·T³
	// 부드러운 곡선 방정식 
	// ㄴ 점을 무조건 지나가야한다면 이거. 만약 곡선 조절에 신경쓰고싶다면 베지어 곡선
	FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		return 0.5 * ((2.0 * P1)
			+ (P2 - P0) * T
			+ (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2
			+ (3.0 * P1 - P0 - 3.0 * P2 + P3) * T3);
	}
}

// 도로가 모이는 지점(교차로)을 반환
FVector AInfiniteMapGenerator::GetRoadNode(const FIntPoint& RegionCoord) const
{
	const int32 Size = FMath::Max(2, RegionSizeInChunks);
	const FPOIInfo Info = GetPOIForRegion(RegionCoord);

	// 중심 POI를 기준으로. 만약 아니면 그냥 리전 중간?
	const FIntPoint(NodeChunk) = Info.bHasPOI ?
		Info.CenterChunk : FIntPoint(RegionCoord.X * Size + Size / 2, RegionCoord.Y * Size + Size / 2);

	// 청크 사이즈로 변환
	return FVector((NodeChunk.X + 0.5) * ChunkSize, (NodeChunk.Y + 0.5) * ChunkSize, 0.0);
}

// 점 계산 반환 (거의 마을과 마을 사이 느낌)
void AInfiniteMapGenerator::BuildRoadEdge(const FIntPoint& RegionA, const FIntPoint& RegionB, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	// 청크 사이즈 기준
	
	const FVector A = GetRoadNode(RegionA); // 도로의 첫 부분
	const FVector B = GetRoadNode(RegionB); // 도로의 마지막 부분

	// 생존자 편향
	// 예를 들어 r값을 청크에도 똑같은 공식으로 하면 r이 0.7이상인 경우 값은 높은 확률로 r값에 비례해 높을 수 밖에 없다.
	// 그래서 다르게 한다
	const uint32 Hash = HashCombine(
		HashCombine(GetTypeHash(RegionA), GetTypeHash(RegionB)),
		HashCombine(static_cast<uint32>(GlobalSeed), 0x52A4D11u));
	FRandomStream Stream(static_cast<int32>(Hash));

	const FVector Dir = (B - A).GetSafeNormal2D(); // A -> B
	const FVector Side(-Dir.Y, Dir.X, 0.0);

	// 제어점 A - 여러가지 점 -> B
	TArray<FVector> Ctrl;
	Ctrl.Add(A);
	const int32 N = FMath::Max(0, RoadWigglePoints);
	for (int32 i = 1; i <= N; i++)
	{
		const double T = static_cast<double>(i) / (N + 1); // A와 B의 구간
		const double Offset = Stream.FRandRange(-RoadWiggleAmplitude, RoadWiggleAmplitude);
		Ctrl.Add(FMath::Lerp(A, B, T) + Side * Offset);
	}
	Ctrl.Add(B);

	// 촘촘한 곡선 => 마을과 마을 사이 16개의 마디
	const int32 Steps = 16;
	TArray<FVector> Dense;
	for (int32 s = 0; s < Ctrl.Num() - 1; s++)
	{
		const FVector& P0 = Ctrl[FMath::Max(s - 1, 0)];
		const FVector& P1 = Ctrl[s];
		const FVector& P2 = Ctrl[s + 1];
		const FVector& P3 = Ctrl[FMath::Min(s + 2, Ctrl.Num() - 1)];

		// 단계 나누기
		for (int32 k = 0; k < Steps; k++)
		{
			Dense.Add(CatmullRom(P0, P1, P2, P3, static_cast<double>(k) / Steps));
		}
	}
	Dense.Add(B); //마지막 점

	// 세심한 말뚝박이? => 이거는 한번 다시 분석. 이해가 살짝 힘드네
	const double SegLen = FMath::Max(50.0, static_cast<double>(RoadSegmentLength));
	OutPoints.Add(A);
	double Remain = SegLen; // 현재 값
	for (int32 i = 1; i < Dense.Num(); i++)
	{
		FVector From = Dense[i - 1];
		const FVector To = Dense[i];
		double Len = FVector::Dist(From, To);

		while (Len >= Remain)
		{
			From += (To - From).GetSafeNormal() * Remain;
			OutPoints.Add(From);
			Len -= Remain;
			Remain = SegLen;
		}
		Remain -= Len;
	}
	// 마지막은 정확히 B
	if (FVector::Dist(OutPoints.Last(), B) > SegLen * 0.3)
	{
		OutPoints.Add(B);
	}
	else
	{
		OutPoints.Last() = B;
	}

	// 살짝 띄움 => 땅과 도로 똑같이 0이면 겹쳐서 디자인적으로 깨짐
	const double LiftZ = FloorThickness * 0.5 + 2.0 + Stream.RandRange(0, 7) * 0.4;
	for (FVector& P : OutPoints)
	{
		P.Z = LiftZ;
	}

}


void AInfiniteMapGenerator::GetRoadSegmentsInChunk(const FIntPoint & Coord, TArray<FRoadSegment> & OutSegments) const
{
	OutSegments.Reset();

	const FIntPoint Region = ChunkToRegion(Coord);
	// Coord위치의 (1,1) 청크 박스 => 그릴 박스
	const FBox2D ChunkBox(
		FVector2D(Coord.X * ChunkSize, Coord.Y * ChunkSize),
		FVector2D((Coord.X + 1) * ChunkSize, (Coord.Y + 1) * ChunkSize));

	// 지울 박스
	const FBox2D NearBox = ChunkBox.ExpandBy(RoadWidth + RoadSegmentLength);
	
	TArray<FVector> Points;

	// 3x3
	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			const FIntPoint From = Region + FIntPoint(dx, dy);
			const FIntPoint Targets[2] = { From + FIntPoint(1,0), From + FIntPoint(0,1) };

			for (const FIntPoint& To : Targets)
			{
				BuildRoadEdge(From, To, Points); //

				for (int32 i = 0; i < Points.Num() - 1; i++)
				{
					const FVector& P0 = Points[i];
					const FVector& P1 = Points[i + 1];
					const FVector Mid = (P0 + P1) / 2;

					// Near 밖이면 버린다.
					if (!NearBox.IsInside(FVector2D(Mid)))
					{
						continue;
					}
					
					// 오브젝트걸리면 도로 X
					const FIntPoint MidChunk = WorldToChunk(Mid);
					FPOIInfo Unused;
					if (GetPOIAtChunk(MidChunk, Unused))
					{
						continue;
					}

					const FVector& Prev = Points[FMath::Max(i - 1, 0)];
					const FVector& Next = Points[FMath::Min(i + 2, Points.Num() - 1)];

					FRoadSegment Seg;
					Seg.Start = P0;
					Seg.End = P1;
					Seg.StartTangent = (P1 - Prev) * (i > 0 ? 0.5 : 1.0);
					Seg.EndTangent = (Next - P0) * (i + 2 < Points.Num() ? 0.5 : 1.0);
					Seg.bOwned = (MidChunk == Coord);
					OutSegments.Add(Seg);
				}
			}
		}
	}
}
