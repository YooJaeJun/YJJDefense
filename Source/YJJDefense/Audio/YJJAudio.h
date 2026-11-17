#pragma once

#include "CoreMinimal.h"

// 효과음·배경음. 소리는 Tools/Audio/generate_audio.py가 합성해서 /Game/YJJDefense/Audio에 넣는다.
// 에셋이 없으면 조용히 넘어간다
namespace YJJAudio
{
	// 같은 소리가 한 프레임에 수십 번 겹치지 않도록 짧은 간격으로는 다시 내지 않는다
	YJJDEFENSE_API void Play(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, float Pitch = 1.f);
	YJJDEFENSE_API void StartBgm(const UObject* WorldContext, float Volume = 0.45f);
}
