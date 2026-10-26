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

#include "IntegrityTokenRequest.generated.h"

/**
 * @ingroup PlayIntegrity
 * @{
 */

/**
 * The request used to call the RequestIntegrityToken function.
 *
 * @see UIntegrityManager::RequestIntegrityToken
 */
USTRUCT(BlueprintType)
struct PLAYINTEGRITY_API FIntegrityTokenRequest
{
	GENERATED_BODY()

	/**
	 * The nonce field is a string, URL-safe, Base64 encoded and non-wrapping
	 * used to mitigate certain types of attacks. It must contain a minimum of
	 * 16 characters, and maximum of 500 characters.
	 *
	 * Refer to
	 * https://developer.android.com/google/play/integrity/classic#nonce for
	 * more details.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Google Play Integrity")
	FString Nonce = "";

	/**
	 * The cloud project number is a unique numerical identifier assigned to
	 * each project in Google Cloud. It can be found in Project info in your
	 * Google Cloud Console for the cloud project where Play Integrity API is
	 * enabled.
	 *
	 * For classic requests it is optional and can be omitted if it was linked
	 * in the Play Console.
	 *
	 * Calls to
	 * https://developer.android.com/google/play/integrity/classic#decrypt-verify-google-servers
	 * decrypt the token on Google's server must be authenticated using the
	 * cloud account that was linked to the token in this request.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Google Play Integrity")
	int64 CloudProjectNumber = -1;
};

/** @} */
