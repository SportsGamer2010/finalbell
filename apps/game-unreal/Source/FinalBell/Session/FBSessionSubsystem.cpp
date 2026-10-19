#include "Session/FBSessionSubsystem.h"
#include "Engine/Engine.h"

void UFBSessionSubsystem::ChoosePlayMode(EFBPlayMode Mode)
{
	if (Mode == EFBPlayMode::Unset)
	{
		return;
	}
	PlayMode = Mode;
	UE_LOG(LogTemp, Warning, TEXT("FinalBell play mode chosen: %d"), (int32)Mode);
	if (GEngine)
	{
		const TCHAR* Label =
			Mode == EFBPlayMode::VersusAI ? TEXT("1v1 Sparring (AI)") :
			Mode == EFBPlayMode::Multiplayer ? TEXT("1v1 Multiplayer") :
			Mode == EFBPlayMode::Training ? TEXT("Heavy Bag Training") :
			Mode == EFBPlayMode::Career ? TEXT("1v1 Career") : TEXT("Unset");
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, FString::Printf(TEXT("Mode: %s"), Label));
	}
	OnPlayModeChosen.Broadcast(PlayMode);
}

void UFBSessionSubsystem::ClearPlayMode()
{
	PlayMode = EFBPlayMode::Unset;
}

void UFBSessionSubsystem::SetReturnToMenuAllowed(bool bAllowed)
{
	bReturnToMenuAllowed = bAllowed;
}

bool UFBSessionSubsystem::TryLockTravel()
{
	if (bTravelLocked)
	{
		return false;
	}
	bTravelLocked = true;
	return true;
}

void UFBSessionSubsystem::UnlockTravel()
{
	bTravelLocked = false;
}
