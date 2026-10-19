// Arena photo and poster type for the front-end menu and the sparring mode menu.
#pragma once

#include "CoreMinimal.h"

class UTexture2D;
struct FSlateBrush;
struct FSlateFontInfo;

namespace FBMenuArt
{
	/** Content/UI/FB_MenuArena.jpg. The left side is flat black so the source picture's words are not in the texture. */
	UTexture2D* GetArenaTexture();
	const FSlateBrush* GetArenaBrush();

	FLinearColor Paper();
	FLinearColor Accent();
	FLinearColor Quiet();
	FSlateFontInfo PosterFont(int32 Size, int32 Tracking);
}
