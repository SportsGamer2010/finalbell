// Saved created fighters. Premades are not stored here. Max 10.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Combat/FBFighterBalance.h"
#include "FBFighterSave.generated.h"

UCLASS()
class FINALBELL_API UFBFighterRosterSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotVersion = 1;

	UPROPERTY()
	int32 SchemaVersion = SlotVersion;

	UPROPERTY()
	TArray<FFBFighterRecord> Created;

	static UFBFighterRosterSave* LoadOrCreate();
	/** True only when the save has exactly one ready created fighter and a ready premade in that class. */
	static bool TrySoloCareerPair(FFBFighterRecord& OutPlayer, FFBFighterRecord& OutOpponent);
	void WriteToDisk() const;
	bool CanAdd() const { return Created.Num() < FFBFighterBalance::MaxCreated; }
	bool TryAdd(FFBFighterRecord Record, FString& OutWhy);
	bool TryReplace(const FString& Id, const FFBFighterRecord& Record, FString& OutWhy);
	void Remove(const FString& Id);
	FFBFighterRecord* FindMutable(const FString& Id);
};
