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

#include "InAppUpdatesManager.h"

#include "AppUpdateInfo.h"
#include "InAppUpdatesError.h"
#include "Misc/CoreDelegates.h"
#include "PlayInAppUpdates.h"

#if SUPPORTED_PLATFORM
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"

extern JavaVM* GJavaVM;
extern jobject GGameActivityThis;
#endif  // SUPPORTED_PLATFORM

// Namespace used for handling internal update status listeners.
namespace
{
// Internal update status listener for onResume.
void HandleApplicationHasEnteredForeground()
{
#if SUPPORTED_PLATFORM
	AppUpdateManager_onResume();
#endif  // SUPPORTED_PLATFORM
}

// Internal update status listener for onPause.
void HandleApplicationWillEnterBackground()
{
#if SUPPORTED_PLATFORM
	AppUpdateManager_onPause();
#endif  // SUPPORTED_PLATFORM
}
}  // namespace

void UInAppUpdatesManager::Initialize(FSubsystemCollectionBase& Collection)
{
#if SUPPORTED_PLATFORM
	AppUpdateErrorCode ErrorCode =
		AppUpdateManager_init(GJavaVM, GGameActivityThis);
	if (ErrorCode != APP_UPDATE_NO_ERROR)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("AppUpdateManager_init failed: %s"),
			*UEnum::GetValueAsName(
				InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode))
				.ToString());
		return;
	}

	// Register internal update status listener for onResume
	ResumeHandle =
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddStatic(
			&HandleApplicationHasEnteredForeground);
	// Register internal update status listener for onPause
	PauseHandle =
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddStatic(
			&HandleApplicationWillEnterBackground);
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::Deinitialize()
{
#if SUPPORTED_PLATFORM
	if (ResumeHandle.IsValid())
	{
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(
			ResumeHandle);
		ResumeHandle.Reset();
	}
	if (PauseHandle.IsValid())
	{
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(
			PauseHandle);
		PauseHandle.Reset();
	}
	if (UpdateInfo != nullptr)
	{
		UpdateInfo->ClearAppUpdateInfo();
	}
	if (UpdateOptions != nullptr)
	{
		UpdateOptions->ClearAppUpdateOptions();
	}
	AppUpdateManager_destroy();
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::RequestInfo(
	FRequestInfoOperationCompletedDelegate InOnRequestInfoOperationCompleted)
{
	if (OnRequestInfoOperationCompleted.IsBound())
	{
		OnRequestInfoOperationCompleted.ExecuteIfBound(
			EAppUpdateErrorCode::AppUpdate_REQUEST_CANCELED, nullptr);
	}

	OnRequestInfoOperationCompleted = InOnRequestInfoOperationCompleted;

#if SUPPORTED_PLATFORM
	AppUpdateErrorCode ErrorCode = AppUpdateManager_requestInfo();
	if (ErrorCode != APP_UPDATE_NO_ERROR)
	{
		OnRequestInfoOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode), nullptr);
		OnRequestInfoOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(RequestInfoPollingTimerHandle,
		this, &UInAppUpdatesManager::HandleCheckInfoStatusTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnRequestInfoOperationCompleted.ExecuteIfBound(
		EAppUpdateErrorCode::AppUpdate_NO_ERROR, nullptr);
	OnRequestInfoOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::StartUpdate(UAppUpdateInfo* InUpdateInfo,
	UAppUpdateOptions* InUpdateOptions,
	FUpdateOperationCompletedDelegate InOnStartUpdateOperationCompleted)
{
	if (InUpdateInfo == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("InUpdateInfo is null, please call RequestInfo() first."))
		InOnStartUpdateOperationCompleted.ExecuteIfBound(
			EAppUpdateErrorCode::AppUpdate_INVALID_REQUEST);
		return;
	}

	if (InUpdateOptions == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("InUpdateOptions is null, please call "
				 "UAppUpdateOptions::CreateAppUpdateOptions() first."))
		InOnStartUpdateOperationCompleted.ExecuteIfBound(
			EAppUpdateErrorCode::AppUpdate_INVALID_REQUEST);
		return;
	}

	if (OnStartUpdateOperationCompleted.IsBound())
	{
		OnStartUpdateOperationCompleted.ExecuteIfBound(
			EAppUpdateErrorCode::AppUpdate_REQUEST_CANCELED);
	}

	OnStartUpdateOperationCompleted = InOnStartUpdateOperationCompleted;
	UpdateInfo = InUpdateInfo;
	UpdateOptions = InUpdateOptions;

#if SUPPORTED_PLATFORM
	AppUpdateErrorCode ErrorCode = AppUpdateManager_requestStartUpdate(
		UpdateInfo->GetNativeAppUpdateInfo(),
		UpdateOptions->GetNativeUpdateOptions(), GGameActivityThis);

	if (ErrorCode != APP_UPDATE_NO_ERROR)
	{
		OnStartUpdateOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode));
		OnStartUpdateOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(StartUpdatePollingTimerHandle,
		this, &UInAppUpdatesManager::HandleCheckStartUpdateTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnStartUpdateOperationCompleted.ExecuteIfBound(
		EAppUpdateErrorCode::AppUpdate_NO_ERROR);
	OnStartUpdateOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::CompleteUpdate(
	FUpdateOperationCompletedDelegate InOnCompleteUpdateOperationCompleted)
{
	if (OnCompleteUpdateOperationCompleted.IsBound())
	{
		OnCompleteUpdateOperationCompleted.ExecuteIfBound(
			EAppUpdateErrorCode::AppUpdate_REQUEST_CANCELED);
	}

	OnCompleteUpdateOperationCompleted = InOnCompleteUpdateOperationCompleted;

#if SUPPORTED_PLATFORM
	AppUpdateErrorCode ErrorCode = AppUpdateManager_requestCompleteUpdate();

	if (ErrorCode != APP_UPDATE_NO_ERROR)
	{
		OnCompleteUpdateOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode));
		OnCompleteUpdateOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(
		CompleteUpdatePollingTimerHandle, this,
		&UInAppUpdatesManager::HandleCheckCompleteUpdateTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnCompleteUpdateOperationCompleted.ExecuteIfBound(
		EAppUpdateErrorCode::AppUpdate_NO_ERROR);
	OnCompleteUpdateOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::HandleCheckInfoStatusTimer()
{
#if SUPPORTED_PLATFORM
	AppUpdateInfo* NativeAppUpdateInfo = nullptr;
	AppUpdateErrorCode ErrorCode =
		AppUpdateManager_getInfo(&NativeAppUpdateInfo);
	AppUpdateStatus UpdateStatus = AppUpdateInfo_getStatus(NativeAppUpdateInfo);
	if (ErrorCode == APP_UPDATE_NO_ERROR &&
		(NativeAppUpdateInfo == nullptr ||
			UpdateStatus == APP_UPDATE_REQUEST_INFO_PENDING))
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			RequestInfoPollingTimerHandle, this,
			&UInAppUpdatesManager::HandleCheckInfoStatusTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	if (ErrorCode != APP_UPDATE_NO_ERROR)
	{
		OnRequestInfoOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode), nullptr);
	}
	else if (NativeAppUpdateInfo == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("An error occurred while getting app update info."));
		OnRequestInfoOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode), nullptr);
	}
	else
	{
		UAppUpdateInfo* AppUpdateInfo = NewObject<UAppUpdateInfo>();
		AppUpdateInfo->Initialize(NativeAppUpdateInfo);

		OnRequestInfoOperationCompleted.ExecuteIfBound(
			InAppUpdatesError::ConvertAppUpdateErrorCode(ErrorCode),
			AppUpdateInfo);
		OnRequestInfoOperationCompleted.Clear();
	}
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::HandleCheckStartUpdateTimer()
{
#if SUPPORTED_PLATFORM
	if (UpdateInfo == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("An error occurred while getting app update info. Please call "
				 "RequestInfo() first."));
		return;
	}

	AppUpdateInfo* NativeAppUpdateInfo = UpdateInfo->GetNativeAppUpdateInfo();
	AppUpdateStatus UpdateStatus = AppUpdateInfo_getStatus(NativeAppUpdateInfo);
	if (UpdateStatus == APP_UPDATE_REQUEST_START_UPDATE_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			StartUpdatePollingTimerHandle, this,
			&UInAppUpdatesManager::HandleCheckStartUpdateTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	OnStartUpdateOperationCompleted.ExecuteIfBound(
		EAppUpdateErrorCode::AppUpdate_NO_ERROR);
	OnStartUpdateOperationCompleted.Clear();

	if (UpdateOptions->GetAppUpdateType() ==
		EAppUpdateType::AppUpdate_TYPE_IMMEDIATE)
	{
		UpdateInfo->ClearAppUpdateInfo();
		UpdateOptions->ClearAppUpdateOptions();
		UpdateInfo = nullptr;
		UpdateOptions = nullptr;
	}
#endif  // SUPPORTED_PLATFORM
}

void UInAppUpdatesManager::HandleCheckCompleteUpdateTimer()
{
#if SUPPORTED_PLATFORM
	if (UpdateInfo == nullptr)
	{
		UE_LOG(LogPlayInAppUpdates, Error,
			TEXT("An error occurred while getting app update info. Please call "
				 "RequestInfo() first."));
		return;
	}

	AppUpdateInfo* NativeAppUpdateInfo = UpdateInfo->GetNativeAppUpdateInfo();
	AppUpdateStatus UpdateStatus = AppUpdateInfo_getStatus(NativeAppUpdateInfo);
	if (UpdateStatus == APP_UPDATE_REQUEST_COMPLETE_UPDATE_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			CompleteUpdatePollingTimerHandle, this,
			&UInAppUpdatesManager::HandleCheckCompleteUpdateTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	OnCompleteUpdateOperationCompleted.ExecuteIfBound(
		EAppUpdateErrorCode::AppUpdate_NO_ERROR);
	OnCompleteUpdateOperationCompleted.Clear();

	UpdateInfo->ClearAppUpdateInfo();
	UpdateOptions->ClearAppUpdateOptions();
	UpdateInfo = nullptr;
	UpdateOptions = nullptr;
#endif  // SUPPORTED_PLATFORM
}
