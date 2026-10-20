// Final Bell — tactical AI types and the stamina, cue, and footwork rules the tests lock.
// Ratings and damage stay on the fighter. Difficulty never writes those numbers.
#pragma once

#include "CoreMinimal.h"
#include "Combat/FBCombatTypes.h"
#include "FBTacticalTypes.generated.h"

UENUM(BlueprintType)
enum class EFBTacticalState : uint8
{
	Measure UMETA(DisplayName = "Measure"),
	Approach UMETA(DisplayName = "Approach"),
	Exchange UMETA(DisplayName = "Exchange"),
	Defend UMETA(DisplayName = "Defend"),
	Recover UMETA(DisplayName = "Recover"),
	EscapeRopes UMETA(DisplayName = "Escape Ropes"),
	Hurt UMETA(DisplayName = "Hurt"),
	Inactive UMETA(DisplayName = "Inactive")
};

/** How well the opponent decides. Club is the default. It is not a rating. */
UENUM(BlueprintType)
enum class EFBAIDifficulty : uint8
{
	Novice UMETA(DisplayName = "Novice"),
	Club UMETA(DisplayName = "Club"),
	Expert UMETA(DisplayName = "Expert")
};

/** Editable defaults. The controller owns a copy. Difficulty does not scale these into damage or speed. */
USTRUCT(BlueprintType)
struct FINALBELL_API FFBTacticalTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Decision", meta = (ClampMin = "0.05", ClampMax = "0.50"))
	float DecisionIntervalMin = 0.10f;

	UPROPERTY(EditAnywhere, Category = "Decision", meta = (ClampMin = "0.05", ClampMax = "0.50"))
	float DecisionIntervalMax = 0.20f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.0"))
	float MinDwellSeconds = 0.40f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.2"))
	float MeasureMaxSeconds = 1.40f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.2"))
	float ApproachMaxSeconds = 2.60f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.2"))
	float ExchangeMaxSeconds = 2.40f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.2"))
	float DefendMaxSeconds = 0.95f;

	UPROPERTY(EditAnywhere, Category = "State", meta = (ClampMin = "0.2"))
	float EscapeMaxSeconds = 1.80f;

	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0.0"))
	float RangeHysteresis = 18.f;

	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0.0"))
	float RopeDanger = 72.f;

	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0.0"))
	float RopeClear = 118.f;

	UPROPERTY(EditAnywhere, Category = "Stamina", meta = (ClampMin = "0.05", ClampMax = "0.8"))
	float RecoverEnterFraction = 0.22f;

	UPROPERTY(EditAnywhere, Category = "Stamina", meta = (ClampMin = "0.1", ClampMax = "0.95"))
	float RecoverLeaveFraction = 0.46f;

	UPROPERTY(EditAnywhere, Category = "Stamina", meta = (ClampMin = "0.0"))
	float RecoverGap = 8.f;

	UPROPERTY(EditAnywhere, Category = "Stamina", meta = (ClampMin = "0.0"))
	float CeilingSlack = 0.40f;

	UPROPERTY(EditAnywhere, Category = "Stamina", meta = (ClampMin = "0.0"))
	float AttackReserve = 6.f;

	UPROPERTY(EditAnywhere, Category = "Threat", meta = (ClampMin = "0.0"))
	float SafeDistanceMargin = 28.f;

	UPROPERTY(EditAnywhere, Category = "Threat", meta = (ClampMin = "0.05"))
	float StaleCueSeconds = 0.40f;

	UPROPERTY(EditAnywhere, Category = "Threat", meta = (ClampMin = "0.05"))
	float AnswerWindow = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float HurtEnterStun = 58.f;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float HurtLeaveStun = 34.f;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float SeeDistance = 280.f;

	UPROPERTY(EditAnywhere, Category = "Utility", meta = (ClampMin = "0.0"))
	float SwitchMargin = 0.16f;

	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.5"))
	float MoveEase = 7.f;

	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.05"))
	float MoveFailSeconds = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CommittedMoveScale = 0.18f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightContact = 0.22f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightOpening = 0.14f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightStamina = 0.16f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightCounterRisk = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightStyle = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightTendency = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightPosition = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightRepetition = 0.04f;

	UPROPERTY(EditAnywhere, Category = "Utility")
	float WeightPlan = 0.04f;
};

/** Decision quality only. No damage, health, speed, stamina, or information the fighter cannot see. */
struct FFBAIDifficultyProfile
{
	float DecisionInterval = 0.15f;
	float RecognitionDelay = 0.07f;
	float ErrorRate = 0.18f;
	float ScoreNoise = 0.09f;
	float MemorySeconds = 6.f;
	int32 MaxExchangePunches = 2;
	float ProactiveGuard = 0.50f;
	float FatigueUncertainty = 14.f;
	int32 EvidenceNeeded = 3;
};

/** Numbers the difficulty path is not allowed to rewrite. */
struct FFBPhysicalCombatValues
{
	float StartupSeconds = 0.08f;
	float JabCost = 4.f;
	float JabDamage = 6.f;
	float MaxBurstEnergy = 100.f;
	float WalkSpeed = 320.f;
};

namespace FFBTactical
{
	inline EFBAIDifficulty SanitizeDifficulty(EFBAIDifficulty Difficulty)
	{
		const uint8 Raw = static_cast<uint8>(Difficulty);
		if (Raw > static_cast<uint8>(EFBAIDifficulty::Expert))
		{
			return EFBAIDifficulty::Club;
		}
		return Difficulty;
	}

	inline FFBAIDifficultyProfile ProfileFor(EFBAIDifficulty Difficulty)
	{
		switch (SanitizeDifficulty(Difficulty))
		{
		case EFBAIDifficulty::Novice:
			return { 0.20f, 0.16f, 0.42f, 0.22f, 2.5f, 1, 0.28f, 22.f, 4 };
		case EFBAIDifficulty::Expert:
			return { 0.10f, 0.035f, 0.08f, 0.04f, 10.f, 3, 0.70f, 8.f, 2 };
		default:
			return {};
		}
	}

	inline float ClampDecisionInterval(float Wanted, const FFBTacticalTuning& Tuning)
	{
		const float Lo = FMath::Min(Tuning.DecisionIntervalMin, Tuning.DecisionIntervalMax);
		const float Hi = FMath::Max(Tuning.DecisionIntervalMin, Tuning.DecisionIntervalMax);
		return FMath::Clamp(Wanted, Lo, Hi);
	}

	/** Direction and stick strength stay apart. A short vector must not become a full step. */
	inline void SeparateFootwork(const FVector& Desired, FVector& OutDirection, float& OutIntensity)
	{
		const float Magnitude = Desired.Size2D();
		if (Magnitude < KINDA_SMALL_NUMBER)
		{
			OutDirection = FVector::ZeroVector;
			OutIntensity = 0.f;
			return;
		}
		OutDirection = FVector(Desired.X / Magnitude, Desired.Y / Magnitude, 0.f);
		OutIntensity = FMath::Clamp(Magnitude, 0.f, 1.f);
	}

	inline bool UpdateRangeLatch(bool bLatched, float Distance, float RangeMin, float RangeMax, float Hysteresis)
	{
		if (bLatched)
		{
			return Distance >= RangeMin - Hysteresis && Distance <= RangeMax + Hysteresis;
		}
		return Distance >= RangeMin && Distance <= RangeMax;
	}

	inline float RecoverEnterEnergy(float Ceiling, float EnterFraction)
	{
		return FMath::Max(0.f, Ceiling) * EnterFraction;
	}

	/** Leave line stays under the fatigue ceiling, and above the enter line when the tank has room. */
	inline float RecoverLeaveEnergy(float Ceiling, float EnterFraction, float LeaveFraction, float Gap, float Slack)
	{
		const float SafeCeiling = FMath::Max(0.f, Ceiling);
		const float EnterAt = RecoverEnterEnergy(SafeCeiling, EnterFraction);
		float LeaveAt = FMath::Max(SafeCeiling * LeaveFraction, EnterAt + Gap);
		const float Cap = FMath::Max(0.f, SafeCeiling - Slack);
		if (LeaveAt > Cap)
		{
			LeaveAt = Cap;
		}
		return LeaveAt;
	}

	inline bool WantsEnterRecover(float Energy, float Ceiling, const FFBTacticalTuning& Tuning, float JabCost, EFBGuardState Guard, bool bDanger)
	{
		const float EnterAt = RecoverEnterEnergy(Ceiling, Tuning.RecoverEnterFraction);
		const float LeaveAt = RecoverLeaveEnergy(Ceiling, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
		if (Energy <= EnterAt)
		{
			return true;
		}
		if (!bDanger && Guard != EFBGuardState::Open && Energy < LeaveAt)
		{
			return true;
		}
		if (!bDanger && Energy + 0.5f < JabCost && Energy < LeaveAt)
		{
			return true;
		}
		return false;
	}

	inline bool WantsLeaveRecover(float Energy, float Ceiling, const FFBTacticalTuning& Tuning)
	{
		const float LeaveAt = RecoverLeaveEnergy(Ceiling, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
		return Energy + KINDA_SMALL_NUMBER >= LeaveAt;
	}

	inline bool CanExitSoft(float EnteredAt, float Now, float MinDwell)
	{
		return (Now - EnteredAt) >= MinDwell;
	}

	/**
	 * Contact chance from reach and the glove, not from the style's preferred range.
	 * Predicted distance uses the startup the punch actually has.
	 */
	inline float PunchContactSuitability(
		EFBAttackId Attack,
		float Distance,
		float FacingDot,
		float ClosingSpeed,
		float MinReach,
		float MaxReach,
		float StartupSeconds,
		bool bBody)
	{
		if (FacingDot < 0.20f || MaxReach <= MinReach)
		{
			return 0.f;
		}
		float BandMin = 0.55f;
		float BandMax = 0.90f;
		switch (Attack)
		{
		case EFBAttackId::Jab: BandMin = 0.62f; BandMax = 0.94f; break;
		case EFBAttackId::Cross: BandMin = 0.55f; BandMax = 0.90f; break;
		case EFBAttackId::LeadHook: BandMin = 0.40f; BandMax = 0.70f; break;
		case EFBAttackId::RearHook: BandMin = 0.38f; BandMax = 0.68f; break;
		case EFBAttackId::LeadUppercut: BandMin = 0.34f; BandMax = 0.60f; break;
		case EFBAttackId::RearUppercut: BandMin = 0.32f; BandMax = 0.58f; break;
		default: return 0.f;
		}
		if (bBody)
		{
			BandMin = FMath::Max(0.28f, BandMin - 0.06f);
			BandMax = FMath::Max(BandMin + 0.08f, BandMax - 0.05f);
		}
		const float Predicted = Distance - ClosingSpeed * FMath::Max(0.f, StartupSeconds);
		if (Predicted < MinReach || Predicted > MaxReach)
		{
			return 0.f;
		}
		const float IdealMin = MinReach + (MaxReach - MinReach) * BandMin;
		const float IdealMax = MinReach + (MaxReach - MinReach) * BandMax;
		if (Predicted >= IdealMin && Predicted <= IdealMax)
		{
			return FMath::Clamp(FacingDot, 0.f, 1.f);
		}
		const float Edge = Predicted < IdealMin ? IdealMin : IdealMax;
		const float Span = FMath::Max(20.f, (IdealMax - IdealMin) * 0.5f);
		return FMath::Clamp(1.f - FMath::Abs(Predicted - Edge) / Span, 0.f, 1.f) * FMath::Clamp(FacingDot, 0.f, 1.f);
	}

	inline int32 PickWithNoise(const TArray<float>& Scores, FRandomStream& Stream, float Noise, float ErrorRate)
	{
		if (Scores.Num() == 0)
		{
			return INDEX_NONE;
		}
		int32 Best = 0;
		int32 Second = 0;
		float BestScore = -1.e9f;
		float SecondScore = -1.e9f;
		for (int32 Index = 0; Index < Scores.Num(); ++Index)
		{
			const float Noisy = Scores[Index] + Stream.FRandRange(-Noise, Noise);
			if (Noisy > BestScore)
			{
				Second = Best;
				SecondScore = BestScore;
				Best = Index;
				BestScore = Noisy;
			}
			else if (Noisy > SecondScore)
			{
				Second = Index;
				SecondScore = Noisy;
			}
		}
		if (Scores.Num() > 1 && Stream.FRand() < ErrorRate)
		{
			return Second == Best ? (Best == 0 ? 1 : 0) : Second;
		}
		return Best;
	}
}
