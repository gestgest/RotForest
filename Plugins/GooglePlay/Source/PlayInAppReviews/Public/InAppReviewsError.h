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

#include "InAppReviewsError.generated.h"

#define SUPPORTED_PLATFORM (PLATFORM_ANDROID && USE_ANDROID_JNI)

#if SUPPORTED_PLATFORM
#include "play/review.h"
#endif  // SUPPORTED_PLATFORM

/**
 * @ingroup PlayInAppReviews
 *
 * Errors that can be encountered while using the In-App Review API.
 * This enum maps to the error codes defined in the In-App Review API in
 * https://developer.android.com/reference/native/play/core/group/review#reviewerrorcode
 */
UENUM(BlueprintType)
enum class EInAppReviewErrorCode : uint8
{
	// No error has occurred.
	Review_NO_ERROR,

	// The requested operation failed: need to call ReviewManager_init() first.
	Review_INITIALIZATION_NEEDED,

	// Error initializing dependencies.
	Review_INITIALIZATION_FAILED,

	// The requested operation failed: need to call
	// ReviewManager_requestReviewFlow() first.
	Review_REQUEST_FLOW_NEEDED,

	// ReviewManager_requestReviewFlow() failed.
	Review_REQUEST_FLOW_FAILED,

	// An invalid parameter was passed to the function.
	Review_INVALID_REQUEST,

	// The Play Store app is either not installed or not the official version.
	Review_PLAY_STORE_NOT_FOUND,

	// Unknown state used for initialization.
	Review_UNKNOWN,

	// The request operation was canceled.
	Review_REQUEST_CANCELED,
};

/**
 * @ingroup PlayInAppReviews
 *
 * Status returned when requesting or launching the in-app review flow.
 * This enum maps to the status codes defined in the In-App Review API in
 * https://developer.android.com/reference/native/play/core/group/review#reviewstatus
 */
UENUM(BlueprintType)
enum class EInAppReviewStatus : uint8
{
	// Unknown review status.
	Review_STATUS_UNKNOWN,

	// Waiting for ReviewManager_requestReviewFlow() asynchronous operation to
	// finish.
	Review_REQUEST_FLOW_PENDING,

	// ReviewManager_requestReviewFlow() asynchronous operation has finished.
	Review_REQUEST_FLOW_COMPLETED,

	// Waiting for ReviewManager_launchReviewFlow() asynchronous operation to
	// finish.
	Review_LAUNCH_FLOW_PENDING,

	// ReviewManager_launchReviewFlow() asynchronous operation has finished,
	// and it will receive this status regardless of whether the user completed
	// the review or the user dismissed the dialog.
	Review_LAUNCH_FLOW_COMPLETED,
};

/**
 * A utility namespace that provides error code conversion functions for In-App
 * Review.
 */
namespace InAppReviewsError
{
#if SUPPORTED_PLATFORM
EInAppReviewErrorCode ConvertReviewErrorCode(ReviewErrorCode Code);
EInAppReviewStatus ConvertReviewStatus(ReviewStatus Code);
#endif  // SUPPORTED_PLATFORM
}  // namespace InAppReviewsError

/** @} */
