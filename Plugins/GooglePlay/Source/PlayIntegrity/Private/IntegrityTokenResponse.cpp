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

#include "IntegrityTokenResponse.h"

#include "IntegrityManager.h"
#include "PlayIntegrity.h"
#include "TimerManager.h"

#if SUPPORTED_PLATFORM
extern jobject GGameActivityThis;
#endif  // SUPPORTED_PLATFORM

#if SUPPORTED_PLATFORM
void UIntegrityTokenResponse::Initialize(
	IntegrityTokenResponse* InNativeTokenResponse)
{
	if (InNativeTokenResponse == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Failed to initialize UIntegrityTokenResponse because "
				 "NativeTokenResponse is null."));
		return;
	}
	NativeTokenResponse = InNativeTokenResponse;
	Token = IntegrityTokenResponse_getToken(NativeTokenResponse);
	HasCalledShowDialog = false;
}
#endif  // SUPPORTED_PLATFORM

void UIntegrityTokenResponse::BeginDestroy()
{
#if SUPPORTED_PLATFORM
	if (NativeTokenResponse != nullptr)
	{
		IntegrityTokenResponse_destroy(NativeTokenResponse);
		NativeTokenResponse = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending integrity response..."));
	}
#endif  // SUPPORTED_PLATFORM
	Super::BeginDestroy();
}

void UIntegrityTokenResponse::ShowDialog(int TypeCode,
	FShowDialogOperationCompletedDelegate InOnShowDialogOperationCompleted)
{
	if (OnShowDialogOperationCompleted.IsBound())
	{
		OnShowDialogOperationCompleted.ExecuteIfBound(
			EIntegrityErrorCode::Integrity_REQUEST_CANCELED,
			EIntegrityDialogResponseCode::IntegrityDialog_CANCELED);
	}

#if SUPPORTED_PLATFORM
	if (HasCalledShowDialog)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("You can only call ShowDialog once per response."));
		return;
	}

	HasCalledShowDialog = true;
	OnShowDialogOperationCompleted = InOnShowDialogOperationCompleted;
	if (NativeTokenResponse == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Before requesting to show a dialog, please request the "
				 "integrity token first."));

		OnShowDialogOperationCompleted.ExecuteIfBound(
			EIntegrityErrorCode::Integrity_INVALID_ARGUMENT,
			EIntegrityDialogResponseCode::IntegrityDialog_FAILED);
		OnShowDialogOperationCompleted.Clear();
		return;
	}

	IntegrityErrorCode ErrorCode = IntegrityTokenResponse_showDialog(
		NativeTokenResponse, GGameActivityThis, TypeCode);

	if (ErrorCode != INTEGRITY_NO_ERROR)
	{
		OnShowDialogOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertErrorCode(ErrorCode),
			EIntegrityDialogResponseCode::IntegrityDialog_FAILED);
		OnShowDialogOperationCompleted.Clear();
		return;
	}

	GetTypedOuter<UIntegrityManager>()
		->GetGameInstance()
		->GetTimerManager()
		.SetTimer(PollingTimerHandle, this,
			&UIntegrityTokenResponse::HandleCheckDialogStatusTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnShowDialogOperationCompleted.ExecuteIfBound(
		EIntegrityErrorCode::Integrity_NO_ERROR,
		EIntegrityDialogResponseCode::IntegrityDialog_UNKNOWN);
	OnShowDialogOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UIntegrityTokenResponse::HandleCheckDialogStatusTimer()
{
#if SUPPORTED_PLATFORM
	IntegrityDialogResponseCode ResponseCode;
	IntegrityErrorCode ErrorCode = IntegrityTokenResponse_getDialogResponseCode(
		NativeTokenResponse, &ResponseCode);

	if (ErrorCode == INTEGRITY_NO_ERROR &&
		ResponseCode == INTEGRITY_DIALOG_RESPONSE_UNKNOWN)
	{
		GetTypedOuter<UIntegrityManager>()
			->GetGameInstance()
			->GetTimerManager()
			.SetTimer(PollingTimerHandle, this,
				&UIntegrityTokenResponse::HandleCheckDialogStatusTimer,
				/* InRate= */ 0.1f,
				/* InbLoop= */ false);
		return;
	}

	OnShowDialogOperationCompleted.ExecuteIfBound(
		IntegrityError::ConvertErrorCode(ErrorCode),
		IntegrityError::ConvertIntegrityDialogResponseCode(ResponseCode));
	OnShowDialogOperationCompleted.Clear();
	IntegrityTokenResponse_destroy(NativeTokenResponse);
	NativeTokenResponse = nullptr;
#endif  // SUPPORTED_PLATFORM
}
