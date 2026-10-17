// Final Bell — three-round sparring flow + automated P1 arena
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FBSparringGameMode.generated.h"

class AFBBoxerCharacter;
class AFBBroadcastCamera;

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
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	EFBBoutPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Bout")
	void StartBout();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	int32 TotalRounds = 3;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	float RoundDurationSeconds = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Bout")
	float RestDurationSeconds = 60.f;

	/** When true, GameMode spawns floor/lights/ring/opponent/camera without a custom .umap. */
	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|P1")
	bool bAutoBuildP1Arena = true;

	/** Auto-start the three-round clock after arena build (P1 convenience). */
	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|P1")
	bool bAutoStartBout = true;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	EFBBoutPhase Phase = EFBBoutPhase::PreBout;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 CurrentRound = 0;

	float PhaseTimeRemaining = 0.f;
	bool bResultFinalized = false;
	bool bArenaBuilt = false;
	FTimerHandle ArenaBuildTimer;

	void AdvancePhase();
	void FinalizeResultOnce();
	void TryBuildArenaAndStart();
};
