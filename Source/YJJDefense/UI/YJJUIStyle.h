#pragma once

#include "CoreMinimal.h"

// UI 색. 아트(Tools/Art/generate_art.py)와 같은 팔레트. 배치·크기는 Content/YJJDefense/Data/UILayout.csv
namespace YJJUI
{
	inline FLinearColor Hex(uint32 RGB, float Alpha = 1.f)
	{
		constexpr uint32 ByteMask = 0xFF;
		constexpr uint32 RedShift = 16;
		constexpr uint32 GreenShift = 8;
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor((RGB >> RedShift) & ByteMask, (RGB >> GreenShift) & ByteMask, RGB & ByteMask));
		Color.A = Alpha;
		return Color;
	}

	inline FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}

	// 기본 팔레트
	inline const FLinearColor Ink = Hex(0x241E28);
	inline const FLinearColor White = FLinearColor::White;
	inline const FLinearColor Cream = Hex(0xFFF6DD);
	inline const FLinearColor Red = Hex(0xFF5A4F);
	inline const FLinearColor Orange = Hex(0xFF8A3C);
	inline const FLinearColor Yellow = Hex(0xFFD84A);
	inline const FLinearColor Green = Hex(0x7FD35A);
	inline const FLinearColor Sky = Hex(0x7FD6FF);
	inline const FLinearColor Blue = Hex(0x4F8DFF);
	inline const FLinearColor Purple = Hex(0x9A78FF);
	inline const FLinearColor Pink = Hex(0xFF9EC4);
	inline const FLinearColor Gray = Hex(0xB8BFCA);
	inline const FLinearColor DarkGray = Hex(0x6E6A7C);
	/** 원소 없는 스펠 (선형 값) */
	inline const FLinearColor Arcane(0.85f, 0.6f, 1.f);

	// 쓰임새별
	inline const FLinearColor ButtonDisabled = Hex(0xB9B5C4);
	inline const FLinearColor BarBack = Hex(0x4A4456);
	inline const FLinearColor Divider = Hex(0xE3DCC4);
	inline const FLinearColor Gain = Hex(0x3E9B2B);
	inline const FLinearColor CantAfford = Hex(0xFFB3AB);
	inline const FLinearColor IconDisabled = Hex(0x9A96A8);
	inline const FLinearColor CardPicked = Hex(0xFFE98A);
	inline const FLinearColor CardLocked = Hex(0xCFCBDA);
	inline const FLinearColor IconLocked = Hex(0x7C7888);
	inline const FLinearColor DiskCooling = Hex(0xC9C6D4);
	inline const FLinearColor SocketCooling = Hex(0xD8D5E0);
	inline const FLinearColor SpotSelected = Hex(0xFFF3B0);
	inline const FLinearColor SpotIdleGlow = Hex(0xE8F6D8);
	inline const FLinearColor TowerHover = Hex(0xFFF7D0);
	inline const FLinearColor FrozenTint = Hex(0xA8E4FF);
	inline const FLinearColor PoisonTint = Hex(0xC9F29A);
	inline const FLinearColor SlowTint = Hex(0xC4E6FF);
	inline const FLinearColor HitTint = Hex(0xFF8A7A);
	inline const FLinearColor BombPuff = Hex(0xFFE9C4);
}
