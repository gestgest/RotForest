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

#include "InAppReviewsError.h"

#if SUPPORTED_PLATFORM
EInAppReviewErrorCode InAppReviewsError::ConvertReviewErrorCode(
	ReviewErrorCode Code)
{
	switch (Code)
	{
		case REVIEW_NO_ERROR:
			return EInAppReviewErrorCode::Review_NO_ERROR;
		case REVIEW_INITIALIZATION_NEEDED:
			return EInAppReviewErrorCode::Review_INITIALIZATION_NEEDED;
		case REVIEW_INITIALIZATION_FAILED:
			return EInAppReviewErrorCode::Review_INITIALIZATION_FAILED;
		case REVIEW_REQUEST_FLOW_NEEDED:
			return EInAppReviewErrorCode::Review_REQUEST_FLOW_NEEDED;
		case REVIEW_REQUEST_FLOW_FAILED:
			return EInAppReviewErrorCode::Review_REQUEST_FLOW_FAILED;
		case REVIEW_INVALID_REQUEST:
			return EInAppReviewErrorCode::Review_INVALID_REQUEST;
		case REVIEW_PLAY_STORE_NOT_FOUND:
			return EInAppReviewErrorCode::Review_PLAY_STORE_NOT_FOUND;
		default:
			return EInAppReviewErrorCode::Review_UNKNOWN;
	}
}

EInAppReviewStatus InAppReviewsError::ConvertReviewStatus(ReviewStatus Code)
{
	switch (Code)
	{
		case REVIEW_STATUS_UNKNOWN:
			return EInAppReviewStatus::Review_STATUS_UNKNOWN;
		case REVIEW_REQUEST_FLOW_PENDING:
			return EInAppReviewStatus::Review_REQUEST_FLOW_PENDING;
		case REVIEW_REQUEST_FLOW_COMPLETED:
			return EInAppReviewStatus::Review_REQUEST_FLOW_COMPLETED;
		case REVIEW_LAUNCH_FLOW_PENDING:
			return EInAppReviewStatus::Review_LAUNCH_FLOW_PENDING;
		case REVIEW_LAUNCH_FLOW_COMPLETED:
			return EInAppReviewStatus::Review_LAUNCH_FLOW_COMPLETED;
		default:
			return EInAppReviewStatus::Review_STATUS_UNKNOWN;
	}
}
#endif  // SUPPORTED_PLATFORM
