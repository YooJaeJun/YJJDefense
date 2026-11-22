#include "Game/YJJGameData.h"
#include "Algo/BinarySearch.h"
#include "Data/YJJDataRows.h"
#include "Data/YJJLoc.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/YJJUIStyle.h"
#include "UObject/Package.h"
#include "YJJDefense.h"

FAutoConsoleCommand FYJJGameDatabase::ReloadCommand(
	TEXT("yjj.ReloadData"),
	TEXT("Content/YJJDefense/Data/*.csv 를 다시 읽는다 (글자 포함)"),
	FConsoleCommandDelegate::CreateLambda([]
	{
		YJJLoc::Reload();
		FYJJGameDatabase::Get().Reload();
	}));

FYJJGameDatabase::FYJJGameDatabase()
{
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
		{
			StartDisks[Disk][Socket] = EYJJRune::None;
		}
	}
	for (int32 A = 0; A < static_cast<int32>(EYJJElement::Count); ++A)
	{
		for (int32 B = 0; B < static_cast<int32>(EYJJElement::Count); ++B)
		{
			PairReactions[A][B] = EYJJReaction::None;
		}
	}
}

FYJJGameDatabase& FYJJGameDatabase::Get()
{
	static FYJJGameDatabase Instance;
	if (!Instance.bLoaded)
	{
		Instance.Load();
	}
	return Instance;
}

void FYJJGameDatabase::Reload()
{
	const int32 KeepMap = ActiveMap;
	for (UDataTable* Table : Tables)
	{
		Table->RemoveFromRoot();
	}
	*this = FYJJGameDatabase();
	Load();
	ActiveMap = FMath::Clamp(KeepMap, 0, FMath::Max(Maps.Num() - 1, 0));
}

void FYJJGameDatabase::Problem(const FString& Message)
{
	Problems.Add(Message);
	UE_LOG(LogYJJDefense, Warning, TEXT("[Data] %s"), *Message);
}

FString FYJJGameDatabase::DataPath(const TCHAR* File)
{
	return FPaths::ProjectContentDir() / TEXT("YJJDefense/Data") / File;
}

FLinearColor FYJJGameDatabase::ParseColor(const FString& Hex)
{
	return FColor::FromHex(Hex).ReinterpretAsLinear();
}

UDataTable* FYJJGameDatabase::LoadTable(const TCHAR* File, UScriptStruct* RowStruct)
{
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	Table->RowStruct = RowStruct;
	Table->AddToRoot();
	Tables.Add(Table);

	FString Csv;
	if (!FFileHelper::LoadFileToString(Csv, *DataPath(File)))
	{
		Problem(FString::Printf(TEXT("%s 를 읽지 못했다 (%s)"), File, *DataPath(File)));
		return Table;
	}
	for (const FString& Message : Table->CreateTableFromCSVString(Csv))
	{
		Problem(FString::Printf(TEXT("%s: %s"), File, *Message));
	}
	UE_LOG(LogYJJDefense, Verbose, TEXT("[Data] %s: %d rows"), File, Table->GetRowMap().Num());
	return Table;
}

void FYJJGameDatabase::FillNameValues(const TCHAR* File, const UScriptStruct* Struct, void* Data)
{
	const UDataTable* Table = LoadTable(File, FYJJNameValueRow::StaticStruct());
	TSet<FName> Used;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FNumericProperty* Number = CastField<FNumericProperty>(*It);
		if (!Number)
		{
			continue;
		}
		const FName Name = Number->GetFName();
		const FYJJNameValueRow* Row = Table->FindRow<FYJJNameValueRow>(Name, TEXT("YJJData"), false);
		if (!Row)
		{
			Problem(FString::Printf(TEXT("%s 에 %s 가 없다"), File, *Name.ToString()));
			continue;
		}
		Used.Add(Name);
		void* Value = Number->ContainerPtrToValuePtr<void>(Data);
		if (Number->IsFloatingPoint())
		{
			Number->SetFloatingPointPropertyValue(Value, Row->Value);
		}
		else
		{
			Number->SetIntPropertyValue(Value, static_cast<int64>(FMath::RoundToInt(Row->Value)));
		}
	}
	for (const FName& RowName : Table->GetRowNames())
	{
		if (!Used.Contains(RowName))
		{
			Problem(FString::Printf(TEXT("%s 의 %s 는 쓰는 곳이 없다"), File, *RowName.ToString()));
		}
	}
}

void FYJJGameDatabase::Load()
{
	bLoaded = true;
	Tables.Reset();

	FillNameValues(TEXT("Tuning.csv"), FYJJTuning::StaticStruct(), &Tuning);
	FillNameValues(TEXT("UILayout.csv"), FYJJUILayout::StaticStruct(), &Layout);
	LoadRunes();
	LoadReactions();
	LoadTowers();
	LoadEnemies();
	LoadMaps();
	LoadWaves();
	LoadUnlocks();
	LoadStartDisks();
	LoadBot();
	LoadSounds();
	LoadAnims();
	Validate();
	UE_LOG(LogYJJDefense, Display, TEXT("[Data] loaded: %d runes, %d towers, %d enemies, %d maps, %d problems"), Runes.Num(), Towers.Num(), Enemies.Num(), Maps.Num(), Problems.Num());
}

void FYJJGameDatabase::LoadRunes()
{
	const UDataTable* Table = LoadTable(TEXT("Runes.csv"), FYJJRuneRow::StaticStruct());
	const UEnum* Enum = StaticEnum<EYJJRune>();
	Runes.SetNum(static_cast<int32>(EYJJRune::Count));
	for (int32 Index = 0; Index < Runes.Num(); ++Index)
	{
		const FString Name = Enum->GetNameStringByValue(Index);
		const FYJJRuneRow* Row = Table->FindRow<FYJJRuneRow>(FName(*Name), TEXT("YJJData"), false);
		if (!Row)
		{
			Problem(FString::Printf(TEXT("Runes.csv 에 %s 가 없다"), *Name));
			continue;
		}
		FYJJRuneDef& Def = Runes[Index];
		Def.Rune = static_cast<EYJJRune>(Index);
		Def.Element = Row->Element;
		Def.NameKey = Row->NameKey;
		Def.SpellKey = Row->SpellKey;
		Def.DescKey = Row->DescKey;
		Def.Art = Row->Art;
		Def.Color = ParseColor(Row->Color);
		Def.BurnScale = Row->BurnScale;
		Def.BurnTime = Row->BurnTime;
		Def.SlowFactor = Row->SlowFactor;
		Def.SlowTime = Row->SlowTime;
		Def.StunTime = Row->StunTime;
		Def.PoisonScale = Row->PoisonScale;
		Def.PoisonTime = Row->PoisonTime;
		Def.Knockback = Row->Knockback;
		Def.SpellDamageMult = Row->SpellDamageMult;
		Def.SpellRadiusBonus = Row->SpellRadiusBonus;
	}
}

void FYJJGameDatabase::LoadReactions()
{
	const UDataTable* Table = LoadTable(TEXT("Reactions.csv"), FYJJReactionRow::StaticStruct());
	const UEnum* Enum = StaticEnum<EYJJReaction>();
	Reactions.SetNum(static_cast<int32>(EYJJReaction::Count));
	for (int32 Index = static_cast<int32>(EYJJReaction::None) + 1; Index < Reactions.Num(); ++Index)
	{
		const FString Name = Enum->GetNameStringByValue(Index);
		const FYJJReactionRow* Row = Table->FindRow<FYJJReactionRow>(FName(*Name), TEXT("YJJData"), false);
		if (!Row)
		{
			Problem(FString::Printf(TEXT("Reactions.csv 에 %s 가 없다"), *Name));
			continue;
		}
		FYJJReactionDef& Def = Reactions[Index];
		Def.Reaction = static_cast<EYJJReaction>(Index);
		Def.ElementA = Row->ElementA;
		Def.ElementB = Row->ElementB;
		Def.NameKey = Row->NameKey;
		Def.SpellKey = Row->SpellKey;
		Def.DescKey = Row->DescKey;
		Def.Color = ParseColor(Row->Color);
		Def.Radius = Row->Radius;
		Def.FxRadius = Row->FxRadius;
		Def.Damage = Row->Damage;
		Def.Knockback = Row->Knockback;
		Def.BurnScale = Row->BurnScale;
		Def.BurnTime = Row->BurnTime;
		Def.PoisonScale = Row->PoisonScale;
		Def.PoisonTime = Row->PoisonTime;
		Def.SlowFactor = Row->SlowFactor;
		Def.SlowTime = Row->SlowTime;
		Def.StunTime = Row->StunTime;
		Def.FreezeTime = Row->FreezeTime;
		Def.VulnMult = Row->VulnMult;
		Def.VulnTime = Row->VulnTime;
		Def.Targets = Row->Targets;
		Def.ZoneTime = Row->ZoneTime;
		Def.ZoneDps = Row->ZoneDps;
		Def.ZoneSlow = Row->ZoneSlow;
		Def.ZoneBurn = Row->ZoneBurn;
		Def.bZoneGroundOnly = Row->ZoneGroundOnly;

		const int32 A = static_cast<int32>(Row->ElementA);
		const int32 B = static_cast<int32>(Row->ElementB);
		if (Row->ElementA == EYJJElement::None || Row->ElementB == EYJJElement::None || A == B)
		{
			Problem(FString::Printf(TEXT("Reactions.csv %s: 서로 다른 원소 둘이 필요하다"), *Name));
			continue;
		}
		if (PairReactions[A][B] != EYJJReaction::None)
		{
			Problem(FString::Printf(TEXT("Reactions.csv %s: 원소 짝이 겹친다"), *Name));
		}
		PairReactions[A][B] = Def.Reaction;
		PairReactions[B][A] = Def.Reaction;
	}
}

void FYJJGameDatabase::LoadTowers()
{
	const UDataTable* Table = LoadTable(TEXT("Towers.csv"), FYJJTowerRow::StaticStruct());
	const UEnum* Enum = StaticEnum<EYJJTower>();
	Towers.SetNum(static_cast<int32>(EYJJTower::Count));
	for (int32 Index = 0; Index < Towers.Num(); ++Index)
	{
		const FString Name = Enum->GetNameStringByValue(Index);
		const FYJJTowerRow* Row = Table->FindRow<FYJJTowerRow>(FName(*Name), TEXT("YJJData"), false);
		if (!Row)
		{
			Problem(FString::Printf(TEXT("Towers.csv 에 %s 가 없다"), *Name));
			continue;
		}
		FYJJTowerDef& Def = Towers[Index];
		Def.Type = static_cast<EYJJTower>(Index);
		Def.NameKey = Row->NameKey;
		Def.DescKey = Row->DescKey;
		Def.Art = Row->Art;
		Def.bMagic = Row->Magic;
		Def.bHitsAir = Row->HitsAir;
		Def.Projectile = Row->Projectile;
		Def.ShotSpeed = Row->ShotSpeed;
		Def.ShotTime = Row->ShotTime;
		Def.ShotArcRatio = Row->ShotArcRatio;
		Def.ShotArcHeight = Row->ShotArcHeight;
		Def.bHoming = Row->Homing;
		Def.bLeadTarget = Row->LeadTarget;

		const int32 Costs[YJJ::NumTowerLevels] = { Row->Cost1, Row->Cost2, Row->Cost3 };
		const float Damages[YJJ::NumTowerLevels] = { Row->Damage1, Row->Damage2, Row->Damage3 };
		const float Intervals[YJJ::NumTowerLevels] = { Row->Interval1, Row->Interval2, Row->Interval3 };
		const float Ranges[YJJ::NumTowerLevels] = { Row->Range1, Row->Range2, Row->Range3 };
		const float Splashes[YJJ::NumTowerLevels] = { Row->Splash1, Row->Splash2, Row->Splash3 };
		const float SoldierHPs[YJJ::NumTowerLevels] = { Row->SoldierHP1, Row->SoldierHP2, Row->SoldierHP3 };
		const float SoldierArmors[YJJ::NumTowerLevels] = { Row->SoldierArmor1, Row->SoldierArmor2, Row->SoldierArmor3 };
		for (int32 Level = 0; Level < YJJ::NumTowerLevels; ++Level)
		{
			Def.Levels[Level] = { Costs[Level], Damages[Level], Intervals[Level], Ranges[Level], Splashes[Level], SoldierHPs[Level], SoldierArmors[Level] };
		}
	}
}

void FYJJGameDatabase::LoadEnemies()
{
	const UDataTable* Table = LoadTable(TEXT("Enemies.csv"), FYJJEnemyRow::StaticStruct());
	const UEnum* Enum = StaticEnum<EYJJEnemy>();
	Enemies.SetNum(static_cast<int32>(EYJJEnemy::Count));
	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		const FString Name = Enum->GetNameStringByValue(Index);
		const FYJJEnemyRow* Row = Table->FindRow<FYJJEnemyRow>(FName(*Name), TEXT("YJJData"), false);
		if (!Row)
		{
			Problem(FString::Printf(TEXT("Enemies.csv 에 %s 가 없다"), *Name));
			continue;
		}
		FYJJEnemyDef& Def = Enemies[Index];
		Def.Type = static_cast<EYJJEnemy>(Index);
		Def.NameKey = Row->NameKey;
		Def.Art = Row->Art;
		Def.HP = Row->HP;
		Def.Speed = Row->Speed;
		Def.Armor = Row->Armor;
		Def.MagicResist = Row->MagicResist;
		Def.Gold = Row->Gold;
		Def.LifeCost = Row->LifeCost;
		Def.bFlying = Row->Flying;
		Def.bBoss = Row->Boss;
		Def.MeleeDamage = Row->MeleeDamage;
		Def.MeleeInterval = Row->MeleeInterval;
		Def.Radius = Row->Radius;
		Def.HealPerSecond = Row->HealPerSecond;
		Def.SplitInto = Row->SplitInto;
		Def.SplitCount = Row->SplitCount;
		Def.Immune = Row->Immune;
	}
}

void FYJJGameDatabase::LoadMaps()
{
	const UDataTable* Table = LoadTable(TEXT("Maps.csv"), FYJJMapRow::StaticStruct());
	const UDataTable* Geometry = LoadTable(TEXT("MapGeometry.csv"), FYJJMapGeometryRow::StaticStruct());

	TArray<TPair<int32, FName>> Order;
	Table->ForeachRow<FYJJMapRow>(TEXT("YJJData"), [&Order](const FName& Key, const FYJJMapRow& Row)
	{
		Order.Add({ Row.Order, Key });
	});
	Order.StableSort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B) { return A.Key < B.Key; });

	for (const TPair<int32, FName>& Entry : Order)
	{
		const FYJJMapRow* Row = Table->FindRow<FYJJMapRow>(Entry.Value, TEXT("YJJData"));
		const FYJJMapGeometryRow* Shape = Geometry->FindRow<FYJJMapGeometryRow>(Entry.Value, TEXT("YJJData"), false);
		if (!Shape)
		{
			Problem(FString::Printf(TEXT("MapGeometry.csv 에 %s 가 없다 (Tools/Art/BuildArt.ps1 -SkipImport)"), *Entry.Value.ToString()));
			continue;
		}

		FYJJMapDef& Def = Maps.AddDefaulted_GetRef();
		Def.Key = Entry.Value;
		Def.NameKey = Row->NameKey;
		Def.DescKey = Row->DescKey;
		Def.Texture = Row->Texture;
		Def.Showcase = Row->Showcase;
		Def.Wizard = FVector2f(Shape->Wizard);
		Def.Gate = FVector2f(Shape->Gate);
		for (const FVector2D& Point : Shape->Path)
		{
			Def.Path.Add(FVector2f(Point));
		}
		for (const FVector2D& Point : Shape->Spots)
		{
			Def.Spots.Add(FVector2f(Point));
		}

		float Total = 0.f;
		Def.PathCumulative.Add(Total);
		for (int32 Index = 1; Index < Def.Path.Num(); ++Index)
		{
			Total += FVector2f::Distance(Def.Path[Index - 1], Def.Path[Index]);
			Def.PathCumulative.Add(Total);
		}
	}
}

void FYJJGameDatabase::LoadWaves()
{
	const UDataTable* Table = LoadTable(TEXT("Waves.csv"), FYJJWaveRow::StaticStruct());
	Table->ForeachRow<FYJJWaveRow>(TEXT("YJJData"), [this](const FName& Key, const FYJJWaveRow& Row)
	{
		FYJJMapDef* Map = Maps.FindByPredicate([&Row](const FYJJMapDef& Def) { return Def.Key == Row.Map; });
		if (!Map || Row.Wave < 1 || Row.Enemy >= EYJJEnemy::Count)
		{
			Problem(FString::Printf(TEXT("Waves.csv %s: 지도·웨이브·몬스터가 맞지 않다"), *Key.ToString()));
			return;
		}
		if (Map->Waves.Num() < Row.Wave)
		{
			Map->Waves.SetNum(Row.Wave);
		}
		Map->Waves[Row.Wave - 1].Add({ Row.Enemy, Row.Count, Row.Interval, Row.Delay });
	});
}

void FYJJGameDatabase::LoadUnlocks()
{
	const UDataTable* Table = LoadTable(TEXT("Unlocks.csv"), FYJJUnlockRow::StaticStruct());
	TArray<TPair<int32, FYJJUnlock>> Track;
	Table->ForeachRow<FYJJUnlockRow>(TEXT("YJJData"), [this, &Track](const FName& Key, const FYJJUnlockRow& Row)
	{
		const bool bRune = Row.Rune < EYJJRune::Count;
		const bool bTower = Row.Tower < EYJJTower::Count;
		if (bRune == bTower)
		{
			Problem(FString::Printf(TEXT("Unlocks.csv %s: Rune이나 Tower 중 하나만 적는다"), *Key.ToString()));
			return;
		}
		const uint32 Bit = 1u << (bRune ? static_cast<uint32>(Row.Rune) : static_cast<uint32>(Row.Tower));
		if (Row.Points <= 0)
		{
			(bRune ? StartRuneMask : StartTowerMask) |= Bit;
			return;
		}
		FYJJUnlock Unlock;
		Unlock.bTower = bTower;
		Unlock.Index = bRune ? static_cast<uint8>(Row.Rune) : static_cast<uint8>(Row.Tower);
		Unlock.PointsNeeded = Row.Points;
		Track.Add({ Row.Order, Unlock });
	});
	Track.StableSort([](const TPair<int32, FYJJUnlock>& A, const TPair<int32, FYJJUnlock>& B) { return A.Key < B.Key; });
	for (const TPair<int32, FYJJUnlock>& Entry : Track)
	{
		UnlockTrack.Add(Entry.Value);
	}
}

void FYJJGameDatabase::LoadStartDisks()
{
	const UDataTable* Table = LoadTable(TEXT("StartDisks.csv"), FYJJStartDiskRow::StaticStruct());
	int32 Disk = 0;
	Table->ForeachRow<FYJJStartDiskRow>(TEXT("YJJData"), [this, &Disk](const FName& Key, const FYJJStartDiskRow& Row)
	{
		if (Disk < YJJ::NumDisks)
		{
			StartDisks[Disk][0] = Row.Rune1;
			StartDisks[Disk][1] = Row.Rune2;
		}
		Disk++;
	});
	if (Disk != YJJ::NumDisks)
	{
		Problem(FString::Printf(TEXT("StartDisks.csv: 디스크는 %d개여야 한다 (%d개)"), YJJ::NumDisks, Disk));
	}
}

void FYJJGameDatabase::LoadBot()
{
	const UDataTable* Table = LoadTable(TEXT("BotBuildOrder.csv"), FYJJBotBuildRow::StaticStruct());
	Table->ForeachRow<FYJJBotBuildRow>(TEXT("YJJData"), [this](const FName& Key, const FYJJBotBuildRow& Row)
	{
		BotBuildPattern.Add(Row.Tower);
	});
}

void FYJJGameDatabase::LoadSounds()
{
	const UDataTable* Table = LoadTable(TEXT("Sounds.csv"), FYJJSoundRow::StaticStruct());
	Table->ForeachRow<FYJJSoundRow>(TEXT("YJJData"), [this](const FName& Key, const FYJJSoundRow& Row)
	{
		Sounds.Add(Key, { Row.Sound, Row.Volume });
	});
}

void FYJJGameDatabase::LoadAnims()
{
	const UDataTable* Table = LoadTable(TEXT("Anims.csv"), FYJJAnimRow::StaticStruct());
	Table->ForeachRow<FYJJAnimRow>(TEXT("YJJData"), [this](const FName& Key, const FYJJAnimRow& Row)
	{
		if (Row.Frames <= 0 || Row.HitFrame < 0 || Row.HitFrame >= Row.Frames || Row.Fps < 0.f)
		{
			Problem(FString::Printf(TEXT("Anims.csv %s: 프레임 %d, 속도 %.1f, 타격 프레임 %d가 맞지 않는다"), *Key.ToString(), Row.Frames, Row.Fps, Row.HitFrame));
			return;
		}
		Anims.Add(Key, { Row.Frames, Row.Fps, Row.HitFrame });
	});
}

void FYJJGameDatabase::Validate()
{
	if (Maps.IsEmpty())
	{
		Problem(TEXT("지도가 하나도 없다"));
		Maps.AddDefaulted();
	}
	for (FYJJMapDef& Map : Maps)
	{
		if (Map.Path.Num() < 2)
		{
			Problem(FString::Printf(TEXT("%s: 길이 너무 짧다"), *Map.Key.ToString()));
			Map.Path = { FVector2f::ZeroVector, FVector2f(YJJ::ViewW, 0.f) };
			Map.PathCumulative = { 0.f, YJJ::ViewW };
		}
		if (Map.Waves.IsEmpty())
		{
			Problem(FString::Printf(TEXT("%s: 웨이브가 없다"), *Map.Key.ToString()));
			Map.Waves.AddDefaulted();
		}
		for (int32 Wave = 0; Wave < Map.Waves.Num(); ++Wave)
		{
			if (Map.Waves[Wave].IsEmpty())
			{
				Problem(FString::Printf(TEXT("%s: 웨이브 %d 가 비었다"), *Map.Key.ToString(), Wave + 1));
			}
		}
	}
	if (BotBuildPattern.IsEmpty())
	{
		BotBuildPattern.Add(EYJJTower::Archer);
	}
}

// ---------------------------------------------------------------------------------------------
// YJJData

const FYJJTuning& YJJData::Tune()
{
	return FYJJGameDatabase::Get().Tuning;
}

const FYJJUILayout& YJJData::Layout()
{
	return FYJJGameDatabase::Get().Layout;
}

const FYJJRuneDef& YJJData::Rune(EYJJRune Rune)
{
	const TArray<FYJJRuneDef>& Runes = FYJJGameDatabase::Get().Runes;
	return Runes[FMath::Clamp<int32>(static_cast<int32>(Rune), 0, Runes.Num() - 1)];
}

const FYJJTowerDef& YJJData::Tower(EYJJTower Tower)
{
	const TArray<FYJJTowerDef>& Towers = FYJJGameDatabase::Get().Towers;
	return Towers[FMath::Clamp<int32>(static_cast<int32>(Tower), 0, Towers.Num() - 1)];
}

const FYJJEnemyDef& YJJData::Enemy(EYJJEnemy Enemy)
{
	const TArray<FYJJEnemyDef>& Enemies = FYJJGameDatabase::Get().Enemies;
	return Enemies[FMath::Clamp<int32>(static_cast<int32>(Enemy), 0, Enemies.Num() - 1)];
}

const FYJJReactionDef& YJJData::Reaction(EYJJReaction Reaction)
{
	const TArray<FYJJReactionDef>& Reactions = FYJJGameDatabase::Get().Reactions;
	return Reactions[FMath::Clamp<int32>(static_cast<int32>(Reaction), 0, Reactions.Num() - 1)];
}

EYJJReaction YJJData::ReactionOf(EYJJElement A, EYJJElement B)
{
	if (A >= EYJJElement::Count || B >= EYJJElement::Count)
	{
		return EYJJReaction::None;
	}
	return FYJJGameDatabase::Get().PairReactions[static_cast<int32>(A)][static_cast<int32>(B)];
}

int32 YJJData::NumWaves()
{
	return YJJMap::Def(YJJMap::GetActive()).Waves.Num();
}

TConstArrayView<FYJJWaveGroup> YJJData::Wave(int32 Index)
{
	const TArray<TArray<FYJJWaveGroup>>& Waves = YJJMap::Def(YJJMap::GetActive()).Waves;
	return Waves[FMath::Clamp(Index, 0, Waves.Num() - 1)];
}

TConstArrayView<FYJJUnlock> YJJData::UnlockTrack()
{
	return FYJJGameDatabase::Get().UnlockTrack;
}

uint32 YJJData::StartRuneMask()
{
	return FYJJGameDatabase::Get().StartRuneMask;
}

uint32 YJJData::StartTowerMask()
{
	return FYJJGameDatabase::Get().StartTowerMask;
}

EYJJRune YJJData::StartDisk(int32 Disk, int32 Socket)
{
	return FYJJGameDatabase::Get().StartDisks[FMath::Clamp(Disk, 0, YJJ::NumDisks - 1)][FMath::Clamp(Socket, 0, YJJ::NumSockets - 1)];
}

TConstArrayView<EYJJTower> YJJData::BotBuildPattern()
{
	return FYJJGameDatabase::Get().BotBuildPattern;
}

const FYJJSoundDef* YJJData::Sound(FName Key)
{
	return FYJJGameDatabase::Get().Sounds.Find(Key);
}

const FYJJAnimDef* YJJData::Anim(FName Key)
{
	return FYJJGameDatabase::Get().Anims.Find(Key);
}

FString YJJData::EnemyTraits(EYJJEnemy Type)
{
	const FYJJEnemyDef& Def = Enemy(Type);
	const FYJJTuning& Tuning = Tune();
	TArray<FString> Traits;
	if (Def.bBoss) { Traits.Add(YJJLoc::Get(TEXT("Trait.Boss"))); }
	if (Def.bFlying) { Traits.Add(YJJLoc::Get(TEXT("Trait.Flying"))); }
	if (Def.Speed >= Tuning.TraitFastSpeed) { Traits.Add(YJJLoc::Get(TEXT("Trait.Fast"))); }
	if (Def.Armor >= Tuning.TraitArmor) { Traits.Add(YJJLoc::Get(TEXT("Trait.Armor"))); }
	if (Def.MagicResist >= Tuning.TraitMagicResist) { Traits.Add(YJJLoc::Get(TEXT("Trait.MagicResist"))); }
	if (Def.HealPerSecond > 0.f) { Traits.Add(YJJLoc::Get(TEXT("Trait.Heal"))); }
	if (Def.HasSplit()) { Traits.Add(YJJLoc::Get(TEXT("Trait.Split"))); }
	if (Def.Immune == EYJJElement::Ice) { Traits.Add(YJJLoc::Get(TEXT("Trait.ChillImmune"))); }
	return FString::Join(Traits, *YJJLoc::Get(TEXT("Common.ListSeparator")));
}

const FString& YJJData::ElementName(EYJJElement Element)
{
	if (Element == EYJJElement::None || Element >= EYJJElement::Count)
	{
		return YJJLoc::Get(TEXT("Element.Arcane"));
	}
	return YJJLoc::Get(Rune(ElementRune(Element)).NameKey);
}

FLinearColor YJJData::ElementColor(EYJJElement Element)
{
	if (Element == EYJJElement::None || Element >= EYJJElement::Count)
	{
		return YJJUI::Arcane;
	}
	return Rune(ElementRune(Element)).Color;
}

// ---------------------------------------------------------------------------------------------
// YJJMap

int32 YJJMap::NumMaps()
{
	return FYJJGameDatabase::Get().Maps.Num();
}

void YJJMap::SetActive(int32 Map)
{
	FYJJGameDatabase& Database = FYJJGameDatabase::Get();
	Database.ActiveMap = FMath::Clamp(Map, 0, Database.Maps.Num() - 1);
}

int32 YJJMap::GetActive()
{
	return FYJJGameDatabase::Get().ActiveMap;
}

const FYJJMapDef& YJJMap::Def(int32 Map)
{
	const TArray<FYJJMapDef>& Maps = FYJJGameDatabase::Get().Maps;
	return Maps[FMath::Clamp(Map, 0, Maps.Num() - 1)];
}

const FString& YJJMap::Name(int32 Map)
{
	return YJJLoc::Get(Def(Map).NameKey);
}

const FString& YJJMap::Desc(int32 Map)
{
	return YJJLoc::Get(Def(Map).DescKey);
}

TConstArrayView<FVector2f> YJJMap::Path()
{
	return Def(GetActive()).Path;
}

float YJJMap::PathLength()
{
	return Def(GetActive()).PathCumulative.Last();
}

FVector2f YJJMap::PointAt(float Dist, float Lateral, FVector2f* OutDirection)
{
	const FYJJMapDef& Map = Def(GetActive());
	const TArray<float>& Cumulative = Map.PathCumulative;
	const int32 Num = Cumulative.Num();
	Dist = FMath::Clamp(Dist, 0.f, Cumulative.Last());

	// Cumulative[Index] <= Dist 인 마지막 구간
	const int32 Index = FMath::Clamp(static_cast<int32>(Algo::UpperBound(Cumulative, Dist)) - 1, 0, Num - 2);
	const FVector2f A = Map.Path[Index];
	const FVector2f B = Map.Path[Index + 1];
	const float SegmentLength = FMath::Max(Cumulative[Index + 1] - Cumulative[Index], KINDA_SMALL_NUMBER);
	const FVector2f Direction = (B - A) / SegmentLength;
	if (OutDirection)
	{
		*OutDirection = Direction;
	}
	const FVector2f Right(-Direction.Y, Direction.X);
	return A + Direction * (Dist - Cumulative[Index]) + Right * Lateral;
}

float YJJMap::NearestDist(const FVector2f& Pos)
{
	const FYJJMapDef& Map = Def(GetActive());
	float BestDist = 0.f;
	float BestSq = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index + 1 < Map.Path.Num(); ++Index)
	{
		const FVector2f A = Map.Path[Index];
		const FVector2f AB = Map.Path[Index + 1] - A;
		const float LengthSq = FMath::Max(AB.SizeSquared(), KINDA_SMALL_NUMBER);
		const float T = FMath::Clamp(FVector2f::DotProduct(Pos - A, AB) / LengthSq, 0.f, 1.f);
		const float Sq = FVector2f::DistSquared(Pos, A + AB * T);
		if (Sq < BestSq)
		{
			BestSq = Sq;
			BestDist = Map.PathCumulative[Index] + FMath::Sqrt(LengthSq) * T;
		}
	}
	return BestDist;
}

TConstArrayView<FVector2f> YJJMap::Spots()
{
	return Def(GetActive()).Spots;
}

FVector2f YJJMap::WizardPos()
{
	return Def(GetActive()).Wizard;
}

FVector2f YJJMap::GatePos()
{
	return Def(GetActive()).Gate;
}
