#include "UI/FBMenuArt.h"
#include "UI/FBMenuDeveloperSettings.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

namespace FBMenuArt
{
	static FString ArenaFile()
	{
		const FString Relative = TEXT("UI/FB_MenuArena.jpg");
		const FString Candidates[] = {
			FPaths::ProjectContentDir() / Relative,
			FPaths::ProjectDir() / TEXT("Content") / Relative,
			FPaths::LaunchDir() / TEXT("Content") / Relative
		};
		for (const FString& Path : Candidates)
		{
			if (FPaths::FileExists(Path))
			{
				return Path;
			}
		}
		return Candidates[0];
	}

	UTexture2D* GetArenaTexture()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (Cached.IsValid())
		{
			return Cached.Get();
		}
		UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(ArenaFile());
		if (!Texture)
		{
			UE_LOG(LogTemp, Warning, TEXT("Final Bell menu art missing: %s"), *ArenaFile());
			return nullptr;
		}
		Texture->AddToRoot();
		Cached = Texture;
		return Texture;
	}

	const FSlateBrush* GetArenaBrush()
	{
		static FSlateBrush Brush;
		if (UTexture2D* Texture = GetArenaTexture())
		{
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Image;
		}
		return &Brush;
	}

	static const UFBMenuDeveloperSettings* Style()
	{
		return UFBMenuDeveloperSettings::Get();
	}

	FLinearColor Paper()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->WarmWhite : FLinearColor(0.9569f, 0.9451f, 0.9176f, 1.f);
	}

	FLinearColor Accent()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->BoxingRed : FLinearColor(0.7765f, 0.1765f, 0.1765f, 1.f);
	}

	FLinearColor Quiet()
	{
		const FLinearColor Ink = Paper();
		return FLinearColor(Ink.R, Ink.G, Ink.B, 0.62f);
	}

	FSlateFontInfo PosterFont(int32 Size, int32 Tracking)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
		Info.LetterSpacing = Tracking;
		return Info;
	}
}
