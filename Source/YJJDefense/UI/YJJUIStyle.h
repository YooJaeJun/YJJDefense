#pragma once

#include "CoreMinimal.h"

// UI 색·치수. 아트(Tools/Art/generate_art.py)와 같은 팔레트
namespace YJJUI
{
	inline FLinearColor Hex(uint32 RGB, float Alpha = 1.f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF));
		Color.A = Alpha;
		return Color;
	}

	inline FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}

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

	// 스펠 바 배치
	inline constexpr float BarX = 430.f;
	inline constexpr float BarY = 912.f;
	inline constexpr float BarW = 1060.f;
	inline constexpr float BarH = 162.f;
	inline constexpr float DiskY = 1006.f;
	inline constexpr float DiskSize = 122.f;
	inline constexpr float DiskFirstX = 900.f;
	inline constexpr float DiskSpacing = 160.f;
	inline constexpr float SocketSize = 46.f;
	inline constexpr float BagX = 452.f;
	inline constexpr float BagY = 972.f;
	inline constexpr float BagIcon = 60.f;
	inline constexpr float BagSpacing = 66.f;
}
