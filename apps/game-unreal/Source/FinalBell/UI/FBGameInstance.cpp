#include "UI/FBGameInstance.h"
#include "UI/FBMenuDeveloperSettings.h"
#include "UI/FBPlayerSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"

void UFBGameInstance::Init()
{
	Super::Init();
	if (UFBPlayerSettingsSave* Settings = UFBPlayerSettingsSave::LoadOrCreate())
	{
		Settings->ApplyAudio(this);
	}
}

bool UFBGameInstance::WantsFrontEndBoot() const
{
	if (bFrontEndBootHandled)
	{
		return false;
	}
	const TCHAR* CommandLine = FCommandLine::Get();
	if (FParse::Param(CommandLine, TEXT("SkipMenu")))
	{
		return false;
	}
	if (FParse::Param(CommandLine, TEXT("ShowMenu")))
	{
		return true;
	}
	return !GIsEditor;
}

void UFBGameInstance::OpenFrontEnd(UObject* ContextObject)
{
	if (bFrontEndBootHandled || !ContextObject)
	{
		return;
	}
	bFrontEndBootHandled = true;
	bAwaitingFrontEnd = true;
	TravelToFrontEnd(ContextObject);
}

void UFBGameInstance::TravelToFrontEnd(UObject* ContextObject)
{
	if (!ContextObject)
	{
		return;
	}
	const UFBMenuDeveloperSettings* Style = UFBMenuDeveloperSettings::Get();
	const FString Map = (Style && !Style->MenuMap.IsEmpty()) ? Style->MenuMap : TEXT("/Game/Maps/Ring_P1");
	UGameplayStatics::OpenLevel(
		ContextObject,
		FName(*Map),
		true,
		TEXT("game=/Script/FinalBell.FBMenuGameMode"));
}

void UFBGameInstance::NotifyFrontEndArrived()
{
	bAwaitingFrontEnd = false;
}

void UFBGameInstance::SetSeatMode(EFBMatchSeatMode Mode)
{
	SeatMode = Mode;
}

void UFBGameInstance::SetOpenFighterSelect(bool bOpen)
{
	bOpenFighterSelect = bOpen;
}

bool UFBGameInstance::ConsumeOpenFighterSelect()
{
	const bool bOpen = bOpenFighterSelect;
	bOpenFighterSelect = false;
	return bOpen;
}

void UFBGameInstance::SetCareerFighterSelect(bool bCareer)
{
	bCareerFighterSelect = bCareer;
}

bool UFBGameInstance::ConsumeCareerFighterSelect()
{
	const bool bCareer = bCareerFighterSelect;
	bCareerFighterSelect = false;
	return bCareer;
}

void UFBGameInstance::SetPendingBout(const FFBFighterRecord& PlayerBuild, const FFBFighterRecord& OpponentBuild, EFBAIDifficulty Difficulty)
{
	PendingPlayer = PlayerBuild;
	PendingOpponent = OpponentBuild;
	PendingDifficulty = FFBTactical::SanitizeDifficulty(Difficulty);
	bPendingBout = true;
}

void UFBGameInstance::ClearPendingBout()
{
	PendingPlayer = FFBFighterRecord();
	PendingOpponent = FFBFighterRecord();
	PendingDifficulty = EFBAIDifficulty::Club;
	bPendingBout = false;
}

void UFBGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	Super::OnWorldChanged(OldWorld, NewWorld);
	if (NewWorld)
	{
		if (UFBPlayerSettingsSave* Settings = UFBPlayerSettingsSave::LoadOrCreate())
		{
			Settings->ApplyAudio(NewWorld);
		}
	}
}
