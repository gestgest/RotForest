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

#include "StandardIntegrityManager.h"

#include "IntegrityError.h"
#include "PlayIntegrity.h"
#include "PrepareIntegrityTokenRequest.h"
#include "StandardIntegrityTokenProvider.h"
#include "TimerManager.h"

#if SUPPORTED_PLATFORM
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"

extern JavaVM* GJavaVM;
extern jobject GGameActivityThis;
#endif

void UStandardIntegrityManager::Initialize(FSubsystemCollectionBase& Collection)
{
#if SUPPORTED_PLATFORM
	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityManager_init(GJavaVM, GGameActivityThis);
	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		UE_LOG(LogPlayIntegrity, Error,
			TEXT("StandardIntegrityManager_init failed: %s"),
			*UEnum::GetValueAsName(
				IntegrityError::ConvertStandardErrorCode(ErrorCode))
				.ToString());
	}
#endif
}

void UStandardIntegrityManager::Deinitialize()
{
#if SUPPORTED_PLATFORM
	ClearPendingRequest();
	StandardIntegrityManager_destroy();
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityManager::PrepareIntegrityToken(
	FPrepareIntegrityTokenRequest Request,
	FPrepareIntegrityOperationCompletedDelegate
		InOnPrepareIntegrityOperationCompleted)
{
	if (OnPrepareIntegrityOperationCompleted.IsBound())
	{
		OnPrepareIntegrityOperationCompleted.ExecuteIfBound(
			EStandardIntegrityErrorCode::StandardIntegrity_REQUEST_CANCELED,
			nullptr);
	}

#if SUPPORTED_PLATFORM
	ClearPendingRequest();
	OnPrepareIntegrityOperationCompleted =
		InOnPrepareIntegrityOperationCompleted;
	PrepareIntegrityTokenRequest_create(&NativeProviderRequest);
	PrepareIntegrityTokenRequest_setCloudProjectNumber(
		NativeProviderRequest, Request.CloudProjectNumber);

	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityManager_prepareIntegrityToken(
			NativeProviderRequest, &NativeTokenProvider);

	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		OnPrepareIntegrityOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), nullptr);
		OnPrepareIntegrityOperationCompleted.Clear();
		return;
	}

	GetGameInstance()->GetTimerManager().SetTimer(PollingTimerHandle, this,
		&UStandardIntegrityManager::HandleCheckProviderStatusTimer,
		/* InRate= */ 0.1f,
		/* InbLoop= */ false);
#else   // SUPPORTED_PLATFORM
	OnPrepareIntegrityOperationCompleted.ExecuteIfBound(
		EStandardIntegrityErrorCode::StandardIntegrity_NO_ERROR, nullptr);
	OnPrepareIntegrityOperationCompleted.Clear();
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityManager::HandleCheckProviderStatusTimer()
{
#if SUPPORTED_PLATFORM
	IntegrityResponseStatus TokenProviderStatus;
	StandardIntegrityErrorCode ErrorCode =
		StandardIntegrityTokenProvider_getStatus(
			NativeTokenProvider, &TokenProviderStatus);

	if (ErrorCode == STANDARD_INTEGRITY_NO_ERROR &&
		TokenProviderStatus == INTEGRITY_RESPONSE_PENDING)
	{
		GetGameInstance()->GetTimerManager().SetTimer(PollingTimerHandle, this,
			&UStandardIntegrityManager::HandleCheckProviderStatusTimer,
			/* InRate= */ 0.1f,
			/* InbLoop= */ false);
		return;
	}

	if (ErrorCode != STANDARD_INTEGRITY_NO_ERROR)
	{
		OnPrepareIntegrityOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), nullptr);
	}
	else
	{
		UStandardIntegrityTokenProvider* Provider =
			NewObject<UStandardIntegrityTokenProvider>(this);
		Provider->Initialize(NativeTokenProvider);
		NativeTokenProvider = nullptr;

		OnPrepareIntegrityOperationCompleted.ExecuteIfBound(
			IntegrityError::ConvertStandardErrorCode(ErrorCode), Provider);
	}

	OnPrepareIntegrityOperationCompleted.Clear();
	PrepareIntegrityTokenRequest_destroy(NativeProviderRequest);
	NativeProviderRequest = nullptr;
#endif  // SUPPORTED_PLATFORM
}

void UStandardIntegrityManager::ClearPendingRequest()
{
#if SUPPORTED_PLATFORM
	if (NativeProviderRequest != nullptr)
	{
		PrepareIntegrityTokenRequest_destroy(NativeProviderRequest);
		NativeProviderRequest = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing pending prepare integrity token "
				 "request..."));
	}
	if (NativeTokenProvider != nullptr)
	{
		StandardIntegrityTokenProvider_destroy(NativeTokenProvider);
		NativeTokenProvider = nullptr;
		UE_LOG(LogPlayIntegrity, Warning,
			TEXT("Destroying existing standard integrity token provider..."));
	}
#endif
}