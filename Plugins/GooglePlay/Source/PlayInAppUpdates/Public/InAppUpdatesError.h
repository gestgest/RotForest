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

#include "InAppUpdatesError.generated.h"

#define SUPPORTED_PLATFORM (PLATFORM_ANDROID && USE_ANDROID_JNI)

#if SUPPORTED_PLATFORM
#include "play/app_update.h"
#endif  // SUPPORTED_PLATFORM

/**
 * @ingroup PlayInAppUpdates
 *
 * Errors that can be encountered while using the Play In-App Updates API
 * This enum maps to the error codes defined in the Play In-App Updates API in
 * https://developer.android.com/reference/native/play/core/group/appupdate#appupdateerrorcode
 */
UENUM(BlueprintType)
enum class EAppUpdateErrorCode : uint8
{
	// No error has occurred.
	AppUpdate_NO_ERROR,

	// An unknown error occurred.
	AppUpdate_UNKNOWN_ERROR,

	// The in-app update API isn't available on this device.
	AppUpdate_API_NOT_AVAILABLE,

	// The function call was invalid, for example due to specifying a null
	// parameter.
	AppUpdate_INVALID_REQUEST,

	// The update is unavailable to this user or device.
	AppUpdate_UNAVAILABLE,

	// The update isn't allowed due to the current device state, for example low
	// battery or low disk space.
	AppUpdate_NOT_ALLOWED,

	// The update has not been (fully) downloaded yet.
	AppUpdate_DOWNLOAD_NOT_PRESENT,

	// The update is already in progress and there is no UI flow to resume.
	AppUpdate_IN_PROGRESS,

	// The Play Store app is either not installed or not the official version.
	AppUpdate_PLAY_STORE_NOT_FOUND,

	// The app isn't owned by any user on this device. An app is "owned" if it
	// has been acquired from the Play Store.
	AppUpdate_APP_NOT_OWNED,

	// An internal error occurred.
	AppUpdate_INTERNAL_ERROR,

	// The requested operation failed: call AppUpdateManager_init() first.
	AppUpdate_INITIALIZATION_NEEDED,

	// Error initializing dependencies.
	AppUpdate_INITIALIZATION_FAILED,

	// The previous request has been canceled in favor of a new one.
	AppUpdate_REQUEST_CANCELED,
};

/**
 * @ingroup PlayInAppUpdates
 *
 * Status returned when requesting an update.
 * This enum maps to the error codes defined in the Play In-App Updates API in
 * https://developer.android.com/reference/native/play/core/group/appupdate#appupdatestatus
 */
UENUM(BlueprintType)
enum class EAppUpdateStatus : uint8
{
	// The update status is unknown.
	AppUpdate_STATUS_UNKNOWN,

	// Download of an update is pending and will be processed soon.
	AppUpdate_PENDING,

	// Download of an update is in progress.
	AppUpdate_DOWNLOADING,

	// An update is being installed.
	AppUpdate_INSTALLING,

	// An update has been successfully installed.
	AppUpdate_INSTALLED,

	// An update has failed.
	AppUpdate_FAILED,

	// An update has been canceled.
	AppUpdate_CANCELED,

	// An update has been fully downloaded.
	AppUpdate_DOWNLOADED,

	// Request for update info is pending and will be processed soon.
	AppUpdate_REQUEST_INFO_PENDING,

	// Request for update info has failed.
	AppUpdate_REQUEST_INFO_FAILED,

	// Request for update info has completed.
	AppUpdate_REQUEST_INFO_COMPLETED,

	// Request for starting an update is pending and will be processed soon.
	AppUpdate_REQUEST_START_UPDATE_PENDING,

	// Request for completing an update is pending and will be processed soon.
	AppUpdate_REQUEST_COMPLETE_UPDATE_PENDING,
};

/**
 * @ingroup PlayInAppUpdates
 *
 * Availability info for an in-app update.
 * This enum maps to the error codes defined in the Play In-App Updates API in
 * https://developer.android.com/reference/native/play/core/group/appupdate#appupdateavailability
 */
UENUM(BlueprintType)
enum class EAppUpdateAvailability : uint8
{
	// Update availability is unknown.
	AppUpdate_AVAILABILITY_UNKNOWN,

	// No updates are available.
	AppUpdate_NOT_AVAILABLE,

	// An update is available.
	AppUpdate_AVAILABLE,

	// An update has been triggered by the developer and is in progress.
	AppUpdate_TRIGGERED_IN_PROGRESS,
};

/**
 * @ingroup PlayInAppUpdates
 *
 * Methods for performing the in-app update flow. Note: regardless of the
 * method selected, the app needs to be restarted to install an update.
 * This enum maps to the error codes defined in the Play In-App Updates API in
 * https://developer.android.com/reference/native/play/core/group/appupdate#appupdatetype
 */
UENUM(BlueprintType)
enum class EAppUpdateType : uint8
{
	// The app update type is unknown or unspecified.
	AppUpdate_TYPE_UNKNOWN,

	// Flexible update flow, where the user can still use the app while the
	// update is downloaded.
	AppUpdate_TYPE_FLEXIBLE,

	// Immediate update flow, where the user is unable to interact with the app
	// while the update is downloaded and installed.
	AppUpdate_TYPE_IMMEDIATE,
};

/**
 * A utility namespace that provides error code conversion functions for Google
 * Play In-App Updates.
 */
namespace InAppUpdatesError
{
#if SUPPORTED_PLATFORM
EAppUpdateErrorCode ConvertAppUpdateErrorCode(AppUpdateErrorCode Code);
EAppUpdateStatus ConvertAppUpdateStatus(AppUpdateStatus Code);
EAppUpdateAvailability ConvertAppUpdateAvailability(AppUpdateAvailability Code);
EAppUpdateType ConvertAppUpdateType(AppUpdateType Code);
AppUpdateType ConvertNativeAppUpdateType(EAppUpdateType Code);
#endif  // SUPPORTED_PLATFORM
}  // namespace InAppUpdatesError

/** @} */
