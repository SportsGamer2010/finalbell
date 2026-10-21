// Reads replicated defense and locomotion for a future boxing AnimBP.
// This class does not play clips. PoseMesh remains the visible body until a
// boxing AnimBP is assigned and seen in play.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FBBoxerAnimInstance.generated.h"

class AFBBoxerCharacter;

UCLASS(Transient)
class FINALBELL_API UFBBoxerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Opponent-relative velocity, about -1 to 1. For an in-place shuffle blend space. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float MoveForward = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float MoveRight = 0.f;

	/** Planted head-movement stick. Positive X is the fighter's right. Positive Y is toward the opponent. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float HeadAxisX = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float HeadAxisY = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float DefensePitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float DefenseRoll = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float KneeDrop = 0.f;

	/** 0 open, 1 high, 2 low. Head movement is not a guard value. */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	uint8 Guard = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	bool bSouthpaw = false;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	float PunchAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	uint8 AttackId = 0;

	/**
	 * A punch layer may play above the pelvis. It must not replace the legs.
	 * Weight transfer has to come from clips. This stays false until those clips exist.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Anim")
	bool bPunchMasksLegs = false;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
