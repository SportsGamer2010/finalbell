#include "Network/FBPrivateRoomSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

FString UFBPrivateRoomSubsystem::GenerateRoomCode() const
{
	constexpr TCHAR Alphabet[] = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
	FString Code;
	for (int32 i = 0; i < 6; ++i)
	{
		Code.AppendChar(Alphabet[FMath::RandRange(0, UE_ARRAY_COUNT(Alphabet) - 2)]);
	}
	return Code;
}

FFBPrivateRoomInfo UFBPrivateRoomSubsystem::CreatePrivateRoom()
{
	ActiveRoom = FFBPrivateRoomInfo();
	ActiveRoom.RoomCode = GenerateRoomCode();
	ActiveRoom.HostAddress = TEXT("127.0.0.1");
	ActiveRoom.MaxPlayers = 2;
	ActiveRoom.bRanked = false;

	UE_LOG(LogTemp, Log,
		TEXT("FinalBell private room %s created (listen-server foundation; not a public service)."),
		*ActiveRoom.RoomCode);

	return ActiveRoom;
}

bool UFBPrivateRoomSubsystem::JoinPrivateRoom(const FString& RoomCode)
{
	if (RoomCode.IsEmpty())
	{
		return false;
	}

	ActiveRoom.RoomCode = RoomCode.ToUpper();
	ActiveRoom.bRanked = false;

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			// Dev travel: host must already be listening. Replace with Online Session search when OSS configured.
			const FString URL = FString::Printf(TEXT("%s:7777"), *ActiveRoom.HostAddress);
			PC->ClientTravel(URL, TRAVEL_Absolute);
			return true;
		}
	}
	return false;
}

FString UFBPrivateRoomSubsystem::GetSessionTransportNotes() const
{
	return TEXT(
		"P1 transport: UE native net driver + listen-server / local dedicated process. "
		"Online Subsystem: Null for LAN/dev. EOS/Steam WAN+relay require separate config (P4). "
		"Private rooms are unranked. Ranked pool is a later FB-ENH-06 gate.");
}
