// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieGameInstance.h"
#include "Jobs/JobUILibrary.h"

FText UZombieGameInstance::GetJobDisplayName(EJobType JobType) const
{
	for (const FJobDefinition& Job : Jobs)
	{
		if (Job.JobType == JobType)
		{
			return UJobUILibrary::GetJobDisplayName(Job);
		}
	}

	FJobDefinition Fallback;
	Fallback.JobType = JobType;
	return UJobUILibrary::GetJobDisplayName(Fallback);
}
