// Fill out your copyright notice in the Description page of Project Settings.


#include "AppInfoLibrary.h"
#include "GeneralProjectSettings.h"

FString UAppInfoLibrary::GetAppVersion()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}
