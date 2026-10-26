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

#include "StandardIntegrityTokenRequest.generated.h"

/**
 * @ingroup PlayIntegrity
 * @{
 */

/**
 * The request used to call the Request function.
 *
 * @see UStandardIntegrityTokenProvider::Request
 */
USTRUCT(BlueprintType)
struct PLAYINTEGRITY_API FStandardIntegrityTokenRequest
{
	GENERATED_BODY()

	/**
	 * The request hash provided to the API.
	 *
	 * It is the request hash to bind the integrity token to. It is a
	 * recommended but not required field.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Google Play Integrity")
	FString RequestHash;
};

/** @} */
