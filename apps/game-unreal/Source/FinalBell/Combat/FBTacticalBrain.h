// Final Bell — tactical state machine, observations, and utility scoring.
// The controller submits intents. This class does not write damage or stamina.
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBStylePolicy.h"
#include "Combat/FBTacticalTypes.h"

enum class EFBAIActionKind : uint8
{
	None,
	Jab,
	Cross,
	LeadHook,
	RearHook,
	LeadUppercut,
	RearUppercut,
	GuardHigh,
	GuardLow,
	GuardRelease,
	Slip,
	Roll,
	Recover
};

struct FFBExchangeBeat
{
	EFBAttackId Attack = EFBAttackId::Jab;
	bool bBody = false;
	bool bPower = false;
	bool bGuardHeld = false;
};

struct FFBExchangeRuntime
{
	FName Id = NAME_None;
	TArray<FFBExchangeBeat> Beats;
	int32 Next = 0;
	int32 Accepted = 0;
	int32 RejectStreak = 0;
	bool bActive = false;
	float StartedAt = -1.f;

	void Clear()
	{
		Id = NAME_None;
		Beats.Reset();
		Next = 0;
		Accepted = 0;
		RejectStreak = 0;
		bActive = false;
		StartedAt = -1.f;
	}

	bool HasNext() const
	{
		return bActive && Beats.IsValidIndex(Next);
	}

	void NoteResult(EFBIntentResult Result)
	{
		if (!bActive)
		{
			return;
		}
		if (Result == EFBIntentResult::Accepted || Result == EFBIntentResult::Buffered)
		{
			++Next;
			++Accepted;
			RejectStreak = 0;
			if (Next >= Beats.Num())
			{
				bActive = false;
			}
		}
		else if (Result != EFBIntentResult::RejectedSequence)
		{
			++RejectStreak;
		}
	}
};

struct FFBAttackCue
{
	int32 AttackerId = 0;
	int32 AttackInstanceId = 0;
	EFBAttackId Attack = EFBAttackId::None;
	bool bBody = false;
	bool bPower = false;
	float ObservedAt = 0.f;
	float VisibleAt = 0.f;
	float ExpiresAt = 0.f;
	float Confidence = 0.f;
	bool bVisible = false;
	bool bConsumed = false;
	EFBContactResult Outcome = EFBContactResult::Miss;
	bool bOutcomeKnown = false;
	float OutcomeVisibleAt = 0.f;
};

struct FFBTendencyMemory
{
	int32 Leads = 0;
	int32 HeadShots = 0;
	int32 BodyShots = 0;
	int32 Combos = 0;
	int32 GuardSamples = 0;
	int32 OpenSamples = 0;
	int32 RetreatSamples = 0;
	FVector RetreatDir = FVector::ZeroVector;
	int32 OwnEntries = 0;
	int32 OwnMisses = 0;
	float UpdatedAt = -1.f;

	void DecayToward(float Now, float Window)
	{
		if (UpdatedAt < 0.f || Window <= 0.f)
		{
			UpdatedAt = Now;
			return;
		}
		const float Age = Now - UpdatedAt;
		if (Age > Window)
		{
			Leads = HeadShots = BodyShots = Combos = GuardSamples = OpenSamples = RetreatSamples = 0;
			OwnEntries = OwnMisses = 0;
			RetreatDir = FVector::ZeroVector;
		}
		UpdatedAt = Now;
	}

	int32 Samples() const
	{
		return Leads + HeadShots + BodyShots + GuardSamples + OpenSamples;
	}
};

struct FFBActionChoice
{
	EFBAIActionKind Kind = EFBAIActionKind::None;
	EFBAttackId Attack = EFBAttackId::None;
	bool bBody = false;
	bool bPower = false;
	bool bGuardHeld = false;
	EFBGuardState Guard = EFBGuardState::Open;
	EFBRingAction RingAction = EFBRingAction::None;
	FVector2D MoveAxis = FVector2D::ZeroVector;
	float Score = 0.f;
	bool bSubmit = false;
	FString Label;
};

struct FFBTacticalView
{
	float Now = 0.f;
	bool bHasOpponent = false;
	bool bAiMode = false;
	bool bRoundActive = false;
	bool bPaused = false;
	bool bKnockedDown = false;
	bool bOpponentDown = false;
	bool bClinch = false;
	bool bAttackCommitted = false;
	bool bComboBufferOpen = false;
	bool bFacingLocked = false;
	bool bLastMoveFailed = false;
	int32 FailedMoveBucket = INDEX_NONE;
	float Distance = 0.f;
	float FacingDot = 1.f;
	float ClosingSpeed = 0.f;
	float Energy = 100.f;
	float EnergyMax = 100.f;
	float Fatigue = 0.f;
	float Ceiling = 100.f;
	float BlockHealth = 100.f;
	float BlockMax = 100.f;
	float Stun = 0.f;
	float JabCost = 4.f;
	float PunchRegenDelay = 0.f;
	float MinReach = 60.f;
	float MaxReach = 200.f;
	float StartupSeconds = 0.08f;
	float RopeClearance = 1000.f;
	EFBGuardState OwnGuard = EFBGuardState::Open;
	EFBGuardState SeenGuard = EFBGuardState::Open;
	FVector ToOpponent = FVector::ZeroVector;
	FVector OpponentVelocity = FVector::ZeroVector;
	FVector SaferDirection = FVector::ZeroVector;
	FVector Right = FVector::ZeroVector;
	float AttackCost[8] = { 0.f, 4.f, 8.f, 9.f, 10.f, 10.f, 11.f, 0.f };
};

struct FFBTacticalDecision
{
	EFBTacticalState State = EFBTacticalState::Inactive;
	FString Reason;
	FVector MoveDesired = FVector::ZeroVector;
	int32 MoveBucket = 0;
	FFBActionChoice Action;
	float TopScore = 0.f;
	FString ScoreLine;
};

struct FFBTacticalTelemetry
{
	int32 Exchanges = 0;
	int32 ExchangePunches = 0;
	int32 Rejects = 0;
	int32 Misses = 0;
	int32 RopeStalls = 0;
	int32 RecoverEntries = 0;
	float LastDwell = 0.f;
	float LastEntrySpeed = 0.f;
	float LastResponseLatency = -1.f;
	float ResponseLatencySum = 0.f;
	int32 ResponseSamples = 0;
};

struct FFBTacticalBrain
{
	void Reset(const TCHAR* Reason, float Now);
	void SetSeed(int32 Seed);
	void SetDifficulty(EFBAIDifficulty Difficulty);
	void SetStyle(const FFBStylePolicy& InPolicy, EFBStance InStance);
	void SetTuning(const FFBTacticalTuning& InTuning);

	void NoteAttackCue(
		int32 AttackerId,
		int32 AttackInstanceId,
		EFBAttackId Attack,
		bool bBody,
		bool bPower,
		float Now,
		float FacingDot,
		float Distance);

	void NoteImpact(int32 AttackerId, int32 AttackInstanceId, const FFBImpactEvent& Impact, float Now);
	void NoteOwnImpact(const FFBImpactEvent& Impact, float Now);
	void NoteIntent(EFBIntentResult Result, EFBAIActionKind Kind);
	void ExpireCues(float Now);

	FFBTacticalDecision Decide(const FFBTacticalView& View);
	bool ConsumeReadyDefense(float Now, float StartupSeconds, FFBActionChoice& OutAction);
	bool TryBufferBeat(const FFBTacticalView& View, FFBActionChoice& OutAction);

	const FFBTacticalTelemetry& GetTelemetry() const { return Telemetry; }
	EFBTacticalState GetState() const { return State; }
	const FString& GetReason() const { return Reason; }
	EFBAIDifficulty GetDifficulty() const { return Difficulty; }
	const FFBAIDifficultyProfile& GetProfile() const { return Profile; }
	const FFBStylePolicy& GetPolicy() const { return Policy; }
	bool HasPlan() const { return Plan.bActive; }
	int32 PlanIndex() const { return Plan.Next; }
	int32 CueCount() const { return Cues.Num(); }
	bool IsRangeLatched() const { return bInPreferredRange; }
	float GetFatigueEstimate() const { return FatigueEstimate; }
	float GetFatigueUncertainty() const { return Profile.FatigueUncertainty; }
	void DebugSetPlan(TArray<FFBExchangeBeat> Beats, float Now);
	FString DebugLine(const FFBTacticalView& View, const FFBTacticalDecision& Decision) const;

private:
	void Transition(EFBTacticalState Next, const TCHAR* Why, float Now, bool bHard);
	void ClearPlan();
	bool HasReadyThreat(float Now) const;
	const FFBAttackCue* FindReadyCue(float Now) const;
	void RememberOutcome(const FFBAttackCue& Cue);
	float StyleWeight(EFBAttackId Attack) const;
	float AttackCost(EFBAttackId Attack, const FFBTacticalView& View) const;
	bool BeatLegal(const FFBExchangeBeat& Beat, const FFBTacticalView& View) const;
	void ChoosePlan(const FFBTacticalView& View);
	FFBActionChoice ScoreActions(const FFBTacticalView& View, FString& OutLine, float& OutTop) ;
	FVector ChooseFootwork(const FFBTacticalView& View, int32& OutBucket) const;
	void NormalizeWeights();

	FFBTacticalTuning Tuning;
	FFBStylePolicy Policy;
	EFBStance Stance = EFBStance::Orthodox;
	EFBAIDifficulty Difficulty = EFBAIDifficulty::Club;
	FFBAIDifficultyProfile Profile;
	FRandomStream Stream;

	EFBTacticalState State = EFBTacticalState::Measure;
	FString Reason = TEXT("Bell");
	float StateEnteredAt = 0.f;
	bool bInPreferredRange = false;
	int32 CircleSign = 1;
	int32 LastAttackIndex = INDEX_NONE;
	int32 RepeatCount = 0;
	int32 DefendedInstance = 0;
	float FatigueEstimate = 0.f;
	float ApproachBeganAt = -1.f;
	float ApproachBeganDistance = 0.f;
	FFBActionChoice Committed;
	bool bHasCommitment = false;

	TArray<FFBAttackCue> Cues;
	FFBExchangeRuntime Plan;
	FFBTendencyMemory Memory;
	FFBTacticalTelemetry Telemetry;
	float WeightN[9] = {};
};
