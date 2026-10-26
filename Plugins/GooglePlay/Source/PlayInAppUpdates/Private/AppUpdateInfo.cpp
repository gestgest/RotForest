/* Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AppUpdateInfo.h"

#include "AppUpdateOptions.h"
#include "PlayInAppUpdates.h"

#if SUPPORTED_PLATFORM
void UAppUpdateInfo::Initialize(AppUpdateInfo* InNativeAppUpdateInfo)
{
	NativeAppUpdateInfo = InNativeAppUpdateInfo;
}

AppUpdateInfo* UAppUpdateInfo::GetNativeAppUpdateInfo() const
{
	return NativeAppUpdateInfo;
}
#endif  // SUPPORTED_PLATFORM

void UAppUpdateInfo::BeginDestroy()
{
	ClearAppUpdateInfo();
	Super::BeginDestroy();
}

void UAppUpdateInfo::ClearAppUpdateInfo()
{
#if SUPPORTED_PLATFORM
	if (NativeAppUpdateInfo != nullptr)
	{
		AppUpdateInfo_destroy(NativeAppUpdateInfo);
		NativeAppUpdateInfo = nullptr;
		UE_LOG(LogPlayInAppUpdates, Warning,
			TEXT("Destroying existing app update info..."));
	}
#endif  // SUPPORTED PLATFORM
}

EAppUpdateAvailability UAppUpdateInfo::GetAvailability()
{
	EAppUpdateAvailability Availability =
		EAppUpdateAvailability::AppUpdate_AVAILABILITY_UNKNOWN;
#if SUPPORTED_PLATFORM
	Availability = InAppUpdatesError::ConvertAppUpdateAvailability(
		AppUpdateInfo_getAvailability(NativeAppUpdateInfo));
#endif  // SUPPORTED_PLATFORM
	return Availability;
}

int32 UAppUpdateInfo::GetAvailableVersionCode()
{
	int32 AvailableVersionCode = 0;
#if SUPPORTED_PLATFORM
	AvailableVersionCode =
		AppUpdateInfo_getAvailableVersionCode(NativeAppUpdateInfo);
#endif  // SUPPORTED_PLATFORM
	return AvailableVersionCode;
}

int64 UAppUpdateInfo::GetBytesDownloaded()
{
	int64 BytesDownloaded = 0;
#if SUPPORTED_PLATFORM
	BytesDownloaded = AppUpdateInfo_getBytesDownloaded(NativeAppUpdateInfo);
#endif  // SUPPORTED_PLATFORM
	return BytesDownloaded;
}

int32 UAppUpdateInfo::GetClientVersionStalenessDays()
{
	int32 ClientVersionStalenessDays = 0;
#if SUPPORTED_PLATFORM
	ClientVersionStalenessDays =
		AppUpdateInfo_getClientVersionStalenessDays(NativeAppUpdateInfo);
#endif  // SUPPORTED_PLATFORM
	return ClientVersionStalenessDays;
}

int32 UAppUpdateInfo::GetPriority()
{
	int32 Priority = 0;
#if SUPPORTED_PLATFORM
	Priority = AppUpdateInfo_getPriority(NativeAppUpdateInfo);
#endif  // SUPPORTED_PLATFORM
	return Priority;
}

EAppUpdateStatus UAppUpdateInfo::GetStatus()
{
	EAppUpdateStatus Status = EAppUpdateStatus::AppUpdate_STATUS_UNKNOWN;
#if SUPPORTED_PLATFORM
	Status = InAppUpdatesError::ConvertAppUpdateStatus(
		AppUpdateInfo_getStatus(NativeAppUpdateInfo));
#endif  // SUPPORTED_PLATFORM
	return Status;
}

int64 UAppUpdateInfo::GetTotalBytesToDownload()
{
	int64 TotalBytesToDownload = 0;
#if SUPPORTED_PLATFORM
	TotalBytesToDownload =
		AppUpdateInfo_getTotalBytesToDownload(NativeAppUpdateInfo);
#endif  // SUPPORTED_PLATFORM
	return TotalBytesToDownload;
}

bool UAppUpdateInfo::IsUpdateTypeAllowed(UAppUpdateOptions* InAppUpdateOptions)
{
	bool UpdateTypeAllowed = false;
#if SUPPORTED_PLATFORM
	if (InAppUpdateOptions->GetNativeUpdateOptions() == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT(
				"Please call UAppUpdateOptions::CreateOptions object before."));
		return false;
	}
	UpdateTypeAllowed = AppUpdateInfo_isUpdateTypeAllowed(
		NativeAppUpdateInfo, InAppUpdateOptions->GetNativeUpdateOptions());
#endif  // SUPPORTED_PLATFORM
	return UpdateTypeAllowed;
}
