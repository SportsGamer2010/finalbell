#include "Combat/FBSparringGameState.h"
#include "Net/UnrealNetwork.h"

AFBSparringGameState::AFBSparringGameState()
{
	// The clock and the rotating marks need to reach the other screen often
	// enough that the count and the circles do not jump.
	SetNetUpdateFrequency(20.f);
}

void AFBSparringGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFBSparringGameState, BoutSecondsRemaining);
	DOREPLIFETIME(AFBSparringGameState, CurrentRound);
	DOREPLIFETIME(AFBSparringGameState, ScheduledRounds);
	DOREPLIFETIME(AFBSparringGameState, bBetweenRounds);
	DOREPLIFETIME(AFBSparringGameState, bRoundRunning);
	DOREPLIFETIME(AFBSparringGameState, bKnockdown);
	DOREPLIFETIME(AFBSparringGameState, CountRemaining);
	DOREPLIFETIME(AFBSparringGameState, LeftTargetDegrees);
	DOREPLIFETIME(AFBSparringGameState, RightTargetDegrees);
	DOREPLIFETIME(AFBSparringGameState, DownedBoxer);
	DOREPLIFETIME(AFBSparringGameState, bCardVisible);
	DOREPLIFETIME(AFBSparringGameState, OfficialCard);
	DOREPLIFETIME(AFBSparringGameState, RefereeCall);
	DOREPLIFETIME(AFBSparringGameState, LobbyProtocolSeen);
	DOREPLIFETIME(AFBSparringGameState, BluePick);
	DOREPLIFETIME(AFBSparringGameState, RedPick);
	DOREPLIFETIME(AFBSparringGameState, bBlueReady);
	DOREPLIFETIME(AFBSparringGameState, bRedReady);
	DOREPLIFETIME(AFBSparringGameState, LobbyNotice);
}
