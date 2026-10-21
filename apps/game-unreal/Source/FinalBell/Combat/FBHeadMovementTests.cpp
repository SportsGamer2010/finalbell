#include "Misc/AutomationTest.h"
#include "Combat/FBHeadMovement.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBHeadNeutralTest,
	"FinalBell.Defense.NeutralIsNotADuck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBHeadNeutralTest::RunTest(const FString& Parameters)
{
	const FFBHeadMovementLimits Limits;
	const FVector2D Center = FVector2D::ZeroVector;
	TestEqual(TEXT("Centered stick is neutral"), FFBHeadMovement::Classify(Center, false), EFBHeadMoveKind::Neutral);
	TestFalse(TEXT("Neutral is not a completed slip"), FFBHeadMovement::CountsAsCompletedEvasion(EFBHeadMoveKind::Neutral, Center, Limits.CompletedAxis));

	const FFBHeadMovement::FFBHeadPose Pose = FFBHeadMovement::PoseFromAxes(Center, EFBHeadMoveKind::Neutral, 0.f, Limits);
	TestTrue(TEXT("Neutral pitch is the small offset"), FMath::IsNearlyEqual(Pose.Pitch, Limits.NeutralPitch, 0.01f));
	TestTrue(TEXT("Neutral has no roll"), FMath::IsNearlyZero(Pose.Roll, 0.01f));
	TestTrue(TEXT("Neutral does not drop the knees"), FMath::IsNearlyZero(Pose.KneeDropUU, 0.01f));
	TestTrue(TEXT("Neutral is not the full slip"), Pose.Pitch < Limits.SlipRoll);
	TestTrue(TEXT("Neutral is far under the old 74 degree duck"), Pose.Pitch < 20.f);

	float Sign = 1.f;
	TArray<FFBHeadStickSample> Still;
	Still.Add({ 0.f, Center });
	Still.Add({ 0.1f, FVector2D(0.05f, 0.05f) });
	TestFalse(TEXT("A centered stick is not a roll-under arc"), FFBHeadMovement::DetectRollUnderArc(Still, 0.2f, Sign));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBHeadAxisRoutingTest,
	"FinalBell.Defense.SignedAxes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBHeadAxisRoutingTest::RunTest(const FString& Parameters)
{
	const FFBHeadMovementLimits Limits;
	TestEqual(TEXT("Axes clamp"), FFBHeadMovement::ClampAxes(FVector2D(2.f, -3.f)), FVector2D(1.f, -1.f));

	const FVector2D Right(1.f, 0.f);
	const FVector2D Left(-1.f, 0.f);
	const FVector2D Back(0.f, -1.f);
	const FVector2D Forward(0.f, 1.f);
	TestEqual(TEXT("Right is a slip"), FFBHeadMovement::Classify(Right, false), EFBHeadMoveKind::Slip);
	TestEqual(TEXT("Left is a slip"), FFBHeadMovement::Classify(Left, false), EFBHeadMoveKind::Slip);
	TestEqual(TEXT("Back is a lean"), FFBHeadMovement::Classify(Back, false), EFBHeadMoveKind::LeanBack);
	TestEqual(TEXT("Forward is a level change"), FFBHeadMovement::Classify(Forward, false), EFBHeadMoveKind::LevelChange);

	const FFBHeadMovement::FFBHeadPose SlipRight = FFBHeadMovement::PoseFromAxes(Right, EFBHeadMoveKind::Slip, 0.f, Limits);
	const FFBHeadMovement::FFBHeadPose SlipLeft = FFBHeadMovement::PoseFromAxes(Left, EFBHeadMoveKind::Slip, 0.f, Limits);
	TestTrue(TEXT("Stick right rolls the head to the fighter's left"), SlipRight.Roll < -20.f && SlipRight.Roll > -30.f);
	TestTrue(TEXT("Stick left is the opposite roll"), SlipLeft.Roll > 20.f && SlipLeft.Roll < 30.f);

	const FFBHeadMovement::FFBHeadPose Lean = FFBHeadMovement::PoseFromAxes(Back, EFBHeadMoveKind::LeanBack, 0.f, Limits);
	TestTrue(TEXT("Full lean is the configured pitch"), FMath::IsNearlyEqual(Lean.Pitch, Limits.LeanBackPitch, 0.01f));
	TestTrue(TEXT("Lean is not the old 58 degree layback"), Lean.Pitch > -30.f);

	const FFBHeadMovement::FFBHeadPose Level = FFBHeadMovement::PoseFromAxes(Forward, EFBHeadMoveKind::LevelChange, 0.f, Limits);
	TestTrue(TEXT("Forward uses the knee drop"), FMath::IsNearlyEqual(Level.KneeDropUU, Limits.LevelKneeDropUU, 0.01f));
	TestTrue(TEXT("Forward trunk pitch stays small"), Level.Pitch < 16.f);
	TestFalse(TEXT("A level change is not a completed head evasion"), FFBHeadMovement::CountsAsCompletedEvasion(EFBHeadMoveKind::LevelChange, Forward, Limits.CompletedAxis));

	const FVector2D Short(0.30f, 0.f);
	TestEqual(TEXT("A short stick still grades as a slip"), FFBHeadMovement::Classify(Short, false), EFBHeadMoveKind::Slip);
	TestFalse(TEXT("A short stick is not a completed slip"), FFBHeadMovement::CountsAsCompletedEvasion(EFBHeadMoveKind::Slip, Short, Limits.CompletedAxis));
	const FFBHeadMovement::FFBHeadPose ShortPose = FFBHeadMovement::PoseFromAxes(Short, EFBHeadMoveKind::Slip, 0.f, Limits);
	TestTrue(TEXT("A short stick is a fraction of the full roll"), FMath::Abs(ShortPose.Roll) > 5.f && FMath::Abs(ShortPose.Roll) < 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBHeadReleaseTest,
	"FinalBell.Defense.ReleaseResets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBHeadReleaseTest::RunTest(const FString& Parameters)
{
	const FFBHeadMovementLimits Limits;
	const FFBHeadMovement::FFBHeadPose Released = FFBHeadMovement::PoseFromAxes(FVector2D(1.f, -1.f), EFBHeadMoveKind::Inactive, 1.f, Limits);
	TestTrue(TEXT("Release pitch is zero"), FMath::IsNearlyZero(Released.Pitch, 0.01f));
	TestTrue(TEXT("Release roll is zero"), FMath::IsNearlyZero(Released.Roll, 0.01f));
	TestTrue(TEXT("Release knee drop is zero"), FMath::IsNearlyZero(Released.KneeDropUU, 0.01f));
	TestFalse(TEXT("Inactive is not evasion"), FFBHeadMovement::CountsAsCompletedEvasion(EFBHeadMoveKind::Inactive, FVector2D(1.f, 0.f), Limits.CompletedAxis));

	const FFBHeadMovement::FFBHeadPose Explicit = FFBHeadMovement::ExplicitSlipPose(FVector2D::ZeroVector, -1.f, Limits);
	TestTrue(TEXT("A centered explicit slip does not pose"), FMath::IsNearlyZero(Explicit.Pitch, 0.01f) && FMath::IsNearlyZero(Explicit.Roll, 0.01f));

	const bool bOnTime = FFBHeadMovement::EvasionOnTime(
		1.f, -1.f, -1.f, -1.f, EFBHeadMoveKind::Neutral, FVector2D::ZeroVector, Limits.CompletedAxis, 0.5f);
	TestFalse(TEXT("A released neutral hold does not beat a punch"), bOnTime);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBHeadGuardCoexistTest,
	"FinalBell.Defense.GuardPlusLean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBHeadGuardCoexistTest::RunTest(const FString& Parameters)
{
	const FFBHeadMovementLimits Limits;
	TestEqual(
		TEXT("R2 plus stick down is the low guard"),
		FFBHeadMovement::SelectBlockGuard(true, false, false, -0.80f, Limits.LowGuardStickY),
		EFBGuardState::Low);
	TestEqual(
		TEXT("R2 with the stick up is a high guard"),
		FFBHeadMovement::SelectBlockGuard(true, false, false, 0.20f, Limits.LowGuardStickY),
		EFBGuardState::High);
	TestEqual(
		TEXT("Stick back without R2 is not a guard"),
		FFBHeadMovement::SelectBlockGuard(false, false, false, -1.f, Limits.LowGuardStickY),
		EFBGuardState::Open);
	TestEqual(
		TEXT("L2 keeps a latched low guard"),
		FFBHeadMovement::SelectBlockGuard(true, true, true, 0.9f, Limits.LowGuardStickY),
		EFBGuardState::Low);
	TestEqual(
		TEXT("L2 does not turn stick down into a new low guard"),
		FFBHeadMovement::SelectBlockGuard(true, true, false, -1.f, Limits.LowGuardStickY),
		EFBGuardState::High);

	const FFBHeadMovement::FFBHeadPose Lean = FFBHeadMovement::PoseFromAxes(FVector2D(0.f, -1.f), EFBHeadMoveKind::LeanBack, 0.f, Limits);
	TestTrue(TEXT("The lean still has its own pitch while the guard stays high"), Lean.Pitch < -10.f);
	TestTrue(TEXT("L2 consumes a legal stick"), FFBHeadMovement::ConsumesLocomotionStick(true, false, false));
	TestFalse(TEXT("L2 does not consume the stick in a clinch"), FFBHeadMovement::ConsumesLocomotionStick(true, false, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFBHeadLegalAndPresentationTest,
	"FinalBell.Defense.LegalStateAndPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBHeadLegalAndPresentationTest::RunTest(const FString& Parameters)
{
	const FFBHeadMovementLimits Limits;
	TestFalse(TEXT("Knockdown blocks head movement"), FFBHeadMovement::StateAllows(true, false));
	TestFalse(TEXT("Clinch blocks head movement"), FFBHeadMovement::StateAllows(false, true));
	TestTrue(TEXT("A standing open fighter may move the head"), FFBHeadMovement::StateAllows(false, false));

	const FVector2D FullSlip(-0.80f, 0.10f);
	TestTrue(TEXT("A full lateral is a completed slip"), FFBHeadMovement::CountsAsCompletedEvasion(EFBHeadMoveKind::Slip, FullSlip, Limits.CompletedAxis));
	TestTrue(
		TEXT("A slip that started before the glove window counts"),
		FFBHeadMovement::EvasionOnTime(1.2f, -1.f, -1.f, 0.4f, EFBHeadMoveKind::Slip, FullSlip, Limits.CompletedAxis, 0.8f));
	TestFalse(
		TEXT("A slip that starts after the window is late"),
		FFBHeadMovement::EvasionOnTime(1.2f, -1.f, -1.f, 1.0f, EFBHeadMoveKind::Slip, FullSlip, Limits.CompletedAxis, 0.8f));
	TestFalse(
		TEXT("Holding the mode at center does not evade"),
		FFBHeadMovement::EvasionOnTime(1.2f, -1.f, -1.f, 0.2f, EFBHeadMoveKind::Neutral, FVector2D::ZeroVector, Limits.CompletedAxis, 0.8f));

	const FVector2D OwnerStick(0.f, -0.8f);
	const FVector2D Replicated(-0.9f, 0.f);
	TestEqual(TEXT("The owner presents the local stick"), FFBHeadMovement::PresentationAxes(true, OwnerStick, FVector2D::ZeroVector), OwnerStick);
	TestEqual(TEXT("A remote presents the replicated lean, not velocity"), FFBHeadMovement::PresentationAxes(false, FVector2D::ZeroVector, Replicated), Replicated);

	TArray<FFBHeadStickSample> Arc;
	Arc.Add({ 0.00f, FVector2D(-0.85f, 0.20f) });
	Arc.Add({ 0.12f, FVector2D(0.05f, 0.90f) });
	Arc.Add({ 0.24f, FVector2D(0.85f, 0.20f) });
	float Sign = 0.f;
	TestTrue(TEXT("A side-forward-side sweep is a roll under"), FFBHeadMovement::DetectRollUnderArc(Arc, 0.30f, Sign));
	TestTrue(TEXT("The sweep exits to the right"), FMath::IsNearlyEqual(Sign, 1.f, 0.01f));

	TArray<FFBHeadStickSample> HoldForward;
	HoldForward.Add({ 0.f, FVector2D(0.f, 1.f) });
	HoldForward.Add({ 0.2f, FVector2D(0.1f, 0.9f) });
	TestFalse(TEXT("Holding forward is not a roll under"), FFBHeadMovement::DetectRollUnderArc(HoldForward, 0.3f, Sign));

	const FFBHeadMovement::FFBHeadPose ExplicitRoll = FFBHeadMovement::ExplicitSlipPose(FVector2D(1.f, 0.f), -1.f, Limits);
	TestTrue(TEXT("An explicit slip uses the same roll cap"), FMath::Abs(ExplicitRoll.Roll) <= Limits.SlipRoll + 0.01f);
	TestTrue(TEXT("An explicit slip is not the old 48 degree trunk roll"), FMath::Abs(ExplicitRoll.Roll) < 30.f);
	return true;
}

#endif
