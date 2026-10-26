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

#include "AppUpdateOptions.h"

#include "AppUpdateInfo.generated.h"

/**
 * @ingroup PlayInAppUpdates
 * @{
 */

/**
 * This class represents the information about the app update.
 */
UCLASS(BlueprintType)
class PLAYINAPPUPDATES_API UAppUpdateInfo : public UObject
{
	GENERATED_BODY()
public:
#if SUPPORTED_PLATFORM
	/**
	 * Initializes the UAppUpdateInfo object and sets the native app update
	 * info object.
	 *
	 * @param InNativeAppUpdateInfo The native app update info object.
	 */
	void Initialize(AppUpdateInfo* InNativeAppUpdateInfo);

	/**
	 * Returns the native app update info object.
	 */
	AppUpdateInfo* GetNativeAppUpdateInfo() const;
#endif  // SUPPORTED_PLATFORM

	/**
	 * Part of UObject interface. Called when the object is marked for deletion
	 * by MarkPendingKill.
	 */
	virtual void BeginDestroy() override;

	/**
	 * Clears the native app update info object.
	 */
	void ClearAppUpdateInfo();

	/**
	 * Returns the availability of the app update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	EAppUpdateAvailability GetAvailability();

	/**
	 * Returns the available version code of the app update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	int32 GetAvailableVersionCode();

	/**
	 * Returns the number of bytes downloaded so far.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	int64 GetBytesDownloaded();

	/**
	 * Returns the number of days since the client version was last updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	int32 GetClientVersionStalenessDays();

	/**
	 * Returns the priority of the app update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	int32 GetPriority();

	/**
	 * Returns the status of the app update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	EAppUpdateStatus GetStatus();

	/**
	 * Returns the total number of bytes to download.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	int64 GetTotalBytesToDownload();

	/**
	 * Returns whether the update type is allowed.
	 *
	 * @param InAppUpdateOptions The UAppUpdateOptions object which contains the
	 * type of update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	bool IsUpdateTypeAllowed(UAppUpdateOptions* InAppUpdateOptions);

private:
#if SUPPORTED_PLATFORM
	AppUpdateInfo* NativeAppUpdateInfo = nullptr;
#endif  // SUPPORTED_PLATFORM
};

/** @} */
