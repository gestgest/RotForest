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

#include "InAppUpdatesError.h"

#if SUPPORTED_PLATFORM
EAppUpdateErrorCode InAppUpdatesError::ConvertAppUpdateErrorCode(
	AppUpdateErrorCode Code)
{
	switch (Code)
	{
		case APP_UPDATE_NO_ERROR:
			return EAppUpdateErrorCode::AppUpdate_NO_ERROR;
		case APP_UPDATE_UNKNOWN_ERROR:
			return EAppUpdateErrorCode::AppUpdate_UNKNOWN_ERROR;
		case APP_UPDATE_API_NOT_AVAILABLE:
			return EAppUpdateErrorCode::AppUpdate_API_NOT_AVAILABLE;
		case APP_UPDATE_INVALID_REQUEST:
			return EAppUpdateErrorCode::AppUpdate_INVALID_REQUEST;
		case APP_UPDATE_UNAVAILABLE:
			return EAppUpdateErrorCode::AppUpdate_UNAVAILABLE;
		case APP_UPDATE_NOT_ALLOWED:
			return EAppUpdateErrorCode::AppUpdate_NOT_ALLOWED;
		case APP_UPDATE_DOWNLOAD_NOT_PRESENT:
			return EAppUpdateErrorCode::AppUpdate_DOWNLOAD_NOT_PRESENT;
		case APP_UPDATE_IN_PROGRESS:
			return EAppUpdateErrorCode::AppUpdate_IN_PROGRESS;
		case APP_UPDATE_PLAY_STORE_NOT_FOUND:
			return EAppUpdateErrorCode::AppUpdate_PLAY_STORE_NOT_FOUND;
		case APP_UPDATE_APP_NOT_OWNED:
			return EAppUpdateErrorCode::AppUpdate_APP_NOT_OWNED;
		case APP_UPDATE_INTERNAL_ERROR:
			return EAppUpdateErrorCode::AppUpdate_INTERNAL_ERROR;
		case APP_UPDATE_INITIALIZATION_NEEDED:
			return EAppUpdateErrorCode::AppUpdate_INITIALIZATION_NEEDED;
		case APP_UPDATE_INITIALIZATION_FAILED:
			return EAppUpdateErrorCode::AppUpdate_INITIALIZATION_FAILED;
		default:
			return EAppUpdateErrorCode::AppUpdate_UNKNOWN_ERROR;
	}
}

EAppUpdateStatus InAppUpdatesError::ConvertAppUpdateStatus(AppUpdateStatus Code)
{
	switch (Code)
	{
		case APP_UPDATE_STATUS_UNKNOWN:
			return EAppUpdateStatus::AppUpdate_STATUS_UNKNOWN;
		case APP_UPDATE_PENDING:
			return EAppUpdateStatus::AppUpdate_PENDING;
		case APP_UPDATE_DOWNLOADING:
			return EAppUpdateStatus::AppUpdate_DOWNLOADING;
		case APP_UPDATE_INSTALLING:
			return EAppUpdateStatus::AppUpdate_INSTALLING;
		case APP_UPDATE_INSTALLED:
			return EAppUpdateStatus::AppUpdate_INSTALLED;
		case APP_UPDATE_FAILED:
			return EAppUpdateStatus::AppUpdate_FAILED;
		case APP_UPDATE_CANCELED:
			return EAppUpdateStatus::AppUpdate_CANCELED;
		case APP_UPDATE_DOWNLOADED:
			return EAppUpdateStatus::AppUpdate_DOWNLOADED;
		case APP_UPDATE_REQUEST_INFO_PENDING:
			return EAppUpdateStatus::AppUpdate_REQUEST_INFO_PENDING;
		case APP_UPDATE_REQUEST_INFO_FAILED:
			return EAppUpdateStatus::AppUpdate_REQUEST_INFO_FAILED;
		case APP_UPDATE_REQUEST_INFO_COMPLETED:
			return EAppUpdateStatus::AppUpdate_REQUEST_INFO_COMPLETED;
		case APP_UPDATE_REQUEST_START_UPDATE_PENDING:
			return EAppUpdateStatus::AppUpdate_REQUEST_START_UPDATE_PENDING;
		case APP_UPDATE_REQUEST_COMPLETE_UPDATE_PENDING:
			return EAppUpdateStatus::AppUpdate_REQUEST_COMPLETE_UPDATE_PENDING;
		default:
			return EAppUpdateStatus::AppUpdate_STATUS_UNKNOWN;
	}
}

EAppUpdateAvailability InAppUpdatesError::ConvertAppUpdateAvailability(
	AppUpdateAvailability Code)
{
	switch (Code)
	{
		case APP_UPDATE_AVAILABLILITY_UNKNOWN:
			return EAppUpdateAvailability::AppUpdate_AVAILABILITY_UNKNOWN;
		case APP_UPDATE_NOT_AVAILABLE:
			return EAppUpdateAvailability::AppUpdate_NOT_AVAILABLE;
		case APP_UPDATE_AVAILABLE:
			return EAppUpdateAvailability::AppUpdate_AVAILABLE;
		case APP_UPDATE_TRIGGERED_IN_PROGRESS:
			return EAppUpdateAvailability::AppUpdate_TRIGGERED_IN_PROGRESS;
		default:
			return EAppUpdateAvailability::AppUpdate_AVAILABILITY_UNKNOWN;
	}
}

EAppUpdateType InAppUpdatesError::ConvertAppUpdateType(AppUpdateType Code)
{
	switch (Code)
	{
		case APP_UPDATE_TYPE_UNKNOWN:
			return EAppUpdateType::AppUpdate_TYPE_UNKNOWN;
		case APP_UPDATE_TYPE_FLEXIBLE:
			return EAppUpdateType::AppUpdate_TYPE_FLEXIBLE;
		case APP_UPDATE_TYPE_IMMEDIATE:
			return EAppUpdateType::AppUpdate_TYPE_IMMEDIATE;
		default:
			return EAppUpdateType::AppUpdate_TYPE_UNKNOWN;
	}
}

AppUpdateType InAppUpdatesError::ConvertNativeAppUpdateType(EAppUpdateType Code)
{
	switch (Code)
	{
		case EAppUpdateType::AppUpdate_TYPE_FLEXIBLE:
			return APP_UPDATE_TYPE_FLEXIBLE;
		case EAppUpdateType::AppUpdate_TYPE_IMMEDIATE:
			return APP_UPDATE_TYPE_IMMEDIATE;
		default:
			return APP_UPDATE_TYPE_UNKNOWN;
	}
}
#endif
