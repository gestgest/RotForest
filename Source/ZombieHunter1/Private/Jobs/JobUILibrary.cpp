// Fill out your copyright notice in the Description page of Project Settings.

#include "Jobs/JobUILibrary.h"
#include "Components/ComboBoxString.h"

FText UJobUILibrary::GetJobDisplayName(const FJobDefinition& Job)
{
	// 원본은 구조체의 DisplayName. 에디터에서 편집 가능하고 나중에 String Table로 갈아끼울 수 있다.
	if (!Job.DisplayName.IsEmpty())
	{
		return Job.DisplayName;
	}

	// 폴백: 채워넣는 걸 깜빡했을 때 콤보박스가 빈 칸으로 보이면 원인을 못 찾는다.
	// EJobType의 UMETA(DisplayName)이라도 보여줘서 "어느 칸이 비었는지" 눈에 띄게 한다.
	if (const UEnum* JobEnum = StaticEnum<EJobType>())
	{
		return JobEnum->GetDisplayNameTextByValue(static_cast<int64>(Job.JobType));
	}

	return FText::GetEmpty();
}

void UJobUILibrary::PopulateJobComboBox(UComboBoxString* ComboBox, const TArray<FJobDefinition>& Jobs, int32 InitialIndex)
{
	if (!ComboBox)
	{
		return;
	}

	// 에디터 Default Options가 남아 있어도 여기서 지워지므로 중복되지 않는다.
	ComboBox->ClearOptions();

	for (const FJobDefinition& Job : Jobs)
	{
		ComboBox->AddOption(GetJobDisplayName(Job).ToString());
	}

	// 옵션을 다 채운 뒤에 선택해야 한다. 비어 있을 때 인덱스를 지정하면 아무 일도 일어나지 않는다.
	if (Jobs.IsValidIndex(InitialIndex))
	{
		ComboBox->SetSelectedIndex(InitialIndex);
	}
}
