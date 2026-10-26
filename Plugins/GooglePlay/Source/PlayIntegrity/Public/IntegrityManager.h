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
#include "IntegrityTokenRequest.h"
#include "IntegrityTokenResponse.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "IntegrityManager.generated.h"

/**
 * @defgroup PlayIntegrity Integrity
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a request integrity token
 * operation.
 *
 * @param Error The error code of the operation.
 * @param Response The UIntegrityTokenResponse object.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FClassicRequestOperationCompletedDelegate,
	EIntegrityErrorCode, Error, UIntegrityTokenResponse*, Response);

/**
 * The UIntegrityManager class is used to request an integrity token.
 */
UCLASS()
class PLAYINTEGRITY_API UIntegrityManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Requests an integrity token. If called while a request is pending, the
	 * pending request will be cancelled.
	 *
	 * Refer to
	 * https://developer.android.com/google/play/integrity/classic#request for
	 * more details.
	 *
	 * @param Request The FIntegrityTokenRequest object.
	 * @param InOnClassicRequestOperationCompleted The delegate to handle the
	 * completion of the request.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play Integrity")
	void RequestIntegrityToken(const FIntegrityTokenRequest& Request,
		FClassicRequestOperationCompletedDelegate
			InOnClassicRequestOperationCompleted);

private:
	void HandleCheckResponseStatusTimer();
	void ClearPendingRequest();
	FTimerHandle PollingTimerHandle;
	FClassicRequestOperationCompletedDelegate
		OnClassicRequestOperationCompleted;

#if SUPPORTED_PLATFORM
	IntegrityTokenRequest* NativeTokenRequest = nullptr;
	IntegrityTokenResponse* NativeTokenResponse = nullptr;
#endif  // SUPPORTED_PLATFORM
};

/** @} */
