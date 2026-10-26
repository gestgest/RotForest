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

#include "Engine/TimerHandle.h"
#include "InAppReviewsError.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "InAppReviewsManager.generated.h"

/**
 * @defgroup PlayInAppReviews In-App Reviews
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a request or launch review flow
 * operation.
 *
 * @param Error The error code of the operation.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(
	FReviewOperationCompletedDelegate, EInAppReviewErrorCode, Error);

/**
 * The UInAppReviewsManager class is used to request and launch the in-app
 * review flow.
 *
 * Refer to https://developer.android.com/guide/playcore/in-app-review for
 * more information.
 */
UCLASS()
class PLAYINAPPREVIEWS_API UInAppReviewsManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Requests the information needed to launch the in-app review flow.
	 *
	 * @param InOnRequestFlowOperationCompleted The delegate to be called when
	 * the request flow operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Reviews")
	void RequestReviewFlow(
		FReviewOperationCompletedDelegate InOnRequestFlowOperationCompleted);

	/**
	 * Requests to display the in-app review flow to the user.
	 *
	 * @param InOnLaunchFlowOperationCompleted The delegate to be called when
	 * the launch flow operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play In-App Reviews")
	void LaunchReviewFlow(
		FReviewOperationCompletedDelegate InOnLaunchFlowOperationCompleted);

private:
	void HandleCheckRequestFlowTimer();
	void HandleCheckLaunchFlowTimer();
	FTimerHandle RequestFlowPollingTimerHandle;
	FTimerHandle LaunchFlowPollingTimerHandle;
	FReviewOperationCompletedDelegate OnRequestFlowOperationCompleted;
	FReviewOperationCompletedDelegate OnLaunchFlowOperationCompleted;
	bool HasCalledRequestFlow = false;
};

/** @} */
