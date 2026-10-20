// Final Bell — how a named style wants the fight to look.
// Ratings still decide feet, power, guard, accuracy, and stamina.
// This policy only chooses distance, punch mix, and when to lead.
#pragma once

#include "CoreMinimal.h"

class UWorld;

struct FFBStylePolicy
{
	/** Distance, in Unreal units, where this style is doing its job. */
	float RangeMin = 120.f;
	float RangeMax = 175.f;

	float Jab = 1.f;
	float Straight = 1.f;
	float LeadHook = 0.5f;
	float RearHook = 0.5f;
	float LeadUppercut = 0.2f;
	float RearUppercut = 0.2f;

	/** Chance the punch is aimed at the body. 1 means every shot. */
	float BodyChance = 0.15f;

	/** Chance the punch uses the existing power flag. Not a second damage scale. */
	float PowerChance = 0.25f;

	float SecondsBetweenPunches = 0.55f;

	/** Stop punching while burst energy is under this share of the tank. */
	float RestFraction = 0.16f;

	/** How hard they walk forward when they are outside their range. */
	float Chase = 1.f;

	/** Lateral step while they hold range. Out-boxers circle. Sluggers do not. */
	float Circle = 0.f;

	/** Step to the fighter's left so the rear hand lines up. */
	float SouthpawStep = 0.f;

	/** Stand between the opponent and the center of the ring. */
	float CutRing = 0.f;

	/** 1 blocks before it punches. */
	float Shell = 0.f;

	/** Share of exchanges they refuse to lead. A counter waits for the miss. */
	float WaitForMiss = 0.f;

	/** Chance they slip, roll, or lean when a punch starts. */
	float Slip = 0.25f;

	/** How many punches an exchange prefers. 1 is a jab and leave. */
	float PreferredPunches = 1.8f;

	/** Step off to invite a lead. A counter draws. A swarmer does not. */
	float Draw = 0.15f;

	/** Cut the ring when the other fighter is seen stepping away. */
	float CutRetreat = 0.30f;

	/** Guard on the way in. High guard and peek-a-boo sit near 1. */
	float EntryGuard = 0.35f;

	/** How much a created angle is worth after the exchange. */
	float AnglePreference = 0.40f;

	/** Style name for logs. Stance is not copied into this string. */
	FString Name;

	static FFBStylePolicy ForStyle(const FString& Style);
	static bool FindRingCenter(const UWorld* World, FVector& OutCenter);
	static float RangeFit(float Distance, const FFBStylePolicy& Policy);
};
