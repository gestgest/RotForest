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

#include "IntegrityError.generated.h"

#define SUPPORTED_PLATFORM (PLATFORM_ANDROID && USE_ANDROID_JNI)

#if SUPPORTED_PLATFORM
#include "play/integrity.h"
#endif  // SUPPORTED_PLATFORM

/**
 * @ingroup PlayIntegrity
 *
 * Errors that can be encountered while using the Integrity API
 * This enum maps to the error codes defined in the Play Integrity API in
 * https://developer.android.com/google/play/integrity/reference/com/google/android/play/core/integrity/model/IntegrityErrorCode
 */
UENUM(BlueprintType)
enum class EIntegrityErrorCode : uint8
{
	// No error has occurred.
	Integrity_NO_ERROR = 0,

	// Integrity API is not available.
	Integrity_API_NOT_AVAILABLE,

	// No Play Store app is found on device or not official version is
	// installed.
	Integrity_PLAY_STORE_NOT_FOUND,

	// No available network is found.
	Integrity_NETWORK_ERROR,

	// No Play Store account is found on device.
	Integrity_PLAY_STORE_ACCOUNT_NOT_FOUND,

	// The calling app is not installed.
	Integrity_APP_NOT_INSTALLED,

	// Play Services is not available or version is too old.
	Integrity_PLAY_SERVICES_NOT_FOUND,

	// The calling app UID (user id) does not match the one from Package
	// Manager.
	Integrity_APP_UID_MISMATCH,

	// The calling app is making too many requests to the API and hence is
	// throttled.
	Integrity_TOO_MANY_REQUESTS,

	// Binding to the service in the Play Store has failed.
	Integrity_CANNOT_BIND_TO_SERVICE,

	// Nonce length is too short.
	Integrity_NONCE_TOO_SHORT,

	// Nonce length is too long.
	Integrity_NONCE_TOO_LONG,

	// Unknown internal Google server error.
	Integrity_GOOGLE_SERVER_UNAVAILABLE,

	// Nonce is not encoded as a Base64 web-safe no-wrap string.
	Integrity_NONCE_IS_NOT_BASE64,

	// The Play Store needs to be updated.
	Integrity_PLAY_STORE_VERSION_OUTDATED,

	// Play Services needs to be updated.
	Integrity_PLAY_SERVICES_VERSION_OUTDATED,

	// The provided cloud project number is invalid.
	Integrity_CLOUD_PROJECT_NUMBER_IS_INVALID,

	// There was a transient error in the client device.
	Integrity_CLIENT_TRANSIENT_ERROR,

	// Unknown internal error.
	Integrity_INTERNAL_ERROR,

	// IntegrityManager is not initialized.
	Integrity_INITIALIZATION_NEEDED,

	// There was an error initializing the Integrity API.
	Integrity_INITIALIZATION_FAILED,

	// Invalid argument passed to the Integrity API.
	Integrity_INVALID_ARGUMENT,

	// Unknown state used for initialization.
	Integrity_UNKNOWN,

	// The previous request has been canceled in favor of a new one.
	Integrity_REQUEST_CANCELED,
};

/**
 * @ingroup PlayIntegrity
 *
 * Errors that can be encountered while using the Integrity API
 * This enum maps to the error codes defined in the Play Integrity API in
 * https://developer.android.com/google/play/integrity/reference/com/google/android/play/core/integrity/model/StandardIntegrityErrorCode
 */
UENUM(BlueprintType)
enum class EStandardIntegrityErrorCode : uint8
{
	// No error has occurred.
	StandardIntegrity_NO_ERROR = 0,

	// Standard Integrity API is not available.
	StandardIntegrity_API_NOT_AVAILABLE,

	// No Play Store app is found on device or not official version is
	// installed.
	StandardIntegrity_PLAY_STORE_NOT_FOUND,

	// No available network is found.
	StandardIntegrity_NETWORK_ERROR,

	// The calling app is not installed.
	StandardIntegrity_APP_NOT_INSTALLED,

	// Play Services is not available or version is too old.
	StandardIntegrity_PLAY_SERVICES_NOT_FOUND,

	// The calling app UID (user id) does not match the one from Package
	// Manager.
	StandardIntegrity_APP_UID_MISMATCH,

	// The calling app is making too many requests to the API and hence is
	// throttled.
	StandardIntegrity_TOO_MANY_REQUESTS,

	// Binding to the service in the Play Store has failed.
	StandardIntegrity_CANNOT_BIND_TO_SERVICE,

	// Unknown internal Google server error.
	StandardIntegrity_GOOGLE_SERVER_UNAVAILABLE,

	// The Play Store needs to be updated.
	StandardIntegrity_PLAY_STORE_VERSION_OUTDATED,

	// Play Services needs to be updated.
	StandardIntegrity_PLAY_SERVICES_VERSION_OUTDATED,

	// The provided cloud project number is invalid.
	StandardIntegrity_CLOUD_PROJECT_NUMBER_IS_INVALID,

	// The provided request hash is too long
	StandardIntegrity_REQUEST_HASH_TOO_LONG,

	// There was a transient error in the client device.
	StandardIntegrity_CLIENT_TRANSIENT_ERROR,

	// The StandardIntegrityTokenProvider is invalid.
	StandardIntegrity_INTEGRITY_TOKEN_PROVIDER_INVALID,

	// Unknown internal error.
	StandardIntegrity_INTERNAL_ERROR,

	// StandardIntegrityManager is not initialized.
	StandardIntegrity_INITIALIZATION_NEEDED,

	// There was an error initializing the Standard Integrity API
	StandardIntegrity_INITIALIZATION_FAILED,

	// Invalid argument passed to the Standard Integrity API.
	StandardIntegrity_INVALID_ARGUMENT,

	// Unknown state used for initialization.
	StandardIntegrity_UNKNOWN,

	// The previous request has been canceled in favor of a new one.
	StandardIntegrity_REQUEST_CANCELED,
};

/**
 * @ingroup PlayIntegrity
 *
 * Status returned when showing the integrity dialog.
 * This enum maps to the codes defined in the Play Integrity API in
 * https://developer.android.com/google/play/integrity/reference/com/google/android/play/core/integrity/model/IntegrityDialogResponseCode
 */
UENUM(BlueprintType)
enum class EIntegrityDialogResponseCode : uint8
{
	// The Integrity Dialog is unavailable.
	IntegrityDialog_UNAVAILABLE = 0,

	// An error occurred when trying to show the Integrity Dialog.
	IntegrityDialog_FAILED,

	// The user was shown the Integrity Dialog, but did not interact with it.
	IntegrityDialog_CANCELED,

	// The user was shown the Integrity Dialog, and successfully interacted with
	// it.
	IntegrityDialog_SUCCESSFUL,

	// Unknown state used for initialization.
	IntegrityDialog_UNKNOWN
};

/**
 * A utility namespace that provides error code conversion functions for Google
 * Play Integrity.
 */
namespace IntegrityError
{
#if SUPPORTED_PLATFORM
EIntegrityErrorCode ConvertErrorCode(IntegrityErrorCode Code);
EStandardIntegrityErrorCode ConvertStandardErrorCode(
	StandardIntegrityErrorCode Code);
EIntegrityDialogResponseCode ConvertIntegrityDialogResponseCode(
	IntegrityDialogResponseCode Code);
#endif  // SUPPORTED_PLATFORM
}  // namespace IntegrityError

/** @} */
