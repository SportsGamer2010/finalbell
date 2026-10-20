#include "Combat/FBTacticalBrain.h"

namespace
{
	const TCHAR* AttackLabel(EFBAttackId Attack)
	{
		switch (Attack)
		{
		case EFBAttackId::Jab: return TEXT("jab");
		case EFBAttackId::Cross: return TEXT("cross");
		case EFBAttackId::LeadHook: return TEXT("lead-hook");
		case EFBAttackId::RearHook: return TEXT("rear-hook");
		case EFBAttackId::LeadUppercut: return TEXT("lead-upper");
		case EFBAttackId::RearUppercut: return TEXT("rear-upper");
		default: return TEXT("none");
		}
	}

	const TCHAR* StateLabel(EFBTacticalState Value)
	{
		switch (Value)
		{
		case EFBTacticalState::Measure: return TEXT("Measure");
		case EFBTacticalState::Approach: return TEXT("Approach");
		case EFBTacticalState::Exchange: return TEXT("Exchange");
		case EFBTacticalState::Defend: return TEXT("Defend");
		case EFBTacticalState::Recover: return TEXT("Recover");
		case EFBTacticalState::EscapeRopes: return TEXT("EscapeRopes");
		case EFBTacticalState::Hurt: return TEXT("Hurt");
		default: return TEXT("Inactive");
		}
	}

	EFBAttackId AttackFromKind(EFBAIActionKind Kind)
	{
		switch (Kind)
		{
		case EFBAIActionKind::Jab: return EFBAttackId::Jab;
		case EFBAIActionKind::Cross: return EFBAttackId::Cross;
		case EFBAIActionKind::LeadHook: return EFBAttackId::LeadHook;
		case EFBAIActionKind::RearHook: return EFBAttackId::RearHook;
		case EFBAIActionKind::LeadUppercut: return EFBAttackId::LeadUppercut;
		case EFBAIActionKind::RearUppercut: return EFBAttackId::RearUppercut;
		default: return EFBAttackId::None;
		}
	}

	EFBAIActionKind KindFromAttack(EFBAttackId Attack)
	{
		switch (Attack)
		{
		case EFBAttackId::Jab: return EFBAIActionKind::Jab;
		case EFBAttackId::Cross: return EFBAIActionKind::Cross;
		case EFBAttackId::LeadHook: return EFBAIActionKind::LeadHook;
		case EFBAttackId::RearHook: return EFBAIActionKind::RearHook;
		case EFBAttackId::LeadUppercut: return EFBAIActionKind::LeadUppercut;
		case EFBAttackId::RearUppercut: return EFBAIActionKind::RearUppercut;
		default: return EFBAIActionKind::None;
		}
	}

	FFBActionChoice GuardAction(EFBAIActionKind Kind, EFBGuardState Guard, bool bHeld, const TCHAR* Label)
	{
		FFBActionChoice Choice;
		Choice.Kind = Kind;
		Choice.bGuardHeld = bHeld;
		Choice.Guard = bHeld ? Guard : EFBGuardState::Open;
		Choice.bSubmit = true;
		Choice.Label = Label;
		Choice.Score = 1.f;
		return Choice;
	}

	FFBActionChoice PunchAction(const FFBExchangeBeat& Beat, float Score)
	{
		FFBActionChoice Choice;
		Choice.Kind = KindFromAttack(Beat.Attack);
		Choice.Attack = Beat.Attack;
		Choice.bBody = Beat.bBody;
		Choice.bPower = Beat.bPower;
		Choice.bGuardHeld = Beat.bGuardHeld;
		Choice.Guard = Beat.bGuardHeld ? EFBGuardState::High : EFBGuardState::Open;
		Choice.bSubmit = true;
		Choice.Score = Score;
		Choice.Label = FString::Printf(TEXT("%s%s%s"),
			AttackLabel(Beat.Attack),
			Beat.bBody ? TEXT(" body") : TEXT(" head"),
			Beat.bPower ? TEXT(" power") : TEXT(""));
		return Choice;
	}

	bool IsPunch(EFBAIActionKind Kind)
	{
		return AttackFromKind(Kind) != EFBAttackId::None;
	}
}

void FFBTacticalBrain::Reset(const TCHAR* ReasonText, float Now)
{
	ClearPlan();
	Cues.Reset();
	Memory = FFBTendencyMemory();
	Committed = FFBActionChoice();
	bHasCommitment = false;
	bInPreferredRange = false;
	LastAttackIndex = INDEX_NONE;
	RepeatCount = 0;
	DefendedInstance = 0;
	FatigueEstimate = 0.f;
	ApproachBeganAt = -1.f;
	ApproachBeganDistance = 0.f;
	State = EFBTacticalState::Measure;
	Reason = ReasonText ? ReasonText : TEXT("Reset");
	StateEnteredAt = Now;
	Telemetry = FFBTacticalTelemetry();
}

void FFBTacticalBrain::SetSeed(int32 Seed)
{
	Stream.Initialize(Seed == 0 ? 1 : Seed);
	CircleSign = (Seed & 1) ? 1 : -1;
}

void FFBTacticalBrain::SetDifficulty(EFBAIDifficulty InDifficulty)
{
	Difficulty = FFBTactical::SanitizeDifficulty(InDifficulty);
	Profile = FFBTactical::ProfileFor(Difficulty);
}

void FFBTacticalBrain::SetStyle(const FFBStylePolicy& InPolicy, EFBStance InStance)
{
	Policy = InPolicy;
	Stance = InStance;
}

void FFBTacticalBrain::SetTuning(const FFBTacticalTuning& InTuning)
{
	Tuning = InTuning;
	NormalizeWeights();
}

void FFBTacticalBrain::NormalizeWeights()
{
	const float Raw[] = {
		Tuning.WeightContact,
		Tuning.WeightOpening,
		Tuning.WeightStamina,
		Tuning.WeightCounterRisk,
		Tuning.WeightStyle,
		Tuning.WeightTendency,
		Tuning.WeightPosition,
		Tuning.WeightRepetition,
		Tuning.WeightPlan
	};
	float Sum = 0.f;
	for (float Value : Raw)
	{
		Sum += FMath::Max(0.f, Value);
	}
	if (Sum <= KINDA_SMALL_NUMBER)
	{
		Sum = 1.f;
	}
	for (int32 Index = 0; Index < 9; ++Index)
	{
		WeightN[Index] = FMath::Max(0.f, Raw[Index]) / Sum;
	}
}

void FFBTacticalBrain::ClearPlan()
{
	if (Plan.bActive || Plan.Accepted > 0)
	{
		if (Plan.Accepted > 0)
		{
			++Telemetry.Exchanges;
			Telemetry.ExchangePunches += Plan.Accepted;
		}
	}
	Plan.Clear();
}

void FFBTacticalBrain::DebugSetPlan(TArray<FFBExchangeBeat> Beats, float Now)
{
	Plan.Clear();
	Plan.Beats = MoveTemp(Beats);
	Plan.bActive = Plan.Beats.Num() > 0;
	Plan.StartedAt = Now;
	Plan.Id = TEXT("debug");
}

void FFBTacticalBrain::NoteAttackCue(
	int32 AttackerId,
	int32 AttackInstanceId,
	EFBAttackId Attack,
	bool bBody,
	bool bPower,
	float Now,
	float FacingDot,
	float Distance)
{
	if (Attack == EFBAttackId::None || AttackInstanceId <= 0)
	{
		return;
	}
	for (const FFBAttackCue& Existing : Cues)
	{
		if (Existing.AttackerId == AttackerId && Existing.AttackInstanceId == AttackInstanceId)
		{
			return;
		}
	}
	FFBAttackCue Cue;
	Cue.AttackerId = AttackerId;
	Cue.AttackInstanceId = AttackInstanceId;
	Cue.Attack = Attack;
	Cue.bBody = bBody;
	Cue.bPower = bPower;
	Cue.ObservedAt = Now;
	Cue.VisibleAt = Now + Profile.RecognitionDelay;
	Cue.ExpiresAt = Cue.VisibleAt + Tuning.StaleCueSeconds;
	Cue.bVisible = FacingDot >= 0.20f && Distance <= Tuning.SeeDistance;
	const float Facing = FMath::Clamp((FacingDot - 0.20f) / 0.80f, 0.f, 1.f);
	const float Near = 1.f - FMath::Clamp(Distance / FMath::Max(1.f, Tuning.SeeDistance), 0.f, 1.f);
	Cue.Confidence = Cue.bVisible ? (0.35f + 0.40f * Facing + 0.25f * Near) * FMath::Clamp(Profile.ProactiveGuard + 0.45f, 0.4f, 1.f) : 0.f;
	if (!Cue.bVisible)
	{
		return;
	}
	Cues.Add(Cue);
	if (Cues.Num() > 12)
	{
		Cues.RemoveAt(0);
	}
}

void FFBTacticalBrain::NoteImpact(int32 AttackerId, int32 AttackInstanceId, const FFBImpactEvent& Impact, float Now)
{
	for (FFBAttackCue& Cue : Cues)
	{
		if (Cue.AttackerId == AttackerId && Cue.AttackInstanceId == AttackInstanceId)
		{
			Cue.bOutcomeKnown = true;
			Cue.Outcome = Impact.Result;
			Cue.OutcomeVisibleAt = Now + Profile.RecognitionDelay;
			return;
		}
	}
}

void FFBTacticalBrain::NoteOwnImpact(const FFBImpactEvent& Impact, float Now)
{
	++Memory.OwnEntries;
	if (Impact.Result == EFBContactResult::Miss)
	{
		++Memory.OwnMisses;
		++Telemetry.Misses;
	}
	Memory.UpdatedAt = Now;
}

void FFBTacticalBrain::NoteIntent(EFBIntentResult Result, EFBAIActionKind Kind)
{
	if (IsPunch(Kind))
	{
		Plan.NoteResult(Result);
		if (Result == EFBIntentResult::Accepted || Result == EFBIntentResult::Buffered)
		{
			const int32 Index = static_cast<int32>(AttackFromKind(Kind));
			RepeatCount = Index == LastAttackIndex ? RepeatCount + 1 : 1;
			LastAttackIndex = Index;
		}
	}
	if (Result != EFBIntentResult::Accepted && Result != EFBIntentResult::Buffered)
	{
		++Telemetry.Rejects;
	}
	if (Plan.RejectStreak >= 2)
	{
		ClearPlan();
	}
}

void FFBTacticalBrain::RememberOutcome(const FFBAttackCue& Cue)
{
	if (Cue.Attack == EFBAttackId::Jab || Cue.Attack == EFBAttackId::Cross)
	{
		++Memory.Leads;
	}
	else
	{
		++Memory.Combos;
	}
	if (Cue.bBody)
	{
		++Memory.BodyShots;
	}
	else
	{
		++Memory.HeadShots;
	}
	Memory.UpdatedAt = Cue.OutcomeVisibleAt;
	FatigueEstimate = FMath::Clamp(FatigueEstimate + (Cue.bPower ? 4.f : 2.f), 0.f, 70.f);
}

void FFBTacticalBrain::ExpireCues(float Now)
{
	Memory.DecayToward(Now, Profile.MemorySeconds);
	for (FFBAttackCue& Cue : Cues)
	{
		if (Cue.bOutcomeKnown && !Cue.bConsumed && Now >= Cue.OutcomeVisibleAt)
		{
			RememberOutcome(Cue);
			Cue.bConsumed = true;
		}
	}
	Cues.RemoveAll([Now](const FFBAttackCue& Cue)
	{
		return Now > Cue.ExpiresAt;
	});
	FatigueEstimate = FMath::Max(0.f, FatigueEstimate - 0.15f);
}

const FFBAttackCue* FFBTacticalBrain::FindReadyCue(float Now) const
{
	const FFBAttackCue* Best = nullptr;
	for (const FFBAttackCue& Cue : Cues)
	{
		if (!Cue.bVisible || Cue.bConsumed || Now < Cue.VisibleAt || Now > Cue.ExpiresAt)
		{
			continue;
		}
		if (!Best || Cue.Confidence > Best->Confidence)
		{
			Best = &Cue;
		}
	}
	return Best;
}

bool FFBTacticalBrain::HasReadyThreat(float Now) const
{
	const FFBAttackCue* Cue = FindReadyCue(Now);
	return Cue && Cue->Confidence >= 0.35f && !Cue->bOutcomeKnown;
}

void FFBTacticalBrain::Transition(EFBTacticalState Next, const TCHAR* Why, float Now, bool bHard)
{
	if (Next == State)
	{
		if (Why && Reason.IsEmpty())
		{
			Reason = Why;
		}
		return;
	}
	if (!bHard && !FFBTactical::CanExitSoft(StateEnteredAt, Now, Tuning.MinDwellSeconds))
	{
		return;
	}
	if (State == EFBTacticalState::Approach && ApproachBeganAt >= 0.f)
	{
		const float Elapsed = FMath::Max(0.05f, Now - ApproachBeganAt);
		Telemetry.LastEntrySpeed = FMath::Max(0.f, ApproachBeganDistance) / Elapsed;
	}
	Telemetry.LastDwell = FMath::Max(0.f, Now - StateEnteredAt);
	if (Next == EFBTacticalState::Recover)
	{
		++Telemetry.RecoverEntries;
	}
	if (Next == EFBTacticalState::Approach)
	{
		ApproachBeganAt = Now;
		ApproachBeganDistance = 0.f;
	}
	State = Next;
	Reason = Why ? Why : TEXT("Change");
	StateEnteredAt = Now;
	bHasCommitment = false;
}

float FFBTacticalBrain::StyleWeight(EFBAttackId Attack) const
{
	switch (Attack)
	{
	case EFBAttackId::Jab: return Policy.Jab;
	case EFBAttackId::Cross: return Policy.Straight;
	case EFBAttackId::LeadHook: return Policy.LeadHook;
	case EFBAttackId::RearHook: return Policy.RearHook;
	case EFBAttackId::LeadUppercut: return Policy.LeadUppercut;
	case EFBAttackId::RearUppercut: return Policy.RearUppercut;
	default: return 0.f;
	}
}

float FFBTacticalBrain::AttackCost(EFBAttackId Attack, const FFBTacticalView& View) const
{
	const int32 Index = static_cast<int32>(Attack);
	if (Index < 0 || Index >= UE_ARRAY_COUNT(View.AttackCost))
	{
		return 99.f;
	}
	return View.AttackCost[Index];
}

bool FFBTacticalBrain::BeatLegal(const FFBExchangeBeat& Beat, const FFBTacticalView& View) const
{
	if (!View.bRoundActive || View.bPaused || View.bKnockedDown || View.bClinch || View.bOpponentDown || !View.bAiMode)
	{
		return false;
	}
	if (View.bAttackCommitted && !View.bComboBufferOpen)
	{
		return false;
	}
	if (View.FacingDot < 0.20f)
	{
		return false;
	}
	const float Cost = AttackCost(Beat.Attack, View) * (Beat.bPower ? 1.15f : 1.f);
	const float Reserve = (Plan.bActive && Plan.Next == 0) ? Tuning.AttackReserve : 1.5f;
	if (View.Energy < Cost + (Beat.bPower ? Reserve : 0.5f))
	{
		return false;
	}
	if (View.Energy - Cost < 0.5f)
	{
		return false;
	}
	const float Suit = FFBTactical::PunchContactSuitability(
		Beat.Attack, View.Distance, View.FacingDot, View.ClosingSpeed, View.MinReach, View.MaxReach, View.StartupSeconds, Beat.bBody);
	return Suit > 0.15f;
}

void FFBTacticalBrain::ChoosePlan(const FFBTacticalView& View)
{
	struct FPlan
	{
		const TCHAR* Name = TEXT("jab");
		FFBExchangeBeat Beats[3];
		int32 Count = 1;
		float Want = 1.f;
	};
	const bool bGuard = Policy.EntryGuard >= 0.45f || Policy.Shell >= 0.5f;
	FPlan Library[5];
	auto Fill = [bGuard](FPlan& Plan, const TCHAR* Name, int32 Count, float Want, EFBAttackId A, bool bBodyA, EFBAttackId B, bool bBodyB, EFBAttackId C, bool bBodyC)
	{
		Plan.Name = Name;
		Plan.Count = Count;
		Plan.Want = Want;
		Plan.Beats[0] = { A, bBodyA, false, bGuard };
		if (Count > 1)
		{
			Plan.Beats[1] = { B, bBodyB, false, bGuard };
		}
		if (Count > 2)
		{
			Plan.Beats[2] = { C, bBodyC, false, bGuard };
		}
	};
	Fill(Library[0], TEXT("jab"), 1, 1.2f, EFBAttackId::Jab, false, EFBAttackId::None, false, EFBAttackId::None, false);
	Fill(Library[1], TEXT("jab-cross"), 2, 2.0f, EFBAttackId::Jab, false, EFBAttackId::Cross, false, EFBAttackId::None, false);
	Fill(Library[2], TEXT("jab-cross-hook"), 3, 3.0f, EFBAttackId::Jab, false, EFBAttackId::Cross, false, EFBAttackId::LeadHook, false);
	Fill(Library[3], TEXT("jab-body"), 1, 1.1f, EFBAttackId::Jab, true, EFBAttackId::None, false, EFBAttackId::None, false);
	Fill(Library[4], TEXT("jab-cross-body"), 2, 2.1f, EFBAttackId::Jab, false, EFBAttackId::Cross, true, EFBAttackId::None, false);

	float Best = -1.f;
	int32 BestIndex = 0;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FPlan& Candidate = Library[Index];
		if (Candidate.Count > Profile.MaxExchangePunches)
		{
			continue;
		}
		float CostSum = 0.f;
		bool bLegal = true;
		for (int32 BeatIndex = 0; BeatIndex < Candidate.Count; ++BeatIndex)
		{
			FFBExchangeBeat Beat = Candidate.Beats[BeatIndex];
			if (BeatIndex == 0 && !BeatLegal(Beat, View))
			{
				bLegal = false;
				break;
			}
			CostSum += AttackCost(Beat.Attack, View);
		}
		if (!bLegal || View.Energy < CostSum + Tuning.AttackReserve * 0.35f)
		{
			continue;
		}
		float Score = 1.f - FMath::Min(FMath::Abs(Candidate.Want - Policy.PreferredPunches) / 3.f, 1.f);
		Score += 0.25f * FFBTactical::PunchContactSuitability(
			Candidate.Beats[0].Attack, View.Distance, View.FacingDot, View.ClosingSpeed,
			View.MinReach, View.MaxReach, View.StartupSeconds, Candidate.Beats[0].bBody);
		if (Candidate.Beats[0].bBody)
		{
			Score += Policy.BodyChance * 0.35f;
		}
		if (Memory.Samples() >= Profile.EvidenceNeeded && Memory.BodyShots > Memory.HeadShots && Candidate.Beats[0].bBody)
		{
			Score += 0.15f;
		}
		Score += Stream.FRandRange(-Profile.ScoreNoise, Profile.ScoreNoise);
		if (Score > Best)
		{
			Best = Score;
			BestIndex = Index;
		}
	}
	if (Best < 0.f)
	{
		return;
	}
	Plan.Clear();
	const FPlan& Chosen = Library[BestIndex];
	for (int32 BeatIndex = 0; BeatIndex < Chosen.Count; ++BeatIndex)
	{
		Plan.Beats.Add(Chosen.Beats[BeatIndex]);
	}
	Plan.bActive = true;
	Plan.StartedAt = View.Now;
	Plan.Id = Chosen.Name;
}

FFBActionChoice FFBTacticalBrain::ScoreActions(const FFBTacticalView& View, FString& OutLine, float& OutTop)
{
	struct FScored
	{
		FFBExchangeBeat Beat;
		float Score = 0.f;
	};
	TArray<FScored> Choices;
	const EFBAttackId Ids[] = {
		EFBAttackId::Jab, EFBAttackId::Cross, EFBAttackId::LeadHook,
		EFBAttackId::RearHook, EFBAttackId::LeadUppercut, EFBAttackId::RearUppercut
	};
	const bool bEvidence = Memory.Samples() >= Profile.EvidenceNeeded;
	for (EFBAttackId Id : Ids)
	{
		for (int32 Body = 0; Body < 2; ++Body)
		{
			for (int32 Power = 0; Power < 2; ++Power)
			{
				if (Power && Policy.PowerChance <= 0.02f)
				{
					continue;
				}
				FFBExchangeBeat Beat;
				Beat.Attack = Id;
				Beat.bBody = Body == 1;
				Beat.bPower = Power == 1;
				Beat.bGuardHeld = Policy.EntryGuard >= 0.45f || Policy.Shell >= 0.5f;
				if (!BeatLegal(Beat, View))
				{
					continue;
				}
				const float Contact = FFBTactical::PunchContactSuitability(
					Id, View.Distance, View.FacingDot, View.ClosingSpeed, View.MinReach, View.MaxReach, View.StartupSeconds, Beat.bBody);
				const float Opening = HasReadyThreat(View.Now) ? 0.15f : (Policy.WaitForMiss > 0.5f ? 0.25f : 0.55f);
				const float Tired = FMath::Clamp((FatigueEstimate - Profile.FatigueUncertainty) / 40.f, 0.f, 0.25f);
				const float Cost = AttackCost(Id, View);
				const float After = View.Energy - Cost;
				const float LeaveAt = FFBTactical::RecoverLeaveEnergy(
					View.Ceiling, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
				float Stamina = After >= LeaveAt ? 1.f : FMath::Clamp(After / FMath::Max(1.f, LeaveAt), 0.f, 1.f);
				if (Beat.bPower)
				{
					Stamina *= Policy.PowerChance;
				}
				const float Counter = HasReadyThreat(View.Now) ? 0.2f : 0.7f;
				const float StyleFit = FMath::Clamp(StyleWeight(Id) / 5.f, 0.f, 1.f);
				float Tendency = 0.5f;
				if (bEvidence)
				{
					const float BodyMix = static_cast<float>(Memory.BodyShots) / FMath::Max(1, Memory.HeadShots + Memory.BodyShots);
					Tendency = Beat.bBody ? BodyMix : (1.f - BodyMix);
				}
				const float Position = View.RopeClearance < Tuning.RopeClear ? 0.25f : 0.8f;
				const int32 Index = static_cast<int32>(Id);
				const float Repetition = (Index == LastAttackIndex && RepeatCount >= 2) ? 0.15f : 0.85f;
				const float PlanFit = 0.55f;
				const float Score =
					WeightN[0] * Contact +
					WeightN[1] * (Opening + Tired) +
					WeightN[2] * Stamina +
					WeightN[3] * Counter +
					WeightN[4] * StyleFit +
					WeightN[5] * Tendency +
					WeightN[6] * Position +
					WeightN[7] * Repetition +
					WeightN[8] * PlanFit;
				FScored Row;
				Row.Beat = Beat;
				Row.Score = Score;
				Choices.Add(Row);
			}
		}
	}
	Choices.Sort([](const FScored& A, const FScored& B) { return A.Score > B.Score; });
	if (Choices.Num() > 4)
	{
		Choices.SetNum(4);
	}
	OutTop = Choices.Num() > 0 ? Choices[0].Score : 0.f;
	OutLine.Reset();
	for (int32 Index = 0; Index < Choices.Num() && Index < 3; ++Index)
	{
		if (Index > 0)
		{
			OutLine += TEXT(" | ");
		}
		OutLine += FString::Printf(TEXT("%s %.2f"), AttackLabel(Choices[Index].Beat.Attack), Choices[Index].Score);
	}
	if (Choices.Num() == 0)
	{
		return FFBActionChoice();
	}
	TArray<float> Scores;
	for (const FScored& Row : Choices)
	{
		Scores.Add(Row.Score);
	}
	if (bHasCommitment && Committed.Attack != EFBAttackId::None)
	{
		for (const FScored& Row : Choices)
		{
			if (Row.Beat.Attack == Committed.Attack && Row.Beat.bBody == Committed.bBody && Row.Score + Tuning.SwitchMargin >= OutTop)
			{
				return Committed;
			}
		}
	}
	const int32 Picked = FFBTactical::PickWithNoise(Scores, Stream, Profile.ScoreNoise, Profile.ErrorRate);
	FFBActionChoice Choice = PunchAction(Choices[Picked].Beat, Choices[Picked].Score);
	Committed = Choice;
	bHasCommitment = true;
	return Choice;
}

FVector FFBTacticalBrain::ChooseFootwork(const FFBTacticalView& View, int32& OutBucket) const
{
	const FVector To = View.ToOpponent.GetSafeNormal2D();
	const FVector Right = View.Right.GetSafeNormal2D();
	const float GapOut = View.Distance - Policy.RangeMax;
	const float GapIn = Policy.RangeMin - View.Distance;
	float Southpaw = Policy.SouthpawStep;
	if (Stance == EFBStance::Southpaw)
	{
		Southpaw = FMath::Max(Southpaw, 0.55f);
	}

	struct FStep
	{
		FVector Vector = FVector::ZeroVector;
		float Score = 0.f;
		int32 Bucket = 0;
	};
	FStep Steps[7];
	Steps[0] = { FVector::ZeroVector, 0.15f, 0 };
	const float InIntensity = FMath::Clamp(GapOut / 110.f, 0.f, 1.f) * FMath::Clamp(Policy.Chase, 0.f, 1.f);
	Steps[1] = { To * InIntensity, GapOut > 0.f ? 0.55f + 0.35f * InIntensity : 0.05f, 1 };
	const float Back = Policy.Shell >= 0.5f ? 0.40f : 0.85f;
	const float OutIntensity = FMath::Clamp(GapIn / 80.f, 0.f, 1.f) * Back;
	Steps[2] = { -To * OutIntensity, GapIn > 0.f ? 0.60f : 0.08f, 2 };
	const float Circle = Policy.Circle * 0.85f;
	Steps[3] = { Right * CircleSign * FMath::Clamp(Circle, 0.f, 1.f), Circle > 0.05f ? 0.35f + Policy.AnglePreference * 0.2f : 0.02f, 3 };
	Steps[4] = { -Right * CircleSign * FMath::Max(Southpaw, Circle * 0.45f), Southpaw > 0.2f ? 0.40f : 0.12f, 4 };
	const float ExitIntensity = View.RopeClearance < Tuning.RopeClear ? FMath::Clamp(1.f - View.RopeClearance / Tuning.RopeClear, 0.35f, 1.f) : 0.f;
	Steps[5] = { View.SaferDirection.GetSafeNormal2D() * ExitIntensity, ExitIntensity > 0.f ? 0.75f + ExitIntensity : 0.f, 5 };
	const FVector OppVel = View.OpponentVelocity.GetSafeNormal2D();
	const bool bRetreating = !OppVel.IsNearlyZero() && FVector::DotProduct(OppVel, To) > 0.35f;
	const FVector Cut = (To + OppVel).GetSafeNormal2D();
	Steps[6] = { Cut * (bRetreating ? FMath::Clamp(Policy.CutRetreat, 0.f, 1.f) : 0.f), bRetreating ? Policy.CutRetreat * 0.85f : 0.f, 6 };

	if (State == EFBTacticalState::Recover || State == EFBTacticalState::Hurt)
	{
		Steps[1].Score = 0.02f;
		Steps[2].Score += 0.45f;
		Steps[2].Vector = -To * FMath::Max(OutIntensity, 0.45f);
	}
	else if (State == EFBTacticalState::Measure)
	{
		Steps[1].Vector = To * FMath::Clamp(InIntensity, 0.f, 0.55f);
		Steps[1].Score = GapOut > 12.f ? 0.48f : 0.12f;
		Steps[3].Score += 0.18f;
	}
	else if (State == EFBTacticalState::Exchange || View.bAttackCommitted)
	{
		for (FStep& Step : Steps)
		{
			Step.Vector *= Tuning.CommittedMoveScale;
			Step.Score *= 0.35f;
		}
		Steps[0].Score = 0.5f;
	}
	else if (State == EFBTacticalState::EscapeRopes)
	{
		Steps[5].Score += 0.8f;
		Steps[1].Score = 0.02f;
	}
	else if (State == EFBTacticalState::Defend && Policy.Draw > 0.4f)
	{
		Steps[2].Score += 0.25f;
	}

	if (Policy.Draw > 0.45f && State == EFBTacticalState::Measure && !HasReadyThreat(View.Now))
	{
		Steps[2].Score += Policy.Draw * 0.35f;
		Steps[2].Vector = -To * FMath::Max(OutIntensity, 0.35f);
		Steps[1].Score *= 0.45f;
	}

	int32 Best = 0;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		if (View.bLastMoveFailed && Index == View.FailedMoveBucket)
		{
			Steps[Index].Score = -1.f;
		}
		if (Steps[Index].Vector.Size2D() < 0.04f && Index != 0)
		{
			Steps[Index].Score = -0.5f;
		}
		if (Steps[Index].Score > Steps[Best].Score)
		{
			Best = Index;
		}
	}
	OutBucket = Steps[Best].Bucket;
	return Steps[Best].Vector;
}

bool FFBTacticalBrain::TryBufferBeat(const FFBTacticalView& View, FFBActionChoice& OutAction)
{
	if (!Plan.HasNext() || !View.bComboBufferOpen)
	{
		return false;
	}
	const FFBExchangeBeat& Beat = Plan.Beats[Plan.Next];
	if (!BeatLegal(Beat, View))
	{
		Plan.NoteResult(EFBIntentResult::RejectedPhase);
		++Telemetry.Rejects;
		if (Plan.RejectStreak >= 2)
		{
			ClearPlan();
		}
		return false;
	}
	OutAction = PunchAction(Beat, 0.8f);
	return true;
}

bool FFBTacticalBrain::ConsumeReadyDefense(float Now, float StartupSeconds, FFBActionChoice& OutAction)
{
	const FFBAttackCue* Cue = FindReadyCue(Now);
	if (!Cue || Cue->bOutcomeKnown || Cue->AttackInstanceId == DefendedInstance)
	{
		return false;
	}
	const float Age = Now - Cue->ObservedAt;
	const float StartupLeft = StartupSeconds - Age;
	const bool bOnTime = StartupLeft > 0.02f;
	const bool bHook = Cue->Attack == EFBAttackId::LeadHook || Cue->Attack == EFBAttackId::RearHook;
	if (bHook && bOnTime && Policy.Slip > 0.2f && Cue->Confidence > 0.45f)
	{
		OutAction = GuardAction(EFBAIActionKind::Slip, EFBGuardState::Open, false, TEXT("slip"));
		OutAction.RingAction = EFBRingAction::Slip;
		OutAction.MoveAxis = FVector2D(CircleSign > 0 ? 1.f : -1.f, 0.f);
	}
	else if (!bOnTime && Cue->bBody)
	{
		OutAction = GuardAction(EFBAIActionKind::GuardLow, EFBGuardState::Low, true, TEXT("late-low"));
	}
	else
	{
		OutAction = GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, bOnTime ? TEXT("reactive-high") : TEXT("late-high"));
	}
	DefendedInstance = Cue->AttackInstanceId;
	Telemetry.LastResponseLatency = Age;
	Telemetry.ResponseLatencySum += Age;
	++Telemetry.ResponseSamples;
	return true;
}

FFBTacticalDecision FFBTacticalBrain::Decide(const FFBTacticalView& View)
{
	ExpireCues(View.Now);
	if (State == EFBTacticalState::Approach)
	{
		ApproachBeganDistance = FMath::Max(ApproachBeganDistance, View.Distance);
	}

	FFBTacticalDecision Out;
	auto Finish = [&](FFBActionChoice Action, FVector Move)
	{
		int32 Bucket = 0;
		if (Move.IsNearlyZero() && State != EFBTacticalState::Inactive && State != EFBTacticalState::Hurt)
		{
			Move = ChooseFootwork(View, Bucket);
		}
		else
		{
			Bucket = 0;
		}
		Out.State = State;
		Out.Reason = Reason;
		Out.MoveDesired = Move;
		Out.MoveBucket = Bucket;
		Out.Action = Action;
		return Out;
	};

	if (!View.bHasOpponent || !View.bAiMode)
	{
		Transition(EFBTacticalState::Inactive, TEXT("NoOpponent"), View.Now, true);
		ClearPlan();
		return Finish(FFBActionChoice(), FVector::ZeroVector);
	}
	if (!View.bRoundActive || View.bPaused)
	{
		Transition(EFBTacticalState::Inactive, View.bPaused ? TEXT("Paused") : TEXT("Phase"), View.Now, true);
		ClearPlan();
		return Finish(FFBActionChoice(), FVector::ZeroVector);
	}
	if (View.bKnockedDown)
	{
		Transition(EFBTacticalState::Hurt, TEXT("Knockdown"), View.Now, true);
		ClearPlan();
		return Finish(FFBActionChoice(), FVector::ZeroVector);
	}
	if (View.bClinch)
	{
		Transition(EFBTacticalState::Inactive, TEXT("Clinch"), View.Now, true);
		ClearPlan();
		return Finish(FFBActionChoice(), FVector::ZeroVector);
	}
	if (View.bOpponentDown)
	{
		Transition(EFBTacticalState::Inactive, TEXT("OpponentDown"), View.Now, true);
		ClearPlan();
		FVector Back = View.Distance < 170.f ? -View.ToOpponent.GetSafeNormal2D() * 0.40f : FVector::ZeroVector;
		return Finish(FFBActionChoice(), Back);
	}

	const bool bDanger = HasReadyThreat(View.Now);
	const bool bEnterGas = FFBTactical::WantsEnterRecover(
		View.Energy, View.Ceiling, Tuning, View.JabCost, View.OwnGuard, bDanger);
	const bool bLeaveGas = FFBTactical::WantsLeaveRecover(View.Energy, View.Ceiling, Tuning);
	bInPreferredRange = FFBTactical::UpdateRangeLatch(
		bInPreferredRange, View.Distance, Policy.RangeMin, Policy.RangeMax, Tuning.RangeHysteresis);

	const bool bRope = View.RopeClearance < Tuning.RopeDanger || (State == EFBTacticalState::EscapeRopes && View.RopeClearance < Tuning.RopeClear);
	const bool bHurt = View.Stun >= Tuning.HurtEnterStun || (State == EFBTacticalState::Hurt && View.Stun > Tuning.HurtLeaveStun);

	EFBTacticalState Desired = State;
	const TCHAR* Why = TEXT("Hold");
	bool bHard = false;
	if (bDanger && State != EFBTacticalState::Exchange)
	{
		Desired = EFBTacticalState::Defend;
		Why = TEXT("Threat");
		bHard = true;
	}
	else if (bHurt)
	{
		Desired = EFBTacticalState::Hurt;
		Why = TEXT("Stunned");
		bHard = true;
	}
	else if (bRope)
	{
		Desired = EFBTacticalState::EscapeRopes;
		Why = TEXT("Ropes");
		bHard = true;
		if (View.bLastMoveFailed)
		{
			CircleSign *= -1;
			++Telemetry.RopeStalls;
		}
	}
	else if (State == EFBTacticalState::Recover)
	{
		if (bLeaveGas)
		{
			Desired = bInPreferredRange ? EFBTacticalState::Measure : EFBTacticalState::Approach;
			Why = TEXT("GassedUp");
		}
		else
		{
			Desired = EFBTacticalState::Recover;
			Why = TEXT("Gas");
		}
	}
	else if (bEnterGas && !Plan.HasNext())
	{
		Desired = EFBTacticalState::Recover;
		Why = TEXT("Gas");
		bHard = true;
		ClearPlan();
	}
	else if (Plan.bActive)
	{
		Desired = EFBTacticalState::Exchange;
		Why = TEXT("Exchange");
	}
	else if (!bInPreferredRange)
	{
		Desired = EFBTacticalState::Approach;
		Why = TEXT("Range");
	}
	else
	{
		Desired = EFBTacticalState::Measure;
		Why = TEXT("Measure");
	}

	const float Dwell = View.Now - StateEnteredAt;
	if (State == EFBTacticalState::Measure && Dwell > Tuning.MeasureMaxSeconds && !bDanger && !bInPreferredRange)
	{
		Desired = EFBTacticalState::Approach;
		Why = TEXT("MeasureMax");
		bHard = true;
	}
	else if (State == EFBTacticalState::Approach && Dwell > Tuning.ApproachMaxSeconds && bInPreferredRange)
	{
		Desired = EFBTacticalState::Measure;
		Why = TEXT("ApproachMax");
		bHard = true;
	}
	else if (State == EFBTacticalState::Exchange && Dwell > Tuning.ExchangeMaxSeconds)
	{
		Desired = bEnterGas ? EFBTacticalState::Recover : EFBTacticalState::Measure;
		Why = TEXT("ExchangeMax");
		bHard = true;
		ClearPlan();
	}
	else if (State == EFBTacticalState::Defend && Dwell > Tuning.DefendMaxSeconds && !bDanger)
	{
		Desired = bInPreferredRange ? EFBTacticalState::Measure : EFBTacticalState::Approach;
		Why = TEXT("DefendMax");
		bHard = true;
	}
	else if (State == EFBTacticalState::EscapeRopes && Dwell > Tuning.EscapeMaxSeconds && View.RopeClearance >= Tuning.RopeClear)
	{
		Desired = EFBTacticalState::Measure;
		Why = TEXT("OffRopes");
		bHard = true;
	}

	Transition(Desired, Why, View.Now, bHard || Desired == State);

	int32 Bucket = 0;
	const FVector Move = ChooseFootwork(View, Bucket);
	FFBActionChoice Action;
	FString ScoreLine;
	float Top = 0.f;

	if (State == EFBTacticalState::Recover)
	{
		const bool bSafe = View.Distance > View.MaxReach + Tuning.SafeDistanceMargin && !bDanger;
		Action = bSafe
			? GuardAction(EFBAIActionKind::GuardRelease, EFBGuardState::Open, false, TEXT("release"))
			: GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("recover-guard"));
	}
	else if (State == EFBTacticalState::Hurt || State == EFBTacticalState::EscapeRopes)
	{
		Action = GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("shell"));
	}
	else if (State == EFBTacticalState::Defend)
	{
		Action = GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("defend"));
		const FFBAttackCue* Cue = FindReadyCue(View.Now);
		if (Cue && Cue->bOutcomeKnown && View.Now >= Cue->OutcomeVisibleAt && View.Now <= Cue->OutcomeVisibleAt + Tuning.AnswerWindow
			&& (Cue->Outcome == EFBContactResult::Miss || Cue->Outcome == EFBContactResult::Blocked)
			&& Policy.WaitForMiss > 0.4f && bInPreferredRange && !bEnterGas)
		{
			if (!Plan.bActive)
			{
				ChoosePlan(View);
			}
			if (Plan.HasNext())
			{
				Action = PunchAction(Plan.Beats[Plan.Next], 0.7f);
				Transition(EFBTacticalState::Exchange, TEXT("Counter"), View.Now, true);
			}
		}
	}
	else if (State == EFBTacticalState::Exchange)
	{
		if (!Plan.bActive)
		{
			ChoosePlan(View);
		}
		if (Plan.HasNext() && !View.bAttackCommitted)
		{
			if (BeatLegal(Plan.Beats[Plan.Next], View))
			{
				Action = PunchAction(Plan.Beats[Plan.Next], 0.75f);
			}
			else
			{
				Plan.NoteResult(EFBIntentResult::RejectedPhase);
				++Telemetry.Rejects;
				if (Plan.RejectStreak >= 2)
				{
					ClearPlan();
					Transition(EFBTacticalState::Measure, TEXT("Rejected"), View.Now, true);
				}
			}
		}
		else if (!Plan.bActive)
		{
			const bool bPress = Policy.CutRetreat > 0.6f && View.Energy > FFBTactical::RecoverLeaveEnergy(
				View.Ceiling, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
			Transition(bEnterGas ? EFBTacticalState::Recover : EFBTacticalState::Measure, bPress ? TEXT("Press") : TEXT("Reset"), View.Now, true);
			Action = bEnterGas
				? GuardAction(EFBAIActionKind::GuardRelease, EFBGuardState::Open, false, TEXT("release"))
				: GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("after"));
		}
	}
	else if (State == EFBTacticalState::Measure && bInPreferredRange && Dwell >= Tuning.MinDwellSeconds * 0.5f && !bEnterGas)
	{
		const bool bWait = Policy.WaitForMiss > 0.55f && !bDanger;
		if (!bWait)
		{
			Action = ScoreActions(View, ScoreLine, Top);
			if (Action.bSubmit)
			{
				FFBExchangeBeat First;
				First.Attack = Action.Attack;
				First.bBody = Action.bBody;
				First.bPower = Action.bPower;
				First.bGuardHeld = Action.bGuardHeld;
				Plan.Clear();
				Plan.Beats.Add(First);
				const bool bSecond = Profile.MaxExchangePunches >= 2 && Policy.PreferredPunches >= 1.6f && View.Energy > AttackCost(Action.Attack, View) + AttackCost(EFBAttackId::Cross, View) + Tuning.AttackReserve;
				if (bSecond && Action.Attack == EFBAttackId::Jab)
				{
					FFBExchangeBeat Cross;
					Cross.Attack = EFBAttackId::Cross;
					Cross.bBody = Policy.BodyChance > 0.65f;
					Cross.bGuardHeld = First.bGuardHeld;
					Plan.Beats.Add(Cross);
				}
				Plan.bActive = true;
				Plan.StartedAt = View.Now;
				Plan.Id = bSecond ? TEXT("built") : TEXT("single");
				Transition(EFBTacticalState::Exchange, TEXT("Lead"), View.Now, false);
			}
		}
		if (!Action.bSubmit && View.OwnGuard == EFBGuardState::Open && (Policy.EntryGuard > 0.35f || Policy.Shell > 0.4f || Profile.ProactiveGuard > 0.4f))
		{
			Action = GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("measure-guard"));
		}
	}
	else if (State == EFBTacticalState::Approach || State == EFBTacticalState::Measure)
	{
		const bool bGuardEntry = Policy.EntryGuard > 0.35f || Policy.Shell > 0.4f || Profile.ProactiveGuard > 0.45f;
		if (bGuardEntry && View.OwnGuard == EFBGuardState::Open && View.Distance < View.MaxReach + Tuning.SafeDistanceMargin)
		{
			Action = GuardAction(EFBAIActionKind::GuardHigh, EFBGuardState::High, true, TEXT("entry-guard"));
		}
	}

	Out.State = State;
	Out.Reason = Reason;
	Out.MoveDesired = Move;
	Out.MoveBucket = Bucket;
	Out.Action = Action;
	Out.TopScore = Top;
	Out.ScoreLine = ScoreLine;
	return Out;
}

FString FFBTacticalBrain::DebugLine(const FFBTacticalView& View, const FFBTacticalDecision& Decision) const
{
	const float EnterAt = FFBTactical::RecoverEnterEnergy(View.Ceiling, Tuning.RecoverEnterFraction);
	const float LeaveAt = FFBTactical::RecoverLeaveEnergy(
		View.Ceiling, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
	const TCHAR* Diff = Difficulty == EFBAIDifficulty::Novice ? TEXT("Novice") : (Difficulty == EFBAIDifficulty::Expert ? TEXT("Expert") : TEXT("Club"));
	FVector Dir = FVector::ZeroVector;
	float Intensity = 0.f;
	FFBTactical::SeparateFootwork(Decision.MoveDesired, Dir, Intensity);
	const FFBAttackCue* Cue = FindReadyCue(View.Now);
	const float CueAge = Cue ? View.Now - Cue->ObservedAt : -1.f;
	return FString::Printf(
		TEXT("AI %s %s %s | %s | E %.0f/%.0f fat %.0f ceil %.0f guard %d | enter %.0f leave %.0f | dist %.0f band %.0f-%.0f latched %d | move %d int %.2f | %s | %s | cue %.2f conf %.2f | x%d rej %d miss %d rope %d rec %d lat %.2f"),
		*Policy.Name,
		Diff,
		StateLabel(State),
		*Decision.Reason,
		View.Energy,
		View.EnergyMax,
		View.Fatigue,
		View.Ceiling,
		static_cast<int32>(View.OwnGuard),
		EnterAt,
		LeaveAt,
		View.Distance,
		Policy.RangeMin,
		Policy.RangeMax,
		bInPreferredRange ? 1 : 0,
		Decision.MoveBucket,
		Intensity,
		Decision.ScoreLine.IsEmpty() ? TEXT("-") : *Decision.ScoreLine,
		Decision.Action.Label.IsEmpty() ? TEXT("hold") : *Decision.Action.Label,
		CueAge,
		Cue ? Cue->Confidence : 0.f,
		Telemetry.ExchangePunches,
		Telemetry.Rejects,
		Telemetry.Misses,
		Telemetry.RopeStalls,
		Telemetry.RecoverEntries,
		Telemetry.LastResponseLatency);
}
