// Editable title, palette, and where Training sends the player.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FBMenuDeveloperSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Final Bell Menu"))
class FINALBELL_API UFBMenuDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFBMenuDeveloperSettings();

	static const UFBMenuDeveloperSettings* Get();

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FString GameTitle = TEXT("FINAL BELL");

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FLinearColor Charcoal = FLinearColor(0.0667f, 0.0706f, 0.0784f, 0.94f);

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FLinearColor WarmWhite = FLinearColor(0.9569f, 0.9412f, 0.9020f, 1.f);

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FLinearColor BoxingRed = FLinearColor(0.7765f, 0.1765f, 0.1765f, 1.f);

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FLinearColor MutedGold = FLinearColor(0.7137f, 0.6039f, 0.3529f, 1.f);

	/** Level that plays behind the title and menu. */
	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FString MenuMap = TEXT("/Game/Maps/Ring_P1");

	/** Long package name of the training level. The sparring game mode owns that map. */
	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu")
	FString TrainingMap = TEXT("/Game/Maps/Ring_P1");

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu", meta=(ClampMin="0.0", ClampMax="8.0"))
	float MenuCameraDriftDegrees = 3.5f;

	UPROPERTY(Config, EditAnywhere, Category="FinalBell|Menu", meta=(ClampMin="0.0"))
	float FadeSeconds = 0.35f;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
