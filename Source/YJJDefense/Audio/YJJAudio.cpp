#include "Audio/YJJAudio.h"
#include "Engine/World.h"
#include "Game/YJJGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

TMap<FName, double> FYJJAudio::LastPlayed;

USoundBase* FYJJAudio::LoadSound(const FString& Name)
{
	const FString Path = FString::Printf(TEXT("%s/%s.%s"), AudioFolder, *Name, *Name);
	return LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void FYJJAudio::Play(const UObject* WorldContext, FName Key)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const FYJJSoundDef* Def = YJJData::Sound(Key);
	if (!World || !Def)
	{
		return;
	}

	const FName SoundName(*Def->Sound);
	const double Now = World->GetRealTimeSeconds();
	if (const double* Last = LastPlayed.Find(SoundName); Last && Now >= *Last && Now - *Last < YJJData::Tune().SoundMinInterval)
	{
		return;
	}
	LastPlayed.Add(SoundName, Now);

	if (USoundBase* Sound = LoadSound(Def->Sound))
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Def->Volume);
	}
}

void FYJJAudio::StartMusic(const UObject* WorldContext)
{
	const FYJJSoundDef* Def = YJJData::Sound(MusicKey);
	if (USoundBase* Music = Def ? LoadSound(Def->Sound) : nullptr)
	{
		UGameplayStatics::SpawnSound2D(WorldContext, Music, Def->Volume);
	}
}
