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

#include "AppUpdateOptions.h"

#include "InAppUpdatesError.h"
#include "PlayInAppUpdates.h"

// Namespace used for storing the null options error message.
namespace
{
const FString kNullOptionsErrorMessage =
	"NativeUpdateOptions is null. Ensure that you have created the "
	"UAppUpdateOptions object using the CreateAppUpdateOptions function.";

}

UAppUpdateOptions* UAppUpdateOptions::CreateAppUpdateOptions(
	EAppUpdateType InUpdateType)
{
	UAppUpdateOptions* UpdateOptions = NewObject<UAppUpdateOptions>();
#if SUPPORTED_PLATFORM
	AppUpdateOptions* NativeUpdateOptions = nullptr;
	AppUpdateType NativeUpdateType =
		InAppUpdatesError::ConvertNativeAppUpdateType(InUpdateType);
	AppUpdateOptions_createOptions(NativeUpdateType, &NativeUpdateOptions);
	UpdateOptions->Initialize(NativeUpdateOptions);
#endif
	return UpdateOptions;
}

#if SUPPORTED_PLATFORM
void UAppUpdateOptions::Initialize(AppUpdateOptions* InNativeAppUpdateOptions)
{
	NativeUpdateOptions = InNativeAppUpdateOptions;
}

AppUpdateOptions* UAppUpdateOptions::GetNativeUpdateOptions() const
{
	return NativeUpdateOptions;
}
#endif  // SUPPORTED_PLATFORM

void UAppUpdateOptions::BeginDestroy()
{
	ClearAppUpdateOptions();
	Super::BeginDestroy();
}

void UAppUpdateOptions::ClearAppUpdateOptions()
{
#if SUPPORTED_PLATFORM
	if (NativeUpdateOptions != nullptr)
	{
		AppUpdateOptions_destroy(NativeUpdateOptions);
		NativeUpdateOptions = nullptr;
		UE_LOG(LogPlayInAppUpdates, Warning,
			TEXT("Destroying existing app update options..."));
	}
#endif  // SUPPORTED_PLATFORM
}

EAppUpdateType UAppUpdateOptions::GetAppUpdateType()
{
	EAppUpdateType AppUpdateType = EAppUpdateType::AppUpdate_TYPE_UNKNOWN;
#if SUPPORTED_PLATFORM
	checkf(NativeUpdateOptions != nullptr, *kNullOptionsErrorMessage);
	AppUpdateType = InAppUpdatesError::ConvertAppUpdateType(
		AppUpdateOptions_getAppUpdateType(NativeUpdateOptions));
#endif  // SUPPORTED_PLATFORM
	return AppUpdateType;
}

bool UAppUpdateOptions::IsAssetPackDeletionAllowed()
{
	bool AssetPackDeletionAllowed = false;
#if SUPPORTED_PLATFORM
	checkf(NativeUpdateOptions != nullptr, *kNullOptionsErrorMessage);
	AssetPackDeletionAllowed =
		AppUpdateOptions_isAssetPackDeletionAllowed(NativeUpdateOptions);
#endif  // SUPPORTED_PLATFORM
	return AssetPackDeletionAllowed;
}

EAppUpdateErrorCode UAppUpdateOptions::SetAssetPackDeletionAllowed(bool bAllow)
{
	EAppUpdateErrorCode ErrorCode = EAppUpdateErrorCode::AppUpdate_NO_ERROR;
#if SUPPORTED_PLATFORM
	checkf(NativeUpdateOptions != nullptr, *kNullOptionsErrorMessage);
	ErrorCode = InAppUpdatesError::ConvertAppUpdateErrorCode(
		AppUpdateOptions_setAssetPackDeletionAllowed(
			NativeUpdateOptions, bAllow));
#endif  // SUPPORTED_PLATFORM
	return ErrorCode;
}
