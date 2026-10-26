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

#include "InAppUpdatesError.h"

#include "AppUpdateOptions.generated.h"

/**
 * @ingroup PlayInAppUpdates
 * @{
 */

/**
 * This class represents the options for the app update.
 *
 * To create an instance of this class, use the CreateAppUpdateOptions function,
 * which initializes the object based on the given update type.
 */
UCLASS(BlueprintType)
class PLAYINAPPUPDATES_API UAppUpdateOptions : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Creates an UAppUpdateOptions object.
	 *
	 * @param UpdateType The type of update.
	 *
	 * @return The UAppUpdateOptions object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	static UAppUpdateOptions* CreateAppUpdateOptions(EAppUpdateType UpdateType);

#if SUPPORTED_PLATFORM
	/**
	 * Returns the native app options object.
	 */
	AppUpdateOptions* GetNativeUpdateOptions() const;
#endif  // SUPPORTED_PLATFORM

	/**
	 * Part of UObject interface. Called when the object is marked for deletion
	 * by MarkPendingKill.
	 */
	virtual void BeginDestroy() override;

	/**
	 * Clears the native app update options object.
	 */
	void ClearAppUpdateOptions();

	/**
	 * Returns the type of update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	EAppUpdateType GetAppUpdateType();

	/**
	 * Returns whether the asset pack deletion is allowed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	bool IsAssetPackDeletionAllowed();

	/**
	 * Sets whether the asset pack deletion is allowed.
	 *
	 * @param bAllow Whether the asset pack deletion is allowed.
	 *
	 * @return The error code of the operation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Updates")
	EAppUpdateErrorCode SetAssetPackDeletionAllowed(bool bAllow);

private:
#if SUPPORTED_PLATFORM
	AppUpdateOptions* NativeUpdateOptions = nullptr;
	void Initialize(AppUpdateOptions* InNativeAppUpdateOptions);
#endif  // SUPPORTED_PLATFORM
};

/** @} */
