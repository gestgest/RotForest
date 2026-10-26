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
#include "IntegrityError.h"
#include "PrepareIntegrityTokenRequest.h"
#include "StandardIntegrityTokenProvider.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "StandardIntegrityManager.generated.h"

/**
 * @ingroup PlayIntegrity
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a prepare integrity token
 * operation.
 *
 * @param Error The error code of the operation.
 * @param Response The UStandardIntegrityTokenProvider object.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPrepareIntegrityOperationCompletedDelegate,
	EStandardIntegrityErrorCode, Error, UStandardIntegrityTokenProvider*,
	Response);

/**
 * The Standard Integrity Manager is used to prepare an integrity token.
 */
UCLASS()
class PLAYINTEGRITY_API UStandardIntegrityManager
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Prepares the integrity token and makes it available for requesting. You
	 * can call this method from time to time in order to refresh the resulting
	 * StandardIntegrityTokenProvider. Note that this API makes a call to Google
	 * servers and hence requires a network connection.
	 *
	 * Refer to
	 * https://developer.android.com/google/play/integrity/standard#prepare-integrity
	 * for more details.
	 *
	 * @param Request The FPrepareIntegrityTokenRequest object.
	 * @param InOnPrepareIntegrityOperationCompleted The delegate to call when
	 * the operation is completed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play Integrity")
	void PrepareIntegrityToken(FPrepareIntegrityTokenRequest Request,
		FPrepareIntegrityOperationCompletedDelegate
			InOnPrepareIntegrityOperationCompleted);

private:
	void HandleCheckProviderStatusTimer();
	void ClearPendingRequest();
	FTimerHandle PollingTimerHandle;
	FPrepareIntegrityOperationCompletedDelegate
		OnPrepareIntegrityOperationCompleted;

#if SUPPORTED_PLATFORM
	PrepareIntegrityTokenRequest* NativeProviderRequest = nullptr;
	StandardIntegrityTokenProvider* NativeTokenProvider = nullptr;
#endif  // SUPPORTED_PLATFORM
};

/** @} */
