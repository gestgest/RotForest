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

public class PlayInAppUpdates : ModuleRules
{
	public PlayInAppUpdates(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePathModuleNames.AddRange(
			new string[] { "Launch" }
		);

		PublicDependencyModuleNames.AddRange(
			new string[] { "Core", "Engine", "PlayCommon" }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] { "CoreUObject" }
		);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			var pluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(pluginPath, "PlayInAppUpdates_APL.xml"));
		}
	}
}
