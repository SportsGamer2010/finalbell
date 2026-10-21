#include "Misc/AutomationTest.h"
#include "Combat/FBSelectNav.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFBSelectNavContainmentTest, "FinalBell.Select.Containment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFBSelectNavContainmentTest::RunTest(const FString& Parameters)
{
	const TArray<int32> Sides = { 0, 0, 0, 1, 1, 2 };
	TestEqual(TEXT("Down from the last left row wraps inside the left panel"), FFBSelectNav::Step(Sides, 2, 1, 0), 0);
	TestEqual(TEXT("Up from the first left row wraps inside the left panel"), FFBSelectNav::Step(Sides, 0, -1, 0), 2);
	TestEqual(TEXT("Down on the right panel never enters the action bar"), FFBSelectNav::Step(Sides, 4, 1, 1), 3);
	TestEqual(TEXT("A focus that sits on the other panel snaps into the requested panel"), FFBSelectNav::Step(Sides, 0, 1, 1), 3);
	TestEqual(TEXT("An empty panel does not move focus"), FFBSelectNav::Step(Sides, 1, 1, 9), 1);

	TestEqual(TEXT("VS AI ignores a second controller"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::VersusAI, 0, 1, false, 1), INDEX_NONE);
	TestEqual(TEXT("VS AI stays on the active panel"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::VersusAI, 0, 1, false, 0), 1);
	TestEqual(TEXT("Local player 1 is the left panel"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::LocalVersus, 0, 0, true, 0), 0);
	TestEqual(TEXT("Local player 2 is the right panel after joining"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::LocalVersus, 1, 0, true, 1), 1);
	TestEqual(TEXT("A second controller cannot browse before it joins"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::LocalVersus, 1, 0, false, 1), INDEX_NONE);
	TestEqual(TEXT("A third controller is ignored"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::LocalVersus, 0, 0, true, 2), INDEX_NONE);
	TestEqual(TEXT("An online guest's first controller edits network seat 2"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::Online, 1, 0, false, 0), 1);
	TestEqual(TEXT("An online host's controller edits the left panel"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::Online, 0, 1, false, 0), 0);
	TestEqual(TEXT("Online input waits until the server assigns a seat"), FFBSelectNav::PanelForDevice(EFBMatchSeatMode::Online, -1, 0, false, 0), INDEX_NONE);

	TestTrue(TEXT("Local mouse stays on the left"), FFBSelectNav::AllowsPointer(EFBMatchSeatMode::LocalVersus, 0, 0, 0));
	TestFalse(TEXT("Local mouse cannot edit player 2"), FFBSelectNav::AllowsPointer(EFBMatchSeatMode::LocalVersus, 0, 1, 1));
	TestFalse(TEXT("VS AI pointer stays out of the inactive panel"), FFBSelectNav::AllowsPointer(EFBMatchSeatMode::VersusAI, 0, 0, 1));
	TestTrue(TEXT("VS AI pointer can use the active panel"), FFBSelectNav::AllowsPointer(EFBMatchSeatMode::VersusAI, 0, 1, 1));
	return true;
}

#endif
