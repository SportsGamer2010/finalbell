// Volume, display, and reduced motion. Display confirmation is handled by the menu.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Combat/FBTacticalTypes.h"
#include "FBPlayerSettings.generated.h"

UCLASS()
class FINALBELL_API UFBPlayerSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	static const TCHAR* SlotName;

	UPROPERTY()
	float MasterVolume = 1.f;

	UPROPERTY()
	bool bReducedMotion = false;

	/** Last AI difficulty on the fighter screen. Old saves load Club. */
	UPROPERTY()
	EFBAIDifficulty AIDifficulty = EFBAIDifficulty::Club;

	static UFBPlayerSettingsSave* LoadOrCreate();
	void WriteToDisk() const;
	void ApplyAudio(const UObject* WorldContext) const;
};
