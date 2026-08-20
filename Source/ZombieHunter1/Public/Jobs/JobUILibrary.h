// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Jobs/JobComponent.h"
#include "JobUILibrary.generated.h"

class UComboBoxString;

/**
 * 직업 선택 UI가 쓰는 헬퍼 모음.
 *
 * 왜 게임플레이 클래스가 아니라 함수 라이브러리인가:
 * "콤보박스"는 UI의 물건이라 UJobComponent(게임플레이)가 알면 안 된다.
 * 그렇다고 BP에서 ForEach로 손수 돌리면 노드가 8개로 불어난다. 그 사이를 여기가 메운다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UJobUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 콤보박스를 비우고 Jobs 배열의 DisplayName으로 다시 채운다.
	 * 옵션 순서 = 배열 순서이므로, 선택 결과는 인덱스로 되돌리면 된다(문자열 비교 금지).
	 *
	 * @param ComboBox     채울 대상. 없으면 아무 일도 하지 않는다.
	 * @param Jobs         직업 목록. 순서가 곧 콤보박스 순서다.
	 * @param InitialIndex 초기 선택 인덱스. 범위를 벗어나면 선택하지 않는다.
	 *
	 * 주의: 이 함수는 콤보박스의 "보이는 선택"만 맞춘다.
	 * GameInstance의 SelectedJobClass는 BP에서 ApplyJob(InitialIndex)로 따로 넣어야 한다.
	 */

	//콤보박스
	UFUNCTION(BlueprintCallable, Category = "Job|UI", meta = (DisplayName = "Populate Job ComboBox"))
	static void PopulateJobComboBox(UComboBoxString* ComboBox, const TArray<FJobDefinition>& Jobs, int32 InitialIndex = 0);

	// 직업 하나의 표시 이름. DisplayName이 비어 있으면 EJobType의 UMETA(DisplayName)으로 대신한다. 
	UFUNCTION(BlueprintPure, Category = "Job|UI")
	static FText GetJobDisplayName(const FJobDefinition& Job);
};
