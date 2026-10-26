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

#include "IntegrityError.h"

#if SUPPORTED_PLATFORM
EIntegrityErrorCode IntegrityError::ConvertErrorCode(IntegrityErrorCode Code)
{
	switch (Code)
	{
		case INTEGRITY_NO_ERROR:
			return EIntegrityErrorCode::Integrity_NO_ERROR;
		case INTEGRITY_API_NOT_AVAILABLE:
			return EIntegrityErrorCode::Integrity_API_NOT_AVAILABLE;
		case INTEGRITY_PLAY_STORE_NOT_FOUND:
			return EIntegrityErrorCode::Integrity_PLAY_STORE_NOT_FOUND;
		case INTEGRITY_NETWORK_ERROR:
			return EIntegrityErrorCode::Integrity_NETWORK_ERROR;
		case INTEGRITY_PLAY_STORE_ACCOUNT_NOT_FOUND:
			return EIntegrityErrorCode::Integrity_PLAY_STORE_ACCOUNT_NOT_FOUND;
		case INTEGRITY_APP_NOT_INSTALLED:
			return EIntegrityErrorCode::Integrity_APP_NOT_INSTALLED;
		case INTEGRITY_PLAY_SERVICES_NOT_FOUND:
			return EIntegrityErrorCode::Integrity_PLAY_SERVICES_NOT_FOUND;
		case INTEGRITY_APP_UID_MISMATCH:
			return EIntegrityErrorCode::Integrity_APP_UID_MISMATCH;
		case INTEGRITY_TOO_MANY_REQUESTS:
			return EIntegrityErrorCode::Integrity_TOO_MANY_REQUESTS;
		case INTEGRITY_CANNOT_BIND_TO_SERVICE:
			return EIntegrityErrorCode::Integrity_CANNOT_BIND_TO_SERVICE;
		case INTEGRITY_NONCE_TOO_SHORT:
			return EIntegrityErrorCode::Integrity_NONCE_TOO_SHORT;
		case INTEGRITY_NONCE_TOO_LONG:
			return EIntegrityErrorCode::Integrity_NONCE_TOO_LONG;
		case INTEGRITY_GOOGLE_SERVER_UNAVAILABLE:
			return EIntegrityErrorCode::Integrity_GOOGLE_SERVER_UNAVAILABLE;
		case INTEGRITY_NONCE_IS_NOT_BASE64:
			return EIntegrityErrorCode::Integrity_NONCE_IS_NOT_BASE64;
		case INTEGRITY_PLAY_STORE_VERSION_OUTDATED:
			return EIntegrityErrorCode::Integrity_PLAY_STORE_VERSION_OUTDATED;
		case INTEGRITY_PLAY_SERVICES_VERSION_OUTDATED:
			return EIntegrityErrorCode::
				Integrity_PLAY_SERVICES_VERSION_OUTDATED;
		case INTEGRITY_CLOUD_PROJECT_NUMBER_IS_INVALID:
			return EIntegrityErrorCode::
				Integrity_CLOUD_PROJECT_NUMBER_IS_INVALID;
		case INTEGRITY_CLIENT_TRANSIENT_ERROR:
			return EIntegrityErrorCode::Integrity_CLIENT_TRANSIENT_ERROR;
		case INTEGRITY_INTERNAL_ERROR:
			return EIntegrityErrorCode::Integrity_INTERNAL_ERROR;
		case INTEGRITY_INITIALIZATION_NEEDED:
			return EIntegrityErrorCode::Integrity_INITIALIZATION_NEEDED;
		case INTEGRITY_INITIALIZATION_FAILED:
			return EIntegrityErrorCode::Integrity_INITIALIZATION_FAILED;
		case INTEGRITY_INVALID_ARGUMENT:
			return EIntegrityErrorCode::Integrity_INVALID_ARGUMENT;
		default:
			return EIntegrityErrorCode::Integrity_UNKNOWN;
	}
}

EStandardIntegrityErrorCode IntegrityError::ConvertStandardErrorCode(
	StandardIntegrityErrorCode Code)
{
	switch (Code)
	{
		case STANDARD_INTEGRITY_NO_ERROR:
			return EStandardIntegrityErrorCode::StandardIntegrity_NO_ERROR;
		case STANDARD_INTEGRITY_API_NOT_AVAILABLE:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_API_NOT_AVAILABLE;
		case STANDARD_INTEGRITY_PLAY_STORE_NOT_FOUND:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_PLAY_STORE_NOT_FOUND;
		case STANDARD_INTEGRITY_NETWORK_ERROR:
			return EStandardIntegrityErrorCode::StandardIntegrity_NETWORK_ERROR;
		case STANDARD_INTEGRITY_APP_NOT_INSTALLED:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_APP_NOT_INSTALLED;
		case STANDARD_INTEGRITY_PLAY_SERVICES_NOT_FOUND:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_PLAY_SERVICES_NOT_FOUND;
		case STANDARD_INTEGRITY_APP_UID_MISMATCH:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_APP_UID_MISMATCH;
		case STANDARD_INTEGRITY_TOO_MANY_REQUESTS:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_TOO_MANY_REQUESTS;
		case STANDARD_INTEGRITY_CANNOT_BIND_TO_SERVICE:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_CANNOT_BIND_TO_SERVICE;
		case STANDARD_INTEGRITY_GOOGLE_SERVER_UNAVAILABLE:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_GOOGLE_SERVER_UNAVAILABLE;
		case STANDARD_INTEGRITY_PLAY_STORE_VERSION_OUTDATED:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_PLAY_STORE_VERSION_OUTDATED;
		case STANDARD_INTEGRITY_PLAY_SERVICES_VERSION_OUTDATED:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_PLAY_SERVICES_VERSION_OUTDATED;
		case CLOUD_PROJECT_NUMBER_IS_INVALID:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_CLOUD_PROJECT_NUMBER_IS_INVALID;
		case STANDARD_REQUEST_HASH_TOO_LONG:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_REQUEST_HASH_TOO_LONG;
		case STANDARD_CLIENT_TRANSIENT_ERROR:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_CLIENT_TRANSIENT_ERROR;
		case STANDARD_INTEGRITY_TOKEN_PROVIDER_INVALID:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_INTEGRITY_TOKEN_PROVIDER_INVALID;
		case STANDARD_INTEGRITY_INTERNAL_ERROR:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_INTERNAL_ERROR;
		case STANDARD_INTEGRITY_INITIALIZATION_NEEDED:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_INITIALIZATION_NEEDED;
		case STANDARD_INTEGRITY_INITIALIZATION_FAILED:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_INITIALIZATION_FAILED;
		case STANDARD_INTEGRITY_INVALID_ARGUMENT:
			return EStandardIntegrityErrorCode::
				StandardIntegrity_INVALID_ARGUMENT;
		default:
			return EStandardIntegrityErrorCode::StandardIntegrity_UNKNOWN;
	}
}

EIntegrityDialogResponseCode IntegrityError::ConvertIntegrityDialogResponseCode(
	IntegrityDialogResponseCode Code)
{
	switch (Code)
	{
		case INTEGRITY_DIALOG_UNAVAILABLE:
			return EIntegrityDialogResponseCode::IntegrityDialog_UNAVAILABLE;
		case INTEGRITY_DIALOG_FAILED:
			return EIntegrityDialogResponseCode::IntegrityDialog_FAILED;
		case INTEGRITY_DIALOG_CANCELLED:
			return EIntegrityDialogResponseCode::IntegrityDialog_CANCELED;
		case INTEGRITY_DIALOG_SUCCESSFUL:
			return EIntegrityDialogResponseCode::IntegrityDialog_SUCCESSFUL;
		default:
			return EIntegrityDialogResponseCode::IntegrityDialog_UNKNOWN;
	}
}

#endif  // SUPPORTED_PLATFORM
