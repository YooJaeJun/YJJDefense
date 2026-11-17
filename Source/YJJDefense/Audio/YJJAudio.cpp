#include "Audio/YJJAudio.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace YJJAudio
{
	namespace
	{
		constexpr double MinInterval = 0.06;

		USoundBase* LoadSound(const TCHAR* Name)
		{
			const FString Path = FString::Printf(TEXT("/Game/YJJDefense/Audio/%s.%s"), Name, Name);
			return LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}

	void Play(const UObject* WorldContext, const TCHAR* Name, float Volume, float Pitch)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}

		static TMap<FName, double> LastPlayed;
		const double Now = World->GetRealTimeSeconds();
		double& Last = LastPlayed.FindOrAdd(FName(Name), -1.0);
		if (Now >= Last && Now - Last < MinInterval)
		{
			return;
		}
		Last = Now;

		if (USoundBase* Sound = LoadSound(Name))
		{
			UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
		}
	}

	void StartBgm(const UObject* WorldContext, float Volume)
	{
		if (USoundBase* Bgm = LoadSound(TEXT("BGM_Meadow")))
		{
			UGameplayStatics::SpawnSound2D(WorldContext, Bgm, Volume);
		}
	}
}
