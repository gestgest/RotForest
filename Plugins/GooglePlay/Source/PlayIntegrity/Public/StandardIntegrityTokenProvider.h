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

#include "Engine/GameInstance.h"
#include "Engine/TimerHandle.h"
#include "IntegrityError.h"
#include "StandardIntegrityToken.h"
#include "StandardIntegrityTokenRequest.h"

#include "StandardIntegrityTokenProvider.generated.h"

/**
 * @ingroup PlayIntegrity
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a standard integrity token
 * operation.
 *
 * @param Error The error code of the operation.
 * @param Response The token object.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FStandardRequestOperationCompletedDelegate,
	EStandardIntegrityErrorCode, Error, UStandardIntegrityToken*, Response);

/**
 * The Standard Integrity Token Provider is used to request an integrity token.
 */
UCLASS(BlueprintType)
class PLAYINTEGRITY_API UStandardIntegrityTokenProvider : public UObject
{
	GENERATED_BODY()

public:
#if SUPPORTED_PLATFORM
	/**
	 * Initializes the UStandardIntegrityTokenProvider object and sets the
	 * native token provider object.
	 *
	 * @param InNativeTokenProvider The native token provider object.
	 */
	void Initialize(StandardIntegrityTokenProvider* InNativeTokenProvider);
#endif  // SUPPORTED_PLATFORM

	/**
	 * Part of UObject interface. Called when the object is marked for deletion
	 * by MarkPendingKill.
	 */
	virtual void BeginDestroy() override;

	/**
	 * Returns a token for integrity-related enquiries. This must be called only
	 * after UStandardIntegrityManager::PrepareIntegrityToken completes
	 *
	 * Refer to
	 * https://developer.android.com/google/play/integrity/standard#request-integrity
	 * for more details.
	 *
	 * @param Request The FStandardIntegrityTokenRequest object.
	 * @param InOnStandardRequestOperationCompleted The delegate to call when
	 * the operation is completed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play Integrity")
	void Request(const FStandardIntegrityTokenRequest& Request,
		FStandardRequestOperationCompletedDelegate
			InOnStandardRequestOperationCompleted);

private:
	void HandleCheckTokenStatusTimer();
	void ClearPendingRequest(bool DestroyProvider = false);
	FTimerHandle PollingTimerHandle;
	FStandardRequestOperationCompletedDelegate
		OnStandardRequestOperationCompleted;

#if SUPPORTED_PLATFORM
	StandardIntegrityTokenRequest* NativeStandardRequest;
	StandardIntegrityToken* NativeToken;
	StandardIntegrityTokenProvider* NativeTokenProvider;
#endif  // SUPPORTED_PLATFORM
};

/** @} */
