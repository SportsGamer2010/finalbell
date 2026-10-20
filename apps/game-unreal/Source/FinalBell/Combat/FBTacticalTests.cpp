#include "Misc/AutomationTest.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBTacticalBrain.h"
#include "Combat/FBTacticalTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FFBTacticalView LiveView(float Now, float Distance, float Energy, float Ceiling)
	{
		FFBTacticalView View;
		View.Now = Now;
		View.bHasOpponent = true;
		View.bAiMode = true;
		View.bRoundActive = true;
		View.Distance = Distance;
		View.FacingDot = 1.f;
		View.Energy = Energy;
		View.EnergyMax = 100.f;
		View.Fatigue = 100.f - Ceiling;
		View.Ceiling = Ceiling;
		View.JabCost = 4.f;
		View.MinReach = 60.f;
		View.MaxReach = 200.f;
		View.StartupSeconds = 0.08f;
		View.RopeClearance = 400.f;
		View.OwnGuard = EFBGuardState::Open;
		View.ToOpponent = FVector(1.f, 0.f, 0.f);
		View.Right = FVector(0.f, 1.f, 0.f);
		View.AttackCost[static_cast<int32>(EFBAttackId::Jab)] = 4.f;
		View.AttackCost[static_cast<int32>(EFBAttackId::Cross)] = 8.f;
		View.AttackCost[static_cast<int32>(EFBAttackId::LeadHook)] = 9.f;
		return View;
	}

	void Arm(FFBTacticalBrain& Brain)
	{
		FFBStylePolicy Policy;
		Policy.RangeMin = 140.f;
		Policy.RangeMax = 180.f;
		Policy.Chase = 0.80f;
		Policy.Circle = 0.20f;
		Policy.Jab = 3.f;
		Policy.Name = TEXT("Test");
		Brain.SetSeed(17);
		Brain.SetTuning(FFBTacticalTuning());
		Brain.SetDifficulty(EFBAIDifficulty::Club);
		Brain.SetStyle(Policy, EFBStance::Orthodox);
		Brain.Reset(TEXT("Test"), 0.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiFootworkTest,
	"FinalBell.AI.FootworkIntensity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiFootworkTest::RunTest(const FString& Parameters)
{
	FVector Direction = FVector::ZeroVector;
	float Intensity = 1.f;
	FFBTactical::SeparateFootwork(FVector(0.35f, 0.f, 0.f), Direction, Intensity);
	TestTrue(TEXT("Short step keeps its strength"), FMath::IsNearlyEqual(Intensity, 0.35f, 0.001f));
	TestTrue(TEXT("Direction is unit length"), Direction.GetSafeNormal2D().Equals(FVector(1.f, 0.f, 0.f), 0.001f));

	FFBTactical::SeparateFootwork(FVector(3.f, 4.f, 9.f), Direction, Intensity);
	TestTrue(TEXT("A long vector clamps at full stick"), FMath::IsNearlyEqual(Intensity, 1.f, 0.001f));
	TestTrue(TEXT("Vertical does not change the step"), FMath::IsNearlyZero(Direction.Z, 0.001f));

	FFBTacticalBrain Brain;
	Arm(Brain);
	const FFBTacticalDecision Early = Brain.Decide(LiveView(0.10f, 240.f, 80.f, 100.f));
	TestEqual(TEXT("The first look stays in Measure"), Early.State, EFBTacticalState::Measure);
	FFBTactical::SeparateFootwork(Early.MoveDesired, Direction, Intensity);
	TestTrue(TEXT("The first step is not a full shove"), Intensity > 0.05f && Intensity < 0.95f);

	const FFBTacticalDecision Later = Brain.Decide(LiveView(0.55f, 240.f, 80.f, 100.f));
	TestEqual(TEXT("After the dwell the boxer approaches"), Later.State, EFBTacticalState::Approach);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiHysteresisTest,
	"FinalBell.AI.Hysteresis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiHysteresisTest::RunTest(const FString& Parameters)
{
	bool bInside = FFBTactical::UpdateRangeLatch(false, 150.f, 140.f, 180.f, 18.f);
	TestTrue(TEXT("Inside the band latches"), bInside);
	bInside = FFBTactical::UpdateRangeLatch(true, 190.f, 140.f, 180.f, 18.f);
	TestTrue(TEXT("A small step outside the band still holds"), bInside);
	bInside = FFBTactical::UpdateRangeLatch(true, 210.f, 140.f, 180.f, 18.f);
	TestFalse(TEXT("Past the hysteresis the band lets go"), bInside);
	bInside = FFBTactical::UpdateRangeLatch(false, 190.f, 140.f, 180.f, 18.f);
	TestFalse(TEXT("Re-entry uses the tighter band"), bInside);

	const FFBTacticalTuning Tuning;
	const float Leave = FFBTactical::RecoverLeaveEnergy(100.f, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
	const float Enter = FFBTactical::RecoverEnterEnergy(100.f, Tuning.RecoverEnterFraction);
	TestTrue(TEXT("Leave sits above enter"), Leave > Enter + 5.f);
	TestFalse(TEXT("One regen tick does not clear Recover"), FFBTactical::WantsLeaveRecover(Enter + 12.f * 0.05f, 100.f, Tuning));
	TestTrue(TEXT("The high line does clear Recover"), FFBTactical::WantsLeaveRecover(Leave, 100.f, Tuning));
	TestFalse(TEXT("Mid gas does not enter on an open guard"), FFBTactical::WantsEnterRecover(Enter + 8.f, 100.f, Tuning, 4.f, EFBGuardState::Open, false));
	TestTrue(TEXT("Danger does not open Recover just because the guard is up"), FFBTactical::WantsEnterRecover(Enter + 8.f, 100.f, Tuning, 4.f, EFBGuardState::High, true) == false);
	TestTrue(TEXT("No danger and a held guard under the leave line starts Recover"), FFBTactical::WantsEnterRecover(Enter + 8.f, 100.f, Tuning, 4.f, EFBGuardState::High, false));

	const float Tight = FFBTactical::RecoverLeaveEnergy(10.f, Tuning.RecoverEnterFraction, Tuning.RecoverLeaveFraction, Tuning.RecoverGap, Tuning.CeilingSlack);
	TestTrue(TEXT("Fatigue still leaves a reachable exit"), Tight <= 10.f - Tuning.CeilingSlack + 0.01f && Tight > FFBTactical::RecoverEnterEnergy(10.f, Tuning.RecoverEnterFraction));
	TestTrue(TEXT("The tight exit can be reached"), FFBTactical::WantsLeaveRecover(Tight, 10.f, Tuning));
	TestFalse(TEXT("Just under the tight exit stays in Recover"), FFBTactical::WantsLeaveRecover(Tight - 1.f, 10.f, Tuning));

	TestTrue(TEXT("Min dwell blocks a soft exit"), !FFBTactical::CanExitSoft(0.f, 0.10f, Tuning.MinDwellSeconds));
	TestTrue(TEXT("Min dwell allows a later exit"), FFBTactical::CanExitSoft(0.f, Tuning.MinDwellSeconds, Tuning.MinDwellSeconds));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiCueTest,
	"FinalBell.AI.Cues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiCueTest::RunTest(const FString& Parameters)
{
	FFBTacticalBrain Brain;
	Arm(Brain);
	Brain.SetDifficulty(EFBAIDifficulty::Novice);
	Brain.NoteAttackCue(4, 9, EFBAttackId::Jab, false, false, 0.f, 0.9f, 140.f);
	Brain.NoteAttackCue(4, 9, EFBAttackId::Jab, false, false, 0.f, 0.9f, 140.f);
	TestEqual(TEXT("The same attack is stored once"), Brain.CueCount(), 1);

	FFBTacticalDecision TooSoon = Brain.Decide(LiveView(0.10f, 150.f, 80.f, 100.f));
	TestNotEqual(TEXT("A cue inside the recognition delay is not a threat yet"), TooSoon.State, EFBTacticalState::Defend);

	FFBTacticalDecision Ready = Brain.Decide(LiveView(0.20f, 150.f, 80.f, 100.f));
	TestEqual(TEXT("A recognized startup is answered"), Ready.State, EFBTacticalState::Defend);

	FFBTacticalDecision Stale = Brain.Decide(LiveView(0.80f, 150.f, 80.f, 100.f));
	TestNotEqual(TEXT("A stale cue is dropped"), Stale.State, EFBTacticalState::Defend);
	TestEqual(TEXT("Expiry removes the cue"), Brain.CueCount(), 0);

	Brain.NoteAttackCue(4, 11, EFBAttackId::Cross, false, false, 1.f, -0.4f, 80.f);
	TestEqual(TEXT("A punch behind the shoulder is not stored"), Brain.CueCount(), 0);

	const float Outside = FFBTactical::PunchContactSuitability(
		EFBAttackId::Jab, 180.f, 1.f, 0.f, 60.f, 140.f, 0.08f, false);
	TestTrue(TEXT("Preferred range does not make an out-of-reach jab legal"), Outside <= 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiGuardRegenTest,
	"FinalBell.AI.GuardRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiGuardRegenTest::RunTest(const FString& Parameters)
{
	FFBCombatIntent Punch;
	Punch.Attack = EFBAttackId::Jab;
	Punch.bGuardHeld = false;
	TestFalse(TEXT("A punch does not release the guard"), UFBCombatComponent::IntentUpdatesGuard(Punch));

	FFBCombatIntent Release;
	Release.Attack = EFBAttackId::None;
	Release.bGuardHeld = false;
	Release.Guard = EFBGuardState::Open;
	TestTrue(TEXT("A no-attack intent can release the guard"), UFBCombatComponent::IntentUpdatesGuard(Release));

	TestFalse(TEXT("High guard does not regen"), UFBCombatComponent::EnergyRegenEligible(EFBAttackPhase::Idle, EFBGuardState::High, 0.f));
	TestFalse(TEXT("The punch delay blocks regen"), UFBCombatComponent::EnergyRegenEligible(EFBAttackPhase::Idle, EFBGuardState::Open, 0.2f));
	TestTrue(TEXT("Idle, open, and off delay can regen"), UFBCombatComponent::EnergyRegenEligible(EFBAttackPhase::Idle, EFBGuardState::Open, 0.f));

	const float Held = UFBCombatComponent::AdvanceRecoverableEnergy(10.f, 40.f, 12.f, 1.f, false);
	TestTrue(TEXT("Ineligible time adds no energy"), FMath::IsNearlyEqual(Held, 10.f, 0.001f));
	const float Open = UFBCombatComponent::AdvanceRecoverableEnergy(10.f, 40.f, 12.f, 1.f, true);
	TestTrue(TEXT("An eligible second refills under the ceiling"), FMath::IsNearlyEqual(Open, 22.f, 0.001f));
	const float Capped = UFBCombatComponent::AdvanceRecoverableEnergy(38.f, 40.f, 12.f, 1.f, true);
	TestTrue(TEXT("Regen stops at the fatigue ceiling"), FMath::IsNearlyEqual(Capped, 40.f, 0.001f));

	FFBTacticalBrain Brain;
	Arm(Brain);
	const FFBTacticalDecision Safe = Brain.Decide(LiveView(0.2f, 260.f, 10.f, 100.f));
	TestEqual(TEXT("Low gas at a safe distance recovers"), Safe.State, EFBTacticalState::Recover);
	TestEqual(TEXT("Recovery releases the guard"), Safe.Action.Kind, EFBAIActionKind::GuardRelease);
	TestFalse(TEXT("The release is not a punch"), Safe.Action.bGuardHeld);
	TestEqual(TEXT("The release carries no attack"), Safe.Action.Attack, EFBAttackId::None);
	TestTrue(TEXT("The release is submitted"), Safe.Action.bSubmit);

	FFBTacticalView Close = LiveView(0.2f, 110.f, 10.f, 100.f);
	Close.OwnGuard = EFBGuardState::High;
	const FFBTacticalDecision Danger = Brain.Decide(Close);
	TestEqual(TEXT("Close range keeps the guard while gassed"), Danger.Action.Kind, EFBAIActionKind::GuardHigh);
	TestTrue(TEXT("The close guard is held"), Danger.Action.bGuardHeld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiExchangeTest,
	"FinalBell.AI.ExchangeAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiExchangeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The recovery tail can take one punch"), UFBCombatComponent::IsComboBufferWindow(EFBAttackPhase::Recovery, 0.10f, 0.125f, false));
	TestFalse(TEXT("Earlier recovery cannot buffer"), UFBCombatComponent::IsComboBufferWindow(EFBAttackPhase::Recovery, 0.20f, 0.125f, false));
	TestFalse(TEXT("A filled slot cannot buffer again"), UFBCombatComponent::IsComboBufferWindow(EFBAttackPhase::Recovery, 0.10f, 0.125f, true));
	TestFalse(TEXT("Idle is not the buffer"), UFBCombatComponent::IsComboBufferWindow(EFBAttackPhase::Idle, 0.05f, 0.125f, false));
	TestFalse(TEXT("A finished recovery is not the buffer"), UFBCombatComponent::IsComboBufferWindow(EFBAttackPhase::Recovery, 0.f, 0.125f, false));

	FFBTacticalBrain Brain;
	Arm(Brain);
	FFBExchangeBeat Jab;
	Jab.Attack = EFBAttackId::Jab;
	FFBExchangeBeat Cross;
	Cross.Attack = EFBAttackId::Cross;
	Brain.DebugSetPlan({ Jab, Cross }, 0.f);
	Brain.NoteIntent(EFBIntentResult::RejectedEnergy, EFBAIActionKind::Jab);
	TestEqual(TEXT("A reject does not advance the plan"), Brain.PlanIndex(), 0);
	TestTrue(TEXT("The plan stays up after one reject"), Brain.HasPlan());
	Brain.NoteIntent(EFBIntentResult::RejectedPhase, EFBAIActionKind::Jab);
	TestFalse(TEXT("A second reject drops the plan instead of spamming"), Brain.HasPlan());

	Brain.DebugSetPlan({ Jab, Cross }, 1.f);
	Brain.NoteIntent(EFBIntentResult::Buffered, EFBAIActionKind::Jab);
	TestEqual(TEXT("A buffered punch counts"), Brain.PlanIndex(), 1);
	Brain.NoteIntent(EFBIntentResult::Accepted, EFBAIActionKind::Cross);
	TestFalse(TEXT("The last accepted punch ends the plan"), Brain.HasPlan());

	Brain.NoteAttackCue(2, 5, EFBAttackId::Jab, false, false, 2.f, 0.8f, 120.f);
	TestEqual(TEXT("A cue is stored before cleanup"), Brain.CueCount(), 1);
	Brain.Reset(TEXT("Rematch"), 3.f);
	TestFalse(TEXT("Reset drops the plan"), Brain.HasPlan());
	TestEqual(TEXT("Reset drops cues"), Brain.CueCount(), 0);
	TestEqual(TEXT("Reset returns to Measure"), Brain.GetState(), EFBTacticalState::Measure);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBAiDifficultyTest,
	"FinalBell.AI.Difficulty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBAiDifficultyTest::RunTest(const FString& Parameters)
{
	const FFBPhysicalCombatValues Before;
	const FFBAIDifficultyProfile Novice = FFBTactical::ProfileFor(EFBAIDifficulty::Novice);
	const FFBAIDifficultyProfile Expert = FFBTactical::ProfileFor(EFBAIDifficulty::Expert);
	const FFBAIDifficultyProfile Club = FFBTactical::ProfileFor(EFBAIDifficulty::Club);
	const FFBPhysicalCombatValues After;
	TestTrue(TEXT("Novice sees the punch later"), Novice.RecognitionDelay > Expert.RecognitionDelay);
	TestTrue(TEXT("Expert still misses some reads"), Expert.ErrorRate > 0.f && Expert.FatigueUncertainty > 0.f);
	TestTrue(TEXT("Club is between them"), Club.ErrorRate > Expert.ErrorRate && Club.ErrorRate < Novice.ErrorRate);
	TestEqual(TEXT("Startup is untouched"), Before.StartupSeconds, After.StartupSeconds);
	TestEqual(TEXT("Jab cost is untouched"), Before.JabCost, After.JabCost);
	TestEqual(TEXT("Jab damage is untouched"), Before.JabDamage, After.JabDamage);
	TestEqual(TEXT("The tank is untouched"), Before.MaxBurstEnergy, After.MaxBurstEnergy);
	TestEqual(TEXT("Walk speed is untouched"), Before.WalkSpeed, After.WalkSpeed);
	TestTrue(TEXT("Novice cannot plan a longer combo than Expert"), Novice.MaxExchangePunches < Expert.MaxExchangePunches);

	const TArray<float> Scores = { 0.82f, 0.48f };
	int32 Mismatches = 0;
	for (int32 Seed = 1; Seed <= 40; ++Seed)
	{
		FRandomStream NoviceStream(Seed);
		FRandomStream ExpertStream(Seed);
		const int32 NovicePick = FFBTactical::PickWithNoise(Scores, NoviceStream, Novice.ScoreNoise, Novice.ErrorRate);
		const int32 ExpertPick = FFBTactical::PickWithNoise(Scores, ExpertStream, Expert.ScoreNoise, Expert.ErrorRate);
		if (NovicePick != ExpertPick)
		{
			++Mismatches;
		}
	}
	TestTrue(TEXT("The same spot does not produce the same calls"), Mismatches > 0);

	TestEqual(TEXT("A bad difficulty value becomes Club"), FFBTactical::SanitizeDifficulty(static_cast<EFBAIDifficulty>(9)), EFBAIDifficulty::Club);
	return true;
}

#endif
