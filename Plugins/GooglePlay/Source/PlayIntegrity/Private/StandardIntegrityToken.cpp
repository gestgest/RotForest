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

#include "StandardIntegrityToken.h"

#include "PlayIntegrity.h"
#include "StandardIntegrityManager.h"
#include "TimerManager.h"

#if SUPPORTED_PLATFORM
extern jobject GGameActivityThis;
#endif  // SUPPORTED_PLATFORM

#if SUPPORTED_PLATFORM
void UStandardIntegrityToken::Initialize(StandardIntegrityToken* InNativeToken)
{
	if (InNativeToken == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Failed to initialize UStandardIntegrityToken because "
				 "NativeToken is null."));
		return;
	}
	NativeToken = InNativeToken;
	Token = StandardIntegrityToken_getToken(NativeToken);
	HasCalledShowDialog = false;
}
#endif  // SUPPORTED_PLATFORM

void UStandardIntegrityToken::BeginDestroy()
{
#if SUPPORTED_PLATFORM
	if (NativeToken != nullptr)
	{
		StandardIntegrityToken_destroy(NativeToken);
		NativeToken = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending integrity token..."));
	}
#endif  // SUPPORTED_PLATFORM
	Super::BeginDestroy();
}

void UStandardIntegrityToken::ShowDialog(int TypeCode,
	FShowDialogStandardOperationCompletedDelegate
		InOnShowDialogOperationCompleted)
{
	if (OnShowDialogOperationCompleted.IsBound())
	{
		OnShowDialogOperationCompleted.ExecuteIfBound(
			EStandardIntegrityErrorCode::StandardIntegrity_REQUEST_CANCELED,
			EIntegrityDialogResponseCode::IntegrityDialog_CANCELED);
	}

#if SUPPORTED_PLATFORM
	if (HasCalledShowDialog)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("You can only call ShowDialog once per token."));
		return;
	}

	HasCalledShowDialog = true;
	OnShowDialogOperationCompleted = InOnShowDialogOperationCompleted;
	if (NativeToken == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Before requesting to show a dialog, please request the "
				 "integrity token first."));

		OnShowDialogOperationCompleted.ExecuteIfBound(
			EStandardIntegrityErrorCode::StandardIntegrity_INVALID_ARGUMENT,
			EIntegrityDialogResponseCode::IntegrityDialog_FAILED);
		OnShowDialogOperationCompleted.Clear();
		return;
	}

	StandardIntegrityErrorCode ErrorCode = StandardIntegrityToken_showDialog(
		NativeToken, GGameActivityThis, TypeCode);

	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		OnShowDialogOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode),
			EIntegrityDialogResponseCode::IntegrityDialog_FAILED);
		OnShowDialogOperationCompleted.Clear();
		return;
	}

	GetTypedOuter<UStandardIntegrityManager>()
		->GetGameInstance()
		->GetTimerManager()
		.SetTimer(PollingTimerHandle, this,
			&UStandardIntegrityToken::HandleCheckDialogStatusTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnShowDialogOperationCompleted.ExecuteIfBound(
		EStandardIntegrityErrorCode::StandardIntegrity_NO_ERROR,
		EIntegrityDialogResponseCode::IntegrityDialog_UNKNOWN);
	OnShowDialogOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityToken::HandleCheckDialogStatusTimer()
{
#if SUPPORTED_PLATFORM
	IntegrityDialogResponseCode ResponseCode;
	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityToken_getDialogResponseCode(
			NativeToken, &ResponseCode);

	if (ErrorCode == STANDARD_INTEGRITY_NO_ERROR &&
		ResponseCode == INTEGRITY_DIALOG_RESPONSE_UNKNOWN)
	{
		GetTypedOuter<UStandardIntegrityManager>()
			->GetGameInstance()
			->GetTimerManager()
			.SetTimer(PollingTimerHandle, this,
				&UStandardIntegrityToken::HandleCheckDialogStatusTimer,
				/* InRate= */ 0.1f,
				/* InbLoop= */ false);
		return;
	}

	OnShowDialogOperationCompleted.ExecuteIfBound(
		IntegrityError::ConvertStandardErrorCode(ErrorCode),
		IntegrityError::ConvertIntegrityDialogResponseCode(ResponseCode));
	OnShowDialogOperationCompleted.Clear();
	StandardIntegrityToken_destroy(NativeToken);
	NativeToken = nullptr;
#endif  // SUPPORTED_PLATFORM
}
