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

using UnrealBuildTool;
using System.IO;

public class PlayCommon : ModuleRules
{
	readonly string _ndkVersion = "ndk23.0.7599858";

	public PlayCommon(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			var playCoreSdkPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "play-core-native-sdk");
			var playCoreIncludePath = Path.Combine(playCoreSdkPath, "include");
			var playCoreLibPath = Path.Combine(playCoreSdkPath, "libs");
			var pluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);

			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(pluginPath, "PlayCommon_APL.xml"));
			PublicAdditionalLibraries.Add(Path.Combine(playCoreLibPath, "arm64-v8a", _ndkVersion, "c++_static", "libplaycore_static.a"));
			PublicAdditionalLibraries.Add(Path.Combine(playCoreLibPath, "x86_64", _ndkVersion, "c++_static", "libplaycore_static.a"));
			PublicIncludePaths.Add(playCoreIncludePath);
		}
	}
}