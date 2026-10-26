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

#include "StandardIntegrityToken.generated.h"

/**
 * @ingroup PlayIntegrity
 * @{
 */

/**
 * @private
 *
 * The delegate used to handle the completion of a show dialog operation for
 * standard requests.
 *
 * @param Error The error code of the operation.
 * @param Response The dialog response code of the operation.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FShowDialogStandardOperationCompletedDelegate, EStandardIntegrityErrorCode,
	Error, EIntegrityDialogResponseCode, Response);

/**
 * The standard integrity token is used to verify the integrity of the app and
 * the device.
 *
 * @see UStandardIntegrityTokenProvider::Request
 */
UCLASS(BlueprintType)
class PLAYINTEGRITY_API UStandardIntegrityToken : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The token containing the signed and encrypted JSON payload.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Google Play Integrity")
	FString Token;

#if SUPPORTED_PLATFORM
	/**
	 * Initializes the UStandardIntegrityToken object and sets the native token
	 * object.
	 *
	 * @param InNativeToken The native token object.
	 */
	void Initialize(StandardIntegrityToken* InNativeToken);
#endif

	/**
	 * Part of UObject interface. Called when the object is marked for deletion
	 * by MarkPendingKill.
	 */
	virtual void BeginDestroy() override;

	/**
	 * Displays a dialog to the user. This method can only be called once per
	 * Integrity API response. This is a non-blocking call and it will return
	 * immediately.
	 *
	 * Refer to https://developer.android.com/google/play/integrity/remediation
	 * for more details.
	 *
	 * @param TypeCode The type code of the dialog to show. Refer to
	 * https://developer.android.com/google/play/integrity/reference/com/google/android/play/core/integrity/model/IntegrityDialogTypeCode
	 * for more details.
	 * @param InOnShowDialogOperationCompleted The delegate to call when the
	 * dialog is closed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Google Play Integrity")
	void ShowDialog(int TypeCode, FShowDialogStandardOperationCompletedDelegate
									  InOnShowDialogOperationCompleted);

private:
	void HandleCheckDialogStatusTimer();
	FTimerHandle PollingTimerHandle;
	FShowDialogStandardOperationCompletedDelegate
		OnShowDialogOperationCompleted;
	bool HasCalledShowDialog = false;

#if SUPPORTED_PLATFORM
	StandardIntegrityToken* NativeToken = nullptr;
#endif  // SUPPORTED_PLATFORM
};

/** @} */
