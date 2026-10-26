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

#include "InAppReviewsManager.h"

#include "InAppReviewsError.h"
#include "Misc/CoreDelegates.h"
#include "PlayInAppReviews.h"

#if SUPPORTED_PLATFORM
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"

extern JavaVM* GJavaVM;
extern jobject GGameActivityThis;
#endif  // SUPPORTED_PLATFORM

void UInAppReviewsManager::Initialize(FSubsystemCollectionBase& Collection)
{
#if SUPPORTED_PLATFORM
	ReviewErrorCode ErrorCode = ReviewManager_init(GJavaVM, GGameActivityThis);
	if (ErrorCode != REVIEW_NO_ERROR)
	{
		UE_LOG(LogPlayInAppReviews, Error,
			TEXT("ReviewManager_init failed: %s"),
			*UEnum::GetValueAsName(
				InAppReviewsError::ConvertReviewErrorCode(ErrorCode))
				.ToString());
	}
#endif  // SUPPORTED_PLATFORM
}

void UInAppReviewsManager::Deinitialize()
{
#if SUPPORTED_PLATFORM
	ReviewManager_destroy();
#endif  // SUPPORTED_PLATFORM
}

void UInAppReviewsManager::RequestReviewFlow(
	FReviewOperationCompletedDelegate InOnRequestFlowOperationCompleted)
{
	if (OnRequestFlowOperationCompleted.IsBound())
	{
		OnRequestFlowOperationCompleted.ExecuteIfBound(
			EInAppReviewErrorCode::Review_REQUEST_CANCELED);
	}

	OnRequestFlowOperationCompleted = InOnRequestFlowOperationCompleted;
#if SUPPORTED_PLATFORM
	ReviewErrorCode ErrorCode = ReviewManager_requestReviewFlow();
	if (ErrorCode != REVIEW_NO_ERROR)
	{
		OnRequestFlowOperationCompleted.ExecuteIfBound(
			InAppReviewsError::ConvertReviewErrorCode(ErrorCode));
		OnRequestFlowOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(RequestFlowPollingTimerHandle,
		this, &UInAppReviewsManager::HandleCheckRequestFlowTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnRequestFlowOperationCompleted.ExecuteIfBound(
		EInAppReviewErrorCode::Review_NO_ERROR);
	OnRequestFlowOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppReviewsManager::LaunchReviewFlow(
	FReviewOperationCompletedDelegate InOnLaunchFlowOperationCompleted)
{
	if (!HasCalledRequestFlow)
	{
		UE_LOG(LogPlayInAppReviews, Error,
			TEXT("Please call RequestReviewFlow() first."));
		InOnLaunchFlowOperationCompleted.ExecuteIfBound(
			EInAppReviewErrorCode::Review_INVALID_REQUEST);
		return;
	}

	if (OnLaunchFlowOperationCompleted.IsBound())
	{
		OnLaunchFlowOperationCompleted.ExecuteIfBound(
			EInAppReviewErrorCode::Review_REQUEST_CANCELED);
	}

	OnLaunchFlowOperationCompleted = InOnLaunchFlowOperationCompleted;
#if SUPPORTED_PLATFORM
	ReviewErrorCode ErrorCode =
		ReviewManager_launchReviewFlow(GGameActivityThis);
	if (ErrorCode != REVIEW_NO_ERROR)
	{
		OnLaunchFlowOperationCompleted.ExecuteIfBound(
			InAppReviewsError::ConvertReviewErrorCode(ErrorCode));
		OnLaunchFlowOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(LaunchFlowPollingTimerHandle,
		this, &UInAppReviewsManager::HandleCheckLaunchFlowTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnLaunchFlowOperationCompleted.ExecuteIfBound(
		EInAppReviewErrorCode::Review_NO_ERROR);
	OnLaunchFlowOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppReviewsManager::HandleCheckRequestFlowTimer()
{
#if SUPPORTED_PLATFORM
	ReviewStatus Status;
	ReviewErrorCode ErrorCode = ReviewManager_getReviewStatus(&Status);

	if (ErrorCode == REVIEW_NO_ERROR && Status == REVIEW_REQUEST_FLOW_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			RequestFlowPollingTimerHandle, this,
			&UInAppReviewsManager::HandleCheckRequestFlowTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	HasCalledRequestFlow = true;
	OnRequestFlowOperationCompleted.ExecuteIfBound(
		InAppReviewsError::ConvertReviewErrorCode(ErrorCode));
	OnRequestFlowOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UInAppReviewsManager::HandleCheckLaunchFlowTimer()
{
#if SUPPORTED_PLATFORM
	ReviewStatus Status;
	ReviewErrorCode ErrorCode = ReviewManager_getReviewStatus(&Status);

	if (ErrorCode == REVIEW_NO_ERROR && Status == REVIEW_LAUNCH_FLOW_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			LaunchFlowPollingTimerHandle, this,
			&UInAppReviewsManager::HandleCheckLaunchFlowTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	HasCalledRequestFlow = false;
	OnLaunchFlowOperationCompleted.ExecuteIfBound(
		InAppReviewsError::ConvertReviewErrorCode(ErrorCode));
	OnLaunchFlowOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}
