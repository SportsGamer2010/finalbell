// Final Bell — sparring flow + mode-routed AI / MP / Training
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBTacticalTypes.h"
#include "FBSparringGameMode.generated.h"

class AFBBoxerCharacter;
class AFBHeavyBag;
class AFBBroadcastCamera;
struct FFBImpactEvent;

/**
 * What the judges read for one corner. Server only.
 * Generalship is who dictated their range and the center. It is not a punch total.
 * PunchCredit is the punch-count pillar: a clean punch is 1, a block is 0.25.
 * HeadDealt and BodyDealt stay as a record. The judges do not score them.
 */
struct FFBCornerLedger
{
	float Generalship = 0.f;
	float PunchCredit = 0.f;
	float HeadDealt = 0.f;
	float BodyDealt = 0.f;
	int32 Knockdowns = 0;
	int32 PointDeductions = 0;
};

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
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	EFBBoutPhase GetPhase() const { return Phase; }

	EFBPlayMode GetPlayMode() const { return ActivePlayMode; }

	bool IsPresentationPaused() const { return bPresentationPaused; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Bout")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Bout")
	void StartBout();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Session")
	void ApplyPlayMode(EFBPlayMode Mode);

	/** 1v1 vs AI is waiting on the fighter screen. The arena timer must not start the clock. */
	void SetAwaitingFighterConfirm(bool bAwaiting);

	/** Drops knockdown, clinch, clock, and the card without writing a result. */
	void AbandonBoutForMenu();

	/** Same two fighters, same corners, a new 3:00 bell. Does not rebuild the gym. */
	void RequestRematch();

	/** Blue and red corners from the ring volume. Used for the first bell, a rematch, and the return after the round. */
	void SendFightersToCorners();

	/** The confirming player must possess a boxer before the round accepts input. */
	void EnsureConfirmingPlayerPossessesFighter();

	/** False during a knockdown, a rest, after the result, and while the bell is waiting on a glove. */
	bool AllowsNewAttacks() const;

	/** Host only. Freezes the bout clock and the count. A remote client cannot set this. */
	void SetPresentationPaused(bool bPaused);

	bool HadRosterRefusal(FString& OutWhy) const;

	/** The controller that confirmed the pair. Their pawn receives the player build. */
	void NoteBoutRequester(AController* Requester);

	/** Novice, Club, or Expert. Rematch keeps the value from the confirmed bout. */
	void SetAiDifficulty(EFBAIDifficulty Difficulty);
	EFBAIDifficulty GetAiDifficulty() const { return AiDifficulty; }

	/** Drops plans and cues on the AI when the bell rings. Difficulty stays. */
	void ResetAiTactics(const TCHAR* Reason);

	/**
	 * Why: spending the last of the gas tank drops a fighter. The client does not decide this.
	 * What: starts the referee's count if the bout is live and this fighter is not already down.
	 */
	void NotifyEnergyDepleted(AFBBoxerCharacter* Boxer);

	/** A fighter can ask the server to start the count. The server still owns the knockdown. */
	void BeginKnockdown(AFBBoxerCharacter* Boxer);

	/**
	 * Why: the judges need a record of who landed clean work.
	 * What: adds punch credit and damage to the attacker's corner ledger.
	 */
	void NoteImpact(AFBBoxerCharacter* Attacker, const FFBImpactEvent& Impact);

	/**
	 * Why: clinch, fouls, the push, and the stance change are referee matters.
	 * What: the server accepts or refuses the action. A client cannot deduct a point.
	 */
	void HandleRingAction(AFBBoxerCharacter* Boxer, const FFBCombatIntent& Intent, AActor* Opponent);

	/** Left stick while clinched. Fills that fighter's turn meter. */
	void AddClinchSteer(AFBBoxerCharacter* Boxer, float Axis);

	/**
	 * Why: getting up is a check the server scores, using the sticks the downed fighter is holding.
	 * What: compares both sticks to the rotating marks. A short hold beats the count.
	 */
	void SubmitGetUpAim(APlayerController* From, FVector2D LeftStick, FVector2D RightStick);

	void NotifyGuardBroken();

protected:
	/** Three three-minute rounds. The last bell sends it to the judges. */
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

	EFBAIDifficulty AiDifficulty = EFBAIDifficulty::Club;

	float PhaseTimeRemaining = 0.f;
	bool bResultFinalized = false;
	bool bKnockdownActive = false;
	float CountRemaining = 0.f;
	float LeftTargetDegrees = 30.f;
	float RightTargetDegrees = 210.f;
	float AlignSeconds = 0.f;
	float LastAimServerTime = -1.f;
	float AiGetUpDelay = 0.f;
	FFBOfficialCard BuiltCard;
	FFBCornerLedger BlueLedger;
	FFBCornerLedger RedLedger;
	int32 BoutBlueDeductions = 0;
	int32 BoutRedDeductions = 0;
	TWeakObjectPtr<AFBBoxerCharacter> DownedBoxer;
	TWeakObjectPtr<AActor> RefereeActor;
	TWeakObjectPtr<AFBBoxerCharacter> ClinchA;
	TWeakObjectPtr<AFBBoxerCharacter> ClinchB;
	TWeakObjectPtr<AFBBoxerCharacter> ClinchInitiator;
	float ClinchSeconds = 0.f;
	float RefereeCallUntil = 0.f;
	int32 NextFoulId = 100000;
	FString RefereeCall;
	bool bArenaBuilt = false;
	bool bAwaitingFighterConfirm = false;
	bool bPresentationPaused = false;
	FString LastRosterRefusal;
	TWeakObjectPtr<AController> BoutRequester;
	int32 NextStartIndex = 0;
	FTimerHandle ArenaBuildTimer;

	void AdvancePhase();
	void FinalizeResultOnce();
	void TryBuildArenaAndStart();
	void ResetBoutState();
	void PushBoutState();
	void CompleteGetUp();
	void FinishKnockout();
	void ScoreClosedRound();
	void BuildDecisionCard();
	void BuildKnockoutCard(AFBBoxerCharacter* Winner);
	void ShowReferee(AFBBoxerCharacter* Downed);
	void HideReferee();
	void SetRefereeCameras(bool bEnable, AFBBoxerCharacter* Downed);
	FFBCornerLedger& LedgerFor(const AFBBoxerCharacter* Boxer);
	int32 ScoreLoser(const FFBCornerLedger& Winner, const FFBCornerLedger& Loser, float Margin) const;
	float JudgePoints(const FFBCornerLedger& Ledger, int32 JudgeIndex) const;
	void TickClinch(float DeltaSeconds);
	void TickRingGeneralship(float DeltaSeconds);
	void BreakClinch(const TCHAR* Call);
	void DeductPoint(AFBBoxerCharacter* Boxer, const TCHAR* Reason);
	void SetRefereeCall(const FString& Call, float Seconds = 2.4f);
	bool AreClinching() const { return ClinchA.IsValid() && ClinchB.IsValid(); }
	void PlaceFightersForMatch();
	bool AnyGloveInFlight() const;
	void ApplyPendingBuilds();
	void RestoreBetweenRounds();
	void AwardCareerFromCard();
	int32 TelemetryClean = 0;
	int32 TelemetryMiss = 0;
	int32 TelemetryBlock = 0;
	int32 TelemetryBody = 0;
	int32 TelemetryGuardBreaks = 0;
	int32 TelemetryDowns = 0;
	void DestroyAllAiOpponents();
	/**
	 * Why: editor play is two PIE clients. Versus AI was keeping both pawns and
	 * also spawning FB_P1Opponent, so the ring had three boxers.
	 * What: the confirming player stays. Every other player pawn is unpossessed,
	 * destroyed, and that controller spectates. Multiplayer does not call this.
	 */
	void BenchExtraHumansForSoloAi();
	/** Keeps one AI opponent in solo sparring, and none in multiplayer or training. */
	void EnforceRoster();
	void DestroyTrainingBag();
	int32 CountHumanBoxers() const;
	AFBHeavyBag* EnsureTrainingBag();
};
