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

#include "IntegrityManager.h"

#include "IntegrityError.h"
#include "IntegrityTokenRequest.h"
#include "IntegrityTokenResponse.h"
#include "PlayIntegrity.h"

#if SUPPORTED_PLATFORM
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"

extern JavaVM* GJavaVM;
extern jobject GGameActivityThis;
#endif  // SUPPORTED_PLATFORM

void UIntegrityManager::Initialize(FSubsystemCollectionBase& Collection)
{
#if SUPPORTED_PLATFORM
	IntegrityErrorCode ErrorCode =
		IntegrityManager_init(GJavaVM, GGameActivityThis);
	if (ErrorCode != INTEGRITY_NO_ERROR)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("IntegrityManager_init failed: %s"),
			*UEnum::GetValueAsName(IntegrityError::ConvertErrorCode(ErrorCode))
				.ToString());
	}
#endif  // SUPPORTED_PLATFORM
}

void UIntegrityManager::Deinitialize()
{
#if SUPPORTED_PLATFORM
	ClearPendingRequest();
	IntegrityManager_destroy();
#endif  // SUPPORTED_PLATFORM
}

void UIntegrityManager::RequestIntegrityToken(
	const FIntegrityTokenRequest& Request,
	FClassicRequestOperationCompletedDelegate
		InOnClassicRequestOperationCompleted)
{
	if (OnClassicRequestOperationCompleted.IsBound())
	{
		OnClassicRequestOperationCompleted.ExecuteIfBound(
			EIntegrityErrorCode::Integrity_REQUEST_CANCELED, nullptr);
	}

#if SUPPORTED_PLATFORM
	ClearPendingRequest();
	OnClassicRequestOperationCompleted = InOnClassicRequestOperationCompleted;
	IntegrityTokenRequest_create(&NativeTokenRequest);
	IntegrityTokenRequest_setNonce(
		NativeTokenRequest, TCHAR_TO_ANSI(*Request.Nonce));
	if (Request.CloudProjectNumber > 0)
	{
		IntegrityTokenRequest_setCloudProjectNumber(
			NativeTokenRequest, Request.CloudProjectNumber);
	}

	IntegrityErrorCode ErrorCode = IntegrityManager_requestIntegrityToken(
		NativeTokenRequest, &NativeTokenResponse);
	if (ErrorCode != INTEGRITY_NO_ERROR)
	{
		OnClassicRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertErrorCode(ErrorCode), nullptr);
		OnClassicRequestOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(PollingTimerHandle, this,
		&UIntegrityManager::HandleCheckResponseStatusTimer,
		/* InRate= */ 0.1f, /* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnClassicRequestOperationCompleted.ExecuteIfBound(
		EIntegrityErrorCode::Integrity_NO_ERROR, nullptr);
	OnClassicRequestOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UIntegrityManager::HandleCheckResponseStatusTimer()
{
#if SUPPORTED_PLATFORM
	IntegrityResponseStatus IntegrityResponseStatus;
	IntegrityErrorCode ErrorCode = IntegrityTokenResponse_getStatus(
		NativeTokenResponse, &IntegrityResponseStatus);

	if (ErrorCode == INTEGRITY_NO_ERROR &&
		IntegrityResponseStatus == INTEGRITY_RESPONSE_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(PollingTimerHandle, this,
			&UIntegrityManager::HandleCheckResponseStatusTimer,
			/* InRate= */ 0.1f, /* InbLoop= */ false);
		return;
	}

	if (ErrorCode != INTEGRITY_NO_ERROR)
	{
		OnClassicRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertErrorCode(ErrorCode), nullptr);
	}
	else
	{
		UIntegrityTokenResponse* Response =
			NewObject<UIntegrityTokenResponse>(this);
		Response->Initialize(NativeTokenResponse);
		NativeTokenResponse = nullptr;

		OnClassicRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertErrorCode(ErrorCode), Response);
	}

	OnClassicRequestOperationCompleted.Clear();
	IntegrityTokenRequest_destroy(NativeTokenRequest);
	NativeTokenRequest = nullptr;
#endif  // SUPPORTED_PLATFORM
}

void UIntegrityManager::ClearPendingRequest()
{
#if SUPPORTED_PLATFORM
	if (NativeTokenRequest != nullptr)
	{
		IntegrityTokenRequest_destroy(NativeTokenRequest);
		NativeTokenRequest = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending integrity request..."));
	}
	if (NativeTokenResponse != nullptr)
	{
		IntegrityTokenResponse_destroy(NativeTokenResponse);
		NativeTokenResponse = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending integrity response..."));
	}
#endif  // SUPPORTED_PLATFORM
}
