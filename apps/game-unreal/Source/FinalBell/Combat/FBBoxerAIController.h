// Final Bell — the opponent's decisions. The server still owns the punch.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/FBCombatTypes.h"
#include "FBBoxerAIController.generated.h"

class AFBBoxerCharacter;
struct FFBStylePolicy;

/**
 * Chooses the next step and the next intent for the unpaired opponent.
 * Why: a default AI controller stands still, so the style on the card never shows up.
 * What: moves and submits FFBCombatIntent. It does not write damage, stamina, or the winner.
 */
UCLASS()
class FINALBELL_API AFBBoxerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFBBoxerAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void StepFeet(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy) const;
	void Offer(
		AFBBoxerCharacter* Self,
		AFBBoxerCharacter* Opponent,
		EFBAttackId Attack,
		bool bGuardHeld,
		EFBGuardState Guard,
		bool bBody,
		bool bPower,
		EFBRingAction Action,
		FVector2D MoveAxis);
	void Throw(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy);
	void ReactToPunch(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy);

	int32 NextSequence = 1;
	float ThinkDelay = 0.5f;
	float AnswerUntil = -1.f;
	EFBAttackPhase PrevOppPhase = EFBAttackPhase::Idle;
	bool bRoundLatched = false;
};
