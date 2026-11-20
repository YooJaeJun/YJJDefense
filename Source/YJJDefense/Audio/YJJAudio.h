#pragma once

#include "CoreMinimal.h"

/**
 * 효과음·배경음. 어떤 사건에 어떤 소리를 얼마나 크게 낼지는 Content/YJJDefense/Data/Sounds.csv.
 * 소리 에셋은 Tools/Audio/generate_audio.py가 합성해서 /Game/YJJDefense/Audio에 넣는다. 없으면 조용히 넘어간다
 */
class YJJDEFENSE_API FYJJAudio
{
public:
	/** Sounds.csv 행 이름으로. 같은 소리가 한 프레임에 수십 번 겹치지 않도록 짧은 간격으로는 다시 내지 않는다 */
	static void Play(const UObject* WorldContext, FName Key);
	/** 배경음 (Sounds.csv 의 Music 행) */
	static void StartMusic(const UObject* WorldContext);

	static inline const FName ClickKey = TEXT("UI_Click");
	static inline const FName MusicKey = TEXT("Music");

private:
	static class USoundBase* LoadSound(const FString& Name);
	static constexpr const TCHAR* AudioFolder = TEXT("/Game/YJJDefense/Audio");

	/** 소리 이름 → 마지막으로 낸 시각 (실제 시간) */
	static TMap<FName, double> LastPlayed;
};
