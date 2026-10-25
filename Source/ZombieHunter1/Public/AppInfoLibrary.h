#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AppInfoLibrary.generated.h"

UCLASS()
class ZOMBIEHUNTER1_API UAppInfoLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 프로젝트 설정의 Project Version (예: 0.3.4)
	UFUNCTION(BlueprintPure, Category = "AppInfo")
	static FString GetAppVersion();
};
