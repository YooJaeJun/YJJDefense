using UnrealBuildTool;

public class YJJDefense : ModuleRules
{
	public YJJDefense(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// "Game/YJJDefenseSim.h" 처럼 모듈 루트 기준으로 include
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		// Slate: UI 테스트가 마우스·키 이벤트를 Slate에 넣는다 (Tests/YJJUITestDriver)
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
