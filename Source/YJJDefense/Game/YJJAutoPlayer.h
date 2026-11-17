#pragma once

#include "Game/YJJDefenseSim.h"

// 사람 대신 한 판을 플레이하는 봇. 시뮬레이션만 건드리므로 자동화 테스트(밸런스 확인)와
// 실제 게임의 오토플레이(yjj.AutoPlay, 스모크 테스트)에서 같이 쓴다
class YJJDEFENSE_API FYJJAutoPlayer
{
public:
	void Reset();

	// Sim.Step() 한 번마다 한 번 호출
	void Tick(FYJJDefenseSim& Sim);

	// false면 스펠을 전혀 쓰지 않는다 (스펠이 실제로 도움이 되는지 비교용)
	bool bUseSpells = true;
	// true면 주기적으로 디스크의 룬을 바꿔 끼워 새 조합을 시험한다
	bool bExperiment = true;

private:
	void TickEconomy(FYJJDefenseSim& Sim);
	void TickSpells(FYJJDefenseSim& Sim);
	void TickForge(FYJJDefenseSim& Sim);

	float ThinkTimer = 0.f;
	float ForgeTimer = 0.f;
	int32 BuildIndex = 0;
	int32 ForgeCursor = 0;
};
