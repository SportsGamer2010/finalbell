// Planted head movement. The stick grades the pose. Holding the mode is not a slip.
#pragma once

#include "CoreMinimal.h"
#include "Combat/FBCombatTypes.h"
#include "FBHeadMovement.generated.h"

/**
 * Trunk command for a planted head movement.
 * BuildDefenseBend stacks pelvis 0.22, waist 0.40, mid 0.24, and chest 0.14,
 * so the chest reaches this command. The neck gives back 0.40 and the head
 * keeps 0.20, which leaves the head near 0.80 of the command.
 * A 74° command was about 59° at the head. These caps stay in a readable range.
 */
USTRUCT(BlueprintType)
struct FFBHeadMovementLimits
{
	GENERATED_BODY()

	/** L2 held, stick centered. A chin tuck, not the full pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float NeutralPitch = 6.f;

	/** Full lateral slip, trunk degrees. Stick right is negative roll (head to the fighter's left). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float SlipRoll = 22.f;

	/** Full lean away. Negative folds the chest back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "-30.0", ClampMax = "0.0"))
	float LeanBackPitch = -16.f;

	/** Full forward level change, trunk degrees. The knees take the rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float LevelPitch = 10.f;

	/** Pelvis drop at a full forward stick, centimeters. Feet stay planted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float LevelKneeDropUU = 12.f;

	/** Roll under, only after the stick travels the arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float RollUnderPitch = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float RollUnderRoll = 14.f;

	/**
	 * Combat gate on the continuous axis. Under this, the pose still grades
	 * and the sample is not a completed slip, lean, or roll.
	 * This does not zero the stick. The only stick dead zone is the mapping's 0.22.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "0.3", ClampMax = "0.9"))
	float CompletedAxis = 0.55f;

	/** R2 plus stick down, and only while L2 is not eating the stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Defense", meta = (ClampMin = "-1.0", ClampMax = "-0.2"))
	float LowGuardStickY = -0.45f;
};

UENUM(BlueprintType)
enum class EFBHeadMoveKind : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Neutral UMETA(DisplayName = "Neutral"),
	Slip UMETA(DisplayName = "Slip"),
	LeanBack UMETA(DisplayName = "Lean Back"),
	LevelChange UMETA(DisplayName = "Level Change"),
	RollUnder UMETA(DisplayName = "Roll Under")
};

/** One stick sample while L2 is held. Not replicated. */
struct FFBHeadStickSample
{
	float Time = 0.f;
	FVector2D Axis = FVector2D::ZeroVector;
};

namespace FFBHeadMovement
{
	/** How long a stick sweep may take and still count as one roll-under arc. */
	inline constexpr float ArcWindowSeconds = 0.40f;
	/** Samples in the arc must be a real push. This does not remap the axis. */
	inline constexpr float ArcMinMagnitude = 0.45f;
	inline constexpr float RollUnderHoldSeconds = 0.35f;

	struct FFBHeadPose
	{
		float Pitch = 0.f;
		float Roll = 0.f;
		float KneeDropUU = 0.f;
	};

	FVector2D ClampAxes(FVector2D Axis);

	/** True when a planted head movement is legal. Down and clinch are not. */
	bool StateAllows(bool bKnockedDown, bool bInClinch);

	/** L2 eats the left stick only while the planted movement is legal. */
	bool ConsumesLocomotionStick(bool bWeaveHeld, bool bKnockedDown, bool bInClinch);

	EFBHeadMoveKind Classify(FVector2D Axis, bool bRollUnderHold);

	/**
	 * A completed slip, lean, or roll. Neutral, a small deflection, and a
	 * forward level change do not qualify.
	 */
	bool CountsAsCompletedEvasion(EFBHeadMoveKind Kind, FVector2D Axis, float CompletedAxis);

	bool EvasionOnTime(
		float Now,
		float SlipStart,
		float SlipEnd,
		float HeadSince,
		EFBHeadMoveKind Kind,
		FVector2D Axis,
		float CompletedAxis,
		float ActiveWindowStart);

	FFBHeadPose PoseFromAxes(FVector2D Axis, EFBHeadMoveKind Kind, float RollUnderSign, const FFBHeadMovementLimits& Limits);

	/**
	 * L1, R1, and L3. Forward on this path is the explicit roll under.
	 * L2 forward is a level change and does not call this.
	 * LeadSideSign is -1 orthodox, +1 southpaw.
	 */
	FFBHeadPose ExplicitSlipPose(FVector2D Axis, float LeadSideSign, const FFBHeadMovementLimits& Limits);

	/**
	 * Owner shows the stick they are holding. A remote shows the replicated
	 * axes, including when their velocity is zero.
	 */
	FVector2D PresentationAxes(bool bOwner, FVector2D LocalStick, FVector2D ReplicatedAxes);

	/**
	 * R2 selects the guard. While L2 consumes the stick, stick down does not
	 * change it. A low guard latched earlier in this R2 hold is kept.
	 */
	EFBGuardState SelectBlockGuard(bool bBlockHeld, bool bHeadConsumesStick, bool bLatchedLow, float StickY, float LowGuardStickY);

	void TrimHistory(TArray<FFBHeadStickSample>& History, float Now, float WindowSeconds);

	/**
	 * True when the stick traveled from one side, through forward, to the other
	 * side inside the window. OutSign is the exit side: +1 right, -1 left.
	 */
	bool DetectRollUnderArc(const TArray<FFBHeadStickSample>& History, float Now, float& OutSign);
}
