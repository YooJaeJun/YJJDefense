#include "Meta/YJJSaveGame.h"
#include "Data/YJJLoc.h"
#include "Game/YJJGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

FString FYJJProgression::SaveSlotName()
{
	FString Slot = DefaultSlot;
	FParse::Value(FCommandLine::Get(), SlotCommandLine, Slot);
	return Slot;
}

int32 FYJJProgression::KeepLowestBits(int32 Mask, int32 MaxCount)
{
	int32 Result = 0;
	for (int32 Bit = 0; Bit < MaskBits && MaxCount > 0; ++Bit)
	{
		if (Mask & (1 << Bit))
		{
			Result |= 1 << Bit;
			--MaxCount;
		}
	}
	return Result;
}

const FString& FYJJProgression::UnlockName(const FYJJUnlock& Unlock)
{
	return Unlock.bTower
		? YJJLoc::Get(YJJData::Tower(static_cast<EYJJTower>(Unlock.Index)).NameKey)
		: YJJLoc::Get(YJJData::Rune(static_cast<EYJJRune>(Unlock.Index)).SpellKey);
}

UYJJSaveGame* FYJJProgression::LoadOrCreate()
{
	UYJJSaveGame* SaveGame = Cast<UYJJSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName(), 0));
	if (!SaveGame)
	{
		SaveGame = NewObject<UYJJSaveGame>();
	}
	SanitizeSelection(*SaveGame);
	return SaveGame;
}

void FYJJProgression::Save(UYJJSaveGame* SaveGame)
{
	if (SaveGame)
	{
		UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName(), 0);
	}
}

void FYJJProgression::SanitizeSelection(UYJJSaveGame& SaveGame)
{
	const FYJJTuning& Tune = YJJData::Tune();

	// 처음부터 가진 것은 언제나 가지고 있다 (Unlocks.csv 를 바꿔도 옛 저장이 깨지지 않게)
	SaveGame.UnlockedRunes |= static_cast<int32>(YJJData::StartRuneMask());
	SaveGame.UnlockedTowers |= static_cast<int32>(YJJData::StartTowerMask());

	SaveGame.SelectedRunes &= SaveGame.UnlockedRunes;
	SaveGame.SelectedTowers &= SaveGame.UnlockedTowers;
	if (SaveGame.SelectedRunes == 0)
	{
		SaveGame.SelectedRunes = SaveGame.UnlockedRunes;
	}
	if (SaveGame.SelectedTowers == 0)
	{
		SaveGame.SelectedTowers = SaveGame.UnlockedTowers;
	}
	SaveGame.SelectedRunes = KeepLowestBits(SaveGame.SelectedRunes, Tune.MaxBagRunes);
	SaveGame.SelectedTowers = KeepLowestBits(SaveGame.SelectedTowers, Tune.MaxBagTowers);
	SaveGame.UnlockedMaps = FMath::Clamp(SaveGame.UnlockedMaps, 1, YJJMap::NumMaps());
	SaveGame.SelectedMap = FMath::Clamp(SaveGame.SelectedMap, 0, SaveGame.UnlockedMaps - 1);
	SaveGame.MaxTier = FMath::Clamp(SaveGame.MaxTier, 1, Tune.MaxTier);
	SaveGame.SelectedTier = FMath::Clamp(SaveGame.SelectedTier, 1, SaveGame.MaxTier);
}

bool FYJJProgression::ToggleBit(int32& Selected, int32 Unlocked, int32 Bit, int32 MaxCount)
{
	if (!(Unlocked & Bit))
	{
		return false;
	}
	const int32 Count = FMath::CountBits(static_cast<uint32>(Selected));
	if (Selected & Bit)
	{
		// 최소 하나는 들고 나가야 한다
		if (Count <= 1)
		{
			return false;
		}
		Selected &= ~Bit;
		return true;
	}
	if (Count >= MaxCount)
	{
		return false;
	}
	Selected |= Bit;
	return true;
}

bool FYJJProgression::ToggleRune(UYJJSaveGame& SaveGame, EYJJRune Rune)
{
	return ToggleBit(SaveGame.SelectedRunes, SaveGame.UnlockedRunes, 1 << static_cast<int32>(Rune), YJJData::Tune().MaxBagRunes);
}

bool FYJJProgression::ToggleTower(UYJJSaveGame& SaveGame, EYJJTower Tower)
{
	return ToggleBit(SaveGame.SelectedTowers, SaveGame.UnlockedTowers, 1 << static_cast<int32>(Tower), YJJData::Tune().MaxBagTowers);
}

FYJJRunReward FYJJProgression::ApplyRun(UYJJSaveGame& SaveGame, const FYJJRunStats& Stats)
{
	const FYJJTuning& Tune = YJJData::Tune();
	FYJJRunReward Reward;

	for (const int32 Combo : Stats.CombosCast)
	{
		if (!SaveGame.DiscoveredCombos.Contains(Combo))
		{
			SaveGame.DiscoveredCombos.Add(Combo);
			Reward.NewCombos++;
		}
	}

	Reward.WavePoints = Stats.WavesCleared / FMath::Max(Tune.WavesPerPoint, 1);
	Reward.VictoryPoints = Stats.bVictory ? Tune.VictoryPoints : 0;
	Reward.StarPoints = Stats.Stars;
	Reward.ComboPoints = FMath::Min(Reward.NewCombos, Tune.MaxComboPoints);
	Reward.TotalPoints = Reward.WavePoints + Reward.VictoryPoints + Reward.StarPoints + Reward.ComboPoints;

	SaveGame.Points += Reward.TotalPoints;
	SaveGame.RunsPlayed++;
	if (Stats.bVictory)
	{
		SaveGame.Wins++;
		if (Stats.Map + 1 == SaveGame.UnlockedMaps && SaveGame.UnlockedMaps < YJJMap::NumMaps())
		{
			Reward.NewMap = SaveGame.UnlockedMaps++;
		}
		if (Stats.Tier >= SaveGame.MaxTier && SaveGame.MaxTier < Tune.MaxTier)
		{
			SaveGame.MaxTier++;
			Reward.bNewTier = true;
		}
	}

	const TConstArrayView<FYJJUnlock> Track = YJJData::UnlockTrack();
	while (Track.IsValidIndex(SaveGame.UnlockIndex) && SaveGame.Points >= Track[SaveGame.UnlockIndex].PointsNeeded)
	{
		const FYJJUnlock& Unlock = Track[SaveGame.UnlockIndex++];
		const int32 Bit = 1 << Unlock.Index;
		int32& Unlocked = Unlock.bTower ? SaveGame.UnlockedTowers : SaveGame.UnlockedRunes;
		int32& Selected = Unlock.bTower ? SaveGame.SelectedTowers : SaveGame.SelectedRunes;
		const int32 MaxCount = Unlock.bTower ? Tune.MaxBagTowers : Tune.MaxBagRunes;
		Unlocked |= Bit;
		if (FMath::CountBits(static_cast<uint32>(Selected)) < MaxCount)
		{
			Selected |= Bit;
		}
		Reward.NewUnlocks.Add(Unlock);
	}

	SanitizeSelection(SaveGame);
	return Reward;
}
