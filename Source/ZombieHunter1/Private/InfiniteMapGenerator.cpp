// Fill out your copyright notice in the Description page of Project Settings.

#include "InfiniteMapGenerator.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "NavigationSystem.h" // 런타임 스폰한 바닥을 NavMesh에 반영
#include "NavMesh/NavMeshBoundsVolume.h" // 플레이어 따라 옮길 나비 경계 볼륨
#include "DrawDebugHelpers.h" // POI 청크 디버그 박스
#include "MoneyPadZone.h" // 마을 발판 상태 저장/복원
#include "Characters/Companion.h" // 마을 경비병 (경비 모드로 스폰)
#include "Characters/Villager.h" // 마을 주민 (비전투 배회 NPC)
#include "Characters/Boss.h" 


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

	// [디버그] 켜져 있으면 초기 생성도 안 함 (맵 없이 시작해 핑 비교용)
	if (debugDisableGeneration)
	{
		return;
	}

	//플레이어를 가져와라
	TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (TrackedPawn)
	{
		const FIntPoint Center = WorldToChunk(TrackedPawn->GetActorLocation()); //벡터값 가져오기
		UpdateChunks(Center);
		LastPlayerChunk = Center;
		bHasGenerated = true;
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
	SpawnFog(Center, Chunk);
	SetupVillege(bIsPOIChunk, POI, Center, Chunk, Stream);
	SetupZombieVillege(bIsPOIChunk, POI, Center, Chunk, Stream);

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

	SpawnObstacles(Stream, Chunk, Origin, bIsPOIChunk);
	LoadedChunks.Add(Coord, MoveTemp(Chunk));
}

// 장애물 생성 : 청크 안에 랜덤 배치 — POI 청크는 비워둔다 (마을 콘텐츠 자리)
void AInfiniteMapGenerator::SpawnObstacles(FRandomStream & Stream, FMapChunk & Chunk, FVector Origin, bool bIsPOIChunk)
{
	if (!bIsPOIChunk && ObstacleMeshes.Num() > 0)
	{
		const int32 Count = Stream.RandRange(MinObstaclesPerChunk, FMath::Max(MinObstaclesPerChunk, MaxObstaclesPerChunk));
		for (int32 i = 0; i < Count; ++i)
		{
			UStaticMesh* Mesh = ObstacleMeshes[Stream.RandRange(0, ObstacleMeshes.Num() - 1)];
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

//POI 발자국 반경(청크). 한 변이 region 크기보다 압도적으로 큰 경우를 막음
//= > 혹시모르는 try문이라고 생각하면 됨
int32 AInfiniteMapGenerator::GetPOIRadiusInChunks() const
{
	const int32 RegionSize = FMath::Max(2, RegionSizeInChunks); //음수나 0 나누기 방지용
	const int32 Radius = (FMath::Max(1, POISizeInChunks) - 1) / 2; // 짝수 크기는 홀수(2R+1)로 내림: 4 → 3×3
	return FMath::Min(Radius, (RegionSize - 1) / 2); //만약 크기를 무식
}

//이 청크가 POI 발자국 안에 있는지
bool AInfiniteMapGenerator::GetPOIAtChunk(const FIntPoint& ChunkCoord, FPOIInfo& OutInfo) const
{
	// 중심을 리전 안쪽으로 클램프해 발자국이 리전을 못 벗어나므로, 자기 리전만 검사하면 된다
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

//월드 좌표가 "마을" 발자국 안인지 — 적 스폰 링 등 외부에서 질의 (좀비마을은 적 지역이라 해당 없음)
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


	// 리전 내 POI 중심 위치. 발자국(2R+1)²이 리전 밖으로 삐져나가지 않게 중심을 [R, Size-1-R]에서만 뽑는다.
	// (삐져나가면 옆 리전 청크가 자기 리전만 검사하는 GetPOIAtChunk에서 잘려버림)
	const int32 Radius = GetPOIRadiusInChunks();
	const int32 MaxCell = Size - 1 - Radius;

	//리전 위치 뽑는 코드
	// 시작 리전은 발자국 가장자리(중심-R)가 플레이어 시작 청크(0,0)에서 최소 2청크 떨어지게.
	// 발자국이 커서 불가능하면 리전에 들어가는 한 최대한 밀어낸다.
	const int32 MinCell = FMath::Min(bStartRegion ? (Radius + 2) : Radius, MaxCell);
	const int32 LocalX = Stream.RandRange(MinCell, MaxCell); 
	const int32 LocalY = Stream.RandRange(MinCell, MaxCell);


	Info.CenterChunk = FIntPoint(RegionCoord.X * Size + LocalX, RegionCoord.Y * Size + LocalY);

	Info.Type = (bStartRegion || Stream.FRand() < VillageRatio)
		? EPOIType::Village : EPOIType::ZombieVillage;
	return Info;
}



///////////////////////////////////////////////////////////////////////////////////////////////
//Villege
// 마을 v1: 마을 안의 오브젝트 생성 (무기고, npc 같은거)
void AInfiniteMapGenerator::SetupVillege(bool bIsPOIChunk, FPOIInfo & POI, const FVector Center, FMapChunk & Chunk, FRandomStream & Stream)
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
	for (int32 i = 0; i < Count; ++i)
	{
		// 캡슐 반높이(~90)만큼 띄워 바닥 위에 스폰
		const FVector GuardLoc = Center + FVector(GuardPosts[i].X, GuardPosts[i].Y, 92.f);
		ACompanion* Guard = GetWorld()->SpawnActor<ACompanion>(
			VillageGuardClass, GuardLoc, FRotator::ZeroRotator, GuardParams);
		if (!Guard)
		{
			continue;
		}

		Guard->Leader = nullptr;        // 아무도 안 따라간다
		Guard->bGuardHome = true;       // 대신 자기 자리를 지킨다
		Guard->HomeLocation = GuardLoc;
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

// 마을 외곽 링(반경 2400 — 발자국 절반 3000 안쪽)에 구조물 6개.
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
		Pad->PaidMoney == Defaults->PaidMoney &&
		Pad->MaxMoney == Defaults->MaxMoney &&
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
	POIStateStore.SavePad(Coord, Pad->PaidMoney, Pad->MaxMoney, Pad->IsConsumed());

	UE_LOG(LogTemp, Log, TEXT("[POIState] 저장: 청크(%d, %d) Paid %d / Max %d"),
		Coord.X, Coord.Y, Pad->PaidMoney, Pad->MaxMoney);
}

// 보스 처치 기록 — 보스가 죽는 순간 자기 소속 마을 좌표를 들고 여기로 온다.
void AInfiniteMapGenerator::MarkBossKilled(const FIntPoint& CenterChunk)
{
	POIStateStore.MarkBossKilled(CenterChunk);

	UE_LOG(LogTemp, Log, TEXT("[POIState] 좀비마을(%d, %d) 보스 처치 기록 — 재방문해도 다시 스폰되지 않는다"),
		CenterChunk.X, CenterChunk.Y);
}


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


void AInfiniteMapGenerator::SpawnFog(const FVector & Center, FMapChunk& Chunk)
{
	if (FogClass)
	{
		const float Base = FMath::Max(1.f, FogBaseSize);
		FActorSpawnParameters FogParams;
		FogParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FogParams.Owner = this;

		const FVector FogLoc = Center + FVector(0.f, 0.f, FogSizeHeight / 2);

		//회전, 위치, 사이즈
		const FTransform FogTransform(FRotator::ZeroRotator, FogLoc, FVector(ChunkSize / Base, ChunkSize / Base, FogSizeHeight / Base));

		if (AActor* Fog = GetWorld()->SpawnActor<AActor>(FogClass, FogTransform, FogParams))
		{
#if WITH_EDITOR
			Fog->SetFolderPath(TEXT("Spawned/Map"));
#endif
			Chunk.SpawnedActors.Add(Fog);
		}
	}
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

//보스 나오는 구역 생성
void AInfiniteMapGenerator::SetupZombieVillege(bool bIsPOIChunk, FPOIInfo& POI, const FVector Center, FMapChunk& Chunk, FRandomStream& Stream)
{
	if (bIsPOIChunk && POI.bIsCenter && POI.Type == EPOIType::ZombieVillage&& BossClass)
	{
		//아마 전리품이나 구조는 이런곳에?


		// todo : 나중에 보스 따로 함수 만들어야 함
		// 이미 이번 판에 클리어한 좀비마을이면 보스를 다시 세우지 않는다.
		// (없으면 보스를 죽이고 멀리 갔다 오는 것만으로 풀피 보스가 부활한다)
		if (POIStateStore.IsBossKilled(POI.CenterChunk))
		{
			return;
		}

		FActorSpawnParameters VillagerParams;
		VillagerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		VillagerParams.Owner = this;

		ABoss* Boss = GetWorld()->SpawnActor<ABoss>(BossClass, Center, FRotator::ZeroRotator, VillagerParams);
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
}
