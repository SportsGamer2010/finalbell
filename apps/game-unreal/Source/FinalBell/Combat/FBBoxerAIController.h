// Final Bell — the opponent's decisions. The server still owns the punch.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBTacticalTypes.h"
#include "FBBoxerAIController.generated.h"

class AFBBoxerCharacter;
class UFBCombatComponent;
struct FFBTacticalBrain;
struct FFBActionChoice;

/**
 * Chooses steps and intents for the unpaired opponent.
 * Why: a default controller stands still, and a rusher never measures.
 * What: runs a tactical state machine on authority in versus-AI and career.
 * It submits FFBCombatIntent. It does not write damage, stamina, or the winner.
 */
UCLASS()
class FINALBELL_API AFBBoxerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFBBoxerAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	void SetDifficulty(EFBAIDifficulty Difficulty);
	void ResetTactics(const TCHAR* Reason);

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|AI")
	FFBTacticalTuning Tuning;

private:
	UFUNCTION()
	void HandleOpponentPhase(EFBAttackPhase Phase);

	UFUNCTION()
	void HandleOpponentImpact(const FFBImpactEvent& Impact);

	UFUNCTION()
	void HandleOwnImpact(const FFBImpactEvent& Impact);

	void BindPair(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent);
	void UnbindAll();
	EFBIntentResult Submit(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBActionChoice& Action);
	void ApplyFootwork(AFBBoxerCharacter* Self, const FVector& Desired, bool bScaleDown);
	void PushDifficultyFromMode();

	FFBTacticalBrain* Brain = nullptr;
	int32 NextSequence = 1;
	float DecisionLeft = 0.f;
	float FailTime = 0.f;
	float LastDebugTime = -1.f;
	int32 LastMoveBucket = INDEX_NONE;
	FVector SmoothedMove = FVector::ZeroVector;
	FVector MoveGoal = FVector::ZeroVector;
	FVector LastLocation = FVector::ZeroVector;
	bool bRoundLatched = false;
	bool bDownLatched = false;
	EFBTacticalState LastLoggedState = EFBTacticalState::Inactive;
	FString LastIntentText;
	TWeakObjectPtr<AFBBoxerCharacter> BoundOpponent;
	TWeakObjectPtr<UFBCombatComponent> BoundOpponentCombat;
	TWeakObjectPtr<UFBCombatComponent> BoundSelfCombat;
};
