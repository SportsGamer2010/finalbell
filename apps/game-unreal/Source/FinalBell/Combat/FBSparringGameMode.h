// Final Bell — sparring flow + mode-routed AI / MP / Training
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Combat/FBCombatTypes.h"
#include "FBSparringGameMode.generated.h"

class AFBBoxerCharacter;
class AFBHeavyBag;

UENUM(BlueprintType)
enum class EFBBoutPhase : uint8
{
	PreBout,
	RoundActive,
	BetweenRounds,
	Complete
};

UCLASS()
class FINALBELL_API AFBSparringGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFBSparringGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	EFBBoutPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Bout")
	void StartBout();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Session")
	void ApplyPlayMode(EFBPlayMode Mode);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	int32 TotalRounds = 3;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	float RoundDurationSeconds = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	float RestDurationSeconds = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|P1")
	bool bAutoBuildP1Arena = true;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|P1")
	bool bAutoStartBout = true;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|P1")
	bool bSpawnAiIfSolo = true;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	EFBBoutPhase Phase = EFBBoutPhase::PreBout;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 CurrentRound = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Session")
	EFBPlayMode ActivePlayMode = EFBPlayMode::Unset;

	float PhaseTimeRemaining = 0.f;
	bool bResultFinalized = false;
	bool bArenaBuilt = false;
	int32 NextStartIndex = 0;
	FTimerHandle ArenaBuildTimer;

	void AdvancePhase();
	void FinalizeResultOnce();
	void TryBuildArenaAndStart();
	void PlaceFightersForMatch();
	void DestroyAllAiOpponents();
	/** Keeps one AI opponent in solo sparring, and none in multiplayer or training. */
	void EnforceRoster();
	void DestroyTrainingBag();
	int32 CountHumanBoxers() const;
	AFBHeavyBag* EnsureTrainingBag();
};
