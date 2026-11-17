#include "Meta/YJJSaveGame.h"
#include "Game/YJJGameData.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	// 테스트·스모크 실행은 -YJJSave=이름 으로 다른 슬롯을 써서 실제 진행 상황을 건드리지 않는다
	FString SaveSlotName()
	{
		FString Slot = TEXT("YJJDefense");
		FParse::Value(FCommandLine::Get(), TEXT("YJJSave="), Slot);
		return Slot;
	}

	// 판을 거듭할수록 룬과 병졸이 하나씩 풀린다 → 만들 수 있는 조합이 늘어난다
	const FYJJUnlock GUnlockTrack[] = {
		{ false, static_cast<uint8>(EYJJRune::Wind), 8 },
		{ true, static_cast<uint8>(EYJJTower::Mage), 20 },
		{ false, static_cast<uint8>(EYJJRune::Poison), 36 },
		{ false, static_cast<uint8>(EYJJRune::Chain), 56 },
		{ true, static_cast<uint8>(EYJJTower::Shrine), 80 },
		{ false, static_cast<uint8>(EYJJRune::Split), 110 },
	};

	// 아래쪽 비트부터 MaxCount개만 남긴다
	int32 KeepLowestBits(int32 Mask, int32 MaxCount)
	{
		int32 Result = 0;
		for (int32 Bit = 0; Bit < 32 && MaxCount > 0; ++Bit)
		{
			if (Mask & (1 << Bit))
			{
				Result |= 1 << Bit;
				--MaxCount;
			}
		}
		return Result;
	}
}

namespace YJJProgression
{
	TConstArrayView<FYJJUnlock> UnlockTrack()
	{
		return GUnlockTrack;
	}

	const TCHAR* UnlockName(const FYJJUnlock& Unlock)
	{
		return Unlock.bTower ? YJJData::Tower(static_cast<EYJJTower>(Unlock.Index)).Name : YJJData::Rune(static_cast<EYJJRune>(Unlock.Index)).SpellName;
	}

	UYJJSaveGame* LoadOrCreate()
	{
		UYJJSaveGame* SaveGame = Cast<UYJJSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName(), 0));
		if (!SaveGame)
		{
			SaveGame = NewObject<UYJJSaveGame>();
		}
		SanitizeSelection(*SaveGame);
		return SaveGame;
	}

	void Save(UYJJSaveGame* SaveGame)
	{
		if (SaveGame)
		{
			UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName(), 0);
		}
	}

	void SanitizeSelection(UYJJSaveGame& SaveGame)
	{
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
		SaveGame.SelectedRunes = KeepLowestBits(SaveGame.SelectedRunes, MaxBagRunes);
		SaveGame.SelectedTowers = KeepLowestBits(SaveGame.SelectedTowers, MaxBagTowers);
		SaveGame.UnlockedMaps = FMath::Clamp(SaveGame.UnlockedMaps, 1, YJJMap::NumMaps());
		SaveGame.SelectedMap = FMath::Clamp(SaveGame.SelectedMap, 0, SaveGame.UnlockedMaps - 1);
		SaveGame.MaxTier = FMath::Clamp(SaveGame.MaxTier, 1, MaxTier);
		SaveGame.SelectedTier = FMath::Clamp(SaveGame.SelectedTier, 1, SaveGame.MaxTier);
	}

	bool ToggleRune(UYJJSaveGame& SaveGame, EYJJRune Rune)
	{
		const int32 Bit = 1 << static_cast<int32>(Rune);
		if (!(SaveGame.UnlockedRunes & Bit))
		{
			return false;
		}
		if (SaveGame.SelectedRunes & Bit)
		{
			// 최소 하나는 들고 나가야 한다
			if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedRunes)) <= 1)
			{
				return false;
			}
			SaveGame.SelectedRunes &= ~Bit;
			return true;
		}
		if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedRunes)) >= MaxBagRunes)
		{
			return false;
		}
		SaveGame.SelectedRunes |= Bit;
		return true;
	}

	bool ToggleTower(UYJJSaveGame& SaveGame, EYJJTower Tower)
	{
		const int32 Bit = 1 << static_cast<int32>(Tower);
		if (!(SaveGame.UnlockedTowers & Bit))
		{
			return false;
		}
		if (SaveGame.SelectedTowers & Bit)
		{
			if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedTowers)) <= 1)
			{
				return false;
			}
			SaveGame.SelectedTowers &= ~Bit;
			return true;
		}
		if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedTowers)) >= MaxBagTowers)
		{
			return false;
		}
		SaveGame.SelectedTowers |= Bit;
		return true;
	}

	FYJJRunReward ApplyRun(UYJJSaveGame& SaveGame, const FYJJRunStats& Stats)
	{
		FYJJRunReward Reward;

		for (const int32 Combo : Stats.CombosCast)
		{
			if (!SaveGame.DiscoveredCombos.Contains(Combo))
			{
				SaveGame.DiscoveredCombos.Add(Combo);
				Reward.NewCombos++;
			}
		}

		Reward.WavePoints = Stats.WavesCleared / 2;
		Reward.VictoryPoints = Stats.bVictory ? 3 : 0;
		Reward.StarPoints = Stats.Stars;
		Reward.ComboPoints = FMath::Min(Reward.NewCombos, 6);
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
			if (Stats.Tier >= SaveGame.MaxTier && SaveGame.MaxTier < MaxTier)
			{
				SaveGame.MaxTier++;
				Reward.bNewTier = true;
			}
		}

		const TConstArrayView<FYJJUnlock> Track = UnlockTrack();
		while (Track.IsValidIndex(SaveGame.UnlockIndex) && SaveGame.Points >= Track[SaveGame.UnlockIndex].PointsNeeded)
		{
			const FYJJUnlock& Unlock = Track[SaveGame.UnlockIndex++];
			if (Unlock.bTower)
			{
				SaveGame.UnlockedTowers |= 1 << Unlock.Index;
				if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedTowers)) < MaxBagTowers)
				{
					SaveGame.SelectedTowers |= 1 << Unlock.Index;
				}
			}
			else
			{
				SaveGame.UnlockedRunes |= 1 << Unlock.Index;
				if (FMath::CountBits(static_cast<uint32>(SaveGame.SelectedRunes)) < MaxBagRunes)
				{
					SaveGame.SelectedRunes |= 1 << Unlock.Index;
				}
			}
			Reward.NewUnlocks.Add(Unlock);
		}

		SanitizeSelection(SaveGame);
		return Reward;
	}
}
