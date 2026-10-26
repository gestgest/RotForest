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

#pragma once

#include "CoreMinimal.h"

#include "AppUpdateInfo.h"
#include "AppUpdateOptions.h"
#include "Delegates/IDelegateInstance.h"
#include "Engine/TimerHandle.h"
#include "InAppUpdatesError.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "InAppUpdatesManager.generated.h"

/**
 * @defgroup PlayInAppUpdates In-App Updates
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a request info
 * operation.
 *
 * @param Error The error code of the operation.
 * @param Response The UAppUpdateInfo object containing information about the
 * update.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FRequestInfoOperationCompletedDelegate,
	EAppUpdateErrorCode, Error, UAppUpdateInfo*, Response);

/**
 * @private
 *
 * The delegate used to handle the completion of a start update
 * operation.
 *
 * @param Error The error code of the operation.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(
	FUpdateOperationCompletedDelegate, EAppUpdateErrorCode, Error);

/**
 * The UInAppUpdatesManager class is used to request app update information,
 * start an update or complete a flexible update.
 *
 * Refer to https://developer.android.com/guide/playcore/in-app-updates
 * for more details.
 */
UCLASS()
class PLAYINAPPUPDATES_API UInAppUpdatesManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Requests information about an update. Needs to be called once before
	 * StartUpdate() to obtain an AppUpdateInfo, which is required to launch the
	 * in-app update flow.
	 *
	 * @param InOnRequestInfoOperationCompleted The delegate to handle the
	 * completion of the request.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	void RequestInfo(FRequestInfoOperationCompletedDelegate
			InOnRequestInfoOperationCompleted);

	/**
	 * Starts the in-app update flow. Use the InUpdateInfo functions to monitor
	 * the current status and download progress of the ongoing update operation.
	 *
	 * @param InUpdateInfo The object containing information needed for an
	 * update.
	 * @param InUpdateOptions The object specifying the update flow type.
	 * @param InOnUpdateOperationCompleted The delegate to handle the
	 * completion of the function.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	void StartUpdate(UAppUpdateInfo* InUpdateInfo,
		UAppUpdateOptions* InUpdateOptions,
		FUpdateOperationCompletedDelegate InOnUpdateOperationCompleted);

	/**
	 * Requests to complete a flexible in-app update flow that was started via
	 * the StartUpdate() function.
	 *
	 * @param InOnStartUpdateOperationCompleted The delegate to handle the
	 * completion of the function.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	void CompleteUpdate(
		FUpdateOperationCompletedDelegate InOnStartUpdateOperationCompleted);

private:
	void HandleCheckInfoStatusTimer();
	void HandleCheckStartUpdateTimer();
	void HandleCheckCompleteUpdateTimer();
	FTimerHandle RequestInfoPollingTimerHandle;
	FTimerHandle StartUpdatePollingTimerHandle;
	FTimerHandle CompleteUpdatePollingTimerHandle;
	FRequestInfoOperationCompletedDelegate OnRequestInfoOperationCompleted;
	FUpdateOperationCompletedDelegate OnStartUpdateOperationCompleted;
	FUpdateOperationCompletedDelegate OnCompleteUpdateOperationCompleted;
	FDelegateHandle PauseHandle;
	FDelegateHandle ResumeHandle;

	UPROPERTY()
	UAppUpdateInfo* UpdateInfo = nullptr;

	UPROPERTY()
	UAppUpdateOptions* UpdateOptions = nullptr;
};

/** @} */
