#include "Network/FBPrivateRoomSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

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

FString UFBPrivateRoomSubsystem::GetPreferredHostAddress() const
{
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return TEXT("127.0.0.1");
	}

	bool bCanBindAll = false;
	TSharedRef<FInternetAddr> Addr = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
	if (Addr->IsValid())
	{
		const FString Host = Addr->ToString(false);
		if (!Host.IsEmpty() && Host != TEXT("0.0.0.0"))
		{
			return Host;
		}
	}
	return TEXT("127.0.0.1");
}

FString UFBPrivateRoomSubsystem::NormalizeHostPort(const FString& HostAddress, int32 Port, FString& OutHost, int32& OutPort)
{
	OutPort = Port > 0 ? Port : 7777;
	OutHost = HostAddress.TrimStartAndEnd();

	int32 ColonIdx = INDEX_NONE;
	if (OutHost.FindLastChar(TEXT(':'), ColonIdx) && ColonIdx > 0)
	{
		const FString PortStr = OutHost.Mid(ColonIdx + 1);
		if (PortStr.IsNumeric())
		{
			OutPort = FCString::Atoi(*PortStr);
			OutHost = OutHost.Left(ColonIdx);
		}
	}

	OutHost.ReplaceInline(TEXT(" "), TEXT(""));
	return FString::Printf(TEXT("%s:%d"), *OutHost, OutPort);
}

FFBPrivateRoomInfo UFBPrivateRoomSubsystem::CreatePrivateRoom()
{
	ActiveRoom = FFBPrivateRoomInfo();
	ActiveRoom.RoomCode = GenerateRoomCode();
	ActiveRoom.HostAddress = GetPreferredHostAddress();
	ActiveRoom.Port = 7777;
	ActiveRoom.MaxPlayers = 2;
	ActiveRoom.bRanked = false;

	UE_LOG(LogTemp, Log,
		TEXT("FinalBell private room %s host=%s:%d (listen-server foundation; not a public service)."),
		*ActiveRoom.RoomCode, *ActiveRoom.HostAddress, ActiveRoom.Port);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 12.f, FColor::Green,
			FString::Printf(TEXT("Room %s — share %s:%d (Tailscale/LAN). Partner: FBJoin %s"),
				*ActiveRoom.RoomCode, *ActiveRoom.HostAddress, ActiveRoom.Port, *ActiveRoom.HostAddress));
	}

	return ActiveRoom;
}

void UFBPrivateRoomSubsystem::SetHostAddress(const FString& Address)
{
	ActiveRoom.HostAddress = Address.TrimStartAndEnd();
}

bool UFBPrivateRoomSubsystem::HostListenServer(int32 Port)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CreatePrivateRoom();
	ActiveRoom.Port = Port > 0 ? Port : 7777;

	const FString TravelURL = TEXT("/Game/Maps/Ring_P1?listen");
	UE_LOG(LogTemp, Warning, TEXT("FinalBell HostListenServer travel: %s (tell partner FBJoin %s port %d)"),
		*TravelURL, *ActiveRoom.HostAddress, ActiveRoom.Port);

	World->ServerTravel(TravelURL, true);
	return true;
}

bool UFBPrivateRoomSubsystem::JoinByAddress(const FString& HostAddress, int32 Port)
{
	if (HostAddress.IsEmpty())
	{
		return false;
	}

	FString Host;
	int32 ResolvedPort = Port;
	const FString URL = NormalizeHostPort(HostAddress, Port, Host, ResolvedPort);

	ActiveRoom.HostAddress = Host;
	ActiveRoom.Port = ResolvedPort;
	ActiveRoom.bRanked = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell JoinByAddress ClientTravel %s"), *URL);
		PC->ClientTravel(URL, TRAVEL_Absolute);
		return true;
	}
	return false;
}

bool UFBPrivateRoomSubsystem::JoinPrivateRoom(const FString& RoomCode)
{
	if (RoomCode.IsEmpty())
	{
		return false;
	}

	ActiveRoom.RoomCode = RoomCode.ToUpper();
	ActiveRoom.bRanked = false;

	if (ActiveRoom.HostAddress.IsEmpty())
	{
		ActiveRoom.HostAddress = TEXT("127.0.0.1");
	}
	if (ActiveRoom.Port <= 0)
	{
		ActiveRoom.Port = 7777;
	}

	return JoinByAddress(ActiveRoom.HostAddress, ActiveRoom.Port);
}

FString UFBPrivateRoomSubsystem::GetSessionTransportNotes() const
{
	return TEXT(
		"P1 transport: UE native IpNetDriver + listen-server. "
		"WAN: Tailscale/ZeroTier or LAN; partner joins host:7777. "
		"Online Subsystem: Null for LAN/dev. EOS/Steam WAN+relay require separate config (P4). "
		"Private rooms are unranked. Ranked pool is a later FB-ENH-06 gate.");
}


