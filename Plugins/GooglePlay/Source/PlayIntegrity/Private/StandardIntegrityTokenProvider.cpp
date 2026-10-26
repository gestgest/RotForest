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

#include "StandardIntegrityTokenProvider.h"

#include "IntegrityError.h"
#include "PlayIntegrity.h"
#include "StandardIntegrityManager.h"
#include "StandardIntegrityToken.h"
#include "StandardIntegrityTokenRequest.h"

#if SUPPORTED_PLATFORM
void UStandardIntegrityTokenProvider::Initialize(
	StandardIntegrityTokenProvider* InNativeTokenProvider)
{
	if (InNativeTokenProvider == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Failed to initialize UStandardIntegrityTokenProvider "
				 "because "
				 "NativeTokenProvider is null."));
		return;
	}
	NativeTokenProvider = InNativeTokenProvider;
}
#endif  // SUPPORTED_PLATFORM

void UStandardIntegrityTokenProvider::BeginDestroy()
{
	ClearPendingRequest(/* DestroyProvider= */ true);
	Super::BeginDestroy();
}

void UStandardIntegrityTokenProvider::Request(
	const FStandardIntegrityTokenRequest& Request,
	FStandardRequestOperationCompletedDelegate
		InOnStandardRequestOperationCompleted)
{
	if (OnStandardRequestOperationCompleted.IsBound())
	{
		OnStandardRequestOperationCompleted.ExecuteIfBound(
			EStandardIntegrityErrorCode::StandardIntegrity_REQUEST_CANCELED,
			nullptr);
	}

#if SUPPORTED_PLATFORM
	OnStandardRequestOperationCompleted = InOnStandardRequestOperationCompleted;
	if (NativeTokenProvider == nullptr)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("Before requesting standard integrity token, please call "
				 "PrepareIntegrityToken first."));

		OnStandardRequestOperationCompleted.ExecuteIfBound(
			EStandardIntegrityErrorCode::
				StandardIntegrity_INTEGRITY_TOKEN_PROVIDER_INVALID,
			nullptr);
		OnStandardRequestOperationCompleted.Clear();
		return;
	}

	ClearPendingRequest();
	StandardIntegrityTokenRequest_create(&NativeStandardRequest);
	if (Request.RequestHash.IsEmpty())
	{
		StandardIntegrityTokenRequest_setRequestHash(
			NativeStandardRequest, TCHAR_TO_ANSI(*Request.RequestHash));
	}

	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityTokenProvider_request(
			NativeTokenProvider, NativeStandardRequest, &NativeToken);

	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		OnStandardRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), nullptr);
		OnStandardRequestOperationCompleted.Clear();
		return;
	}

	GetTypedOuter<UStandardIntegrityManager>()
		->GetGameInstance()
		->GetTimerManager()
		.SetTimer(PollingTimerHandle, this,
			&UStandardIntegrityTokenProvider::HandleCheckTokenStatusTimer,
			/* InRate= */ 0.1f, /* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnStandardRequestOperationCompleted.ExecuteIfBound(
		EStandardIntegrityErrorCode::StandardIntegrity_NO_ERROR, nullptr);
	OnStandardRequestOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityTokenProvider::HandleCheckTokenStatusTimer()
{
#if SUPPORTED_PLATFORM
	IntegrityResponseStatus TokenStatus;
	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityToken_getStatus(NativeToken, &TokenStatus);

	if (ErrorCode == STANDARD_INTEGRITY_NO_ERROR &&
		TokenStatus == INTEGRITY_RESPONSE_PENDING)
	{
		GetTypedOuter<UStandardIntegrityManager>()
			->GetGameInstance()
			->GetTimerManager()
			.SetTimer(PollingTimerHandle, this,
				&UStandardIntegrityTokenProvider::HandleCheckTokenStatusTimer,
				/* InRate= */ 0.1f, /* InbLoop= */ false);
		return;
	}

	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		OnStandardRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), nullptr);
	}
	else
	{
		// We use the StandardIntegrityManager as the outer because it has
		// access to the game instance.
		UStandardIntegrityToken* Response = NewObject<UStandardIntegrityToken>(
			GetTypedOuter<UStandardIntegrityManager>());
		Response->Initialize(NativeToken);
		NativeToken = nullptr;

		OnStandardRequestOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), Response);
	}

	OnStandardRequestOperationCompleted.Clear();
	StandardIntegrityTokenRequest_destroy(NativeStandardRequest);
	NativeStandardRequest = nullptr;
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityTokenProvider::ClearPendingRequest(bool DestroyProvider)
{
#if SUPPORTED_PLATFORM
	if (NativeStandardRequest != nullptr)
	{
		StandardIntegrityTokenRequest_destroy(NativeStandardRequest);
		NativeStandardRequest = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending standard integrity request..."));
	}
	if (NativeToken != nullptr)
	{
		StandardIntegrityToken_destroy(NativeToken);
		NativeToken = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending standard integrity token..."));
	}
	if (DestroyProvider && NativeTokenProvider != nullptr)
	{
		StandardIntegrityTokenProvider_destroy(NativeTokenProvider);
		NativeTokenProvider = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing standard integrity token provider..."));
	}
#endif
}