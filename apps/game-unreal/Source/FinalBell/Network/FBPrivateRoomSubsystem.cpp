#include "Network/FBPrivateRoomSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Session/FBSessionSubsystem.h"
#include "UI/FBGameInstance.h"
#include "HAL/PlatformApplicationMisc.h"

namespace
{
	bool IsTailscaleAddress(const FString& Host)
	{
		TArray<FString> Parts;
		Host.ParseIntoArray(Parts, TEXT("."), true);
		if (Parts.Num() != 4 || Parts[0] != TEXT("100"))
		{
			return false;
		}
		const int32 Second = FCString::Atoi(*Parts[1]);
		return Second >= 64 && Second <= 127;
	}

	void PreparePrivateTravel(UGameInstance* GI)
	{
		if (!GI)
		{
			return;
		}
		if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GI))
		{
			FrontEnd->KeepMatchWorld();
			FrontEnd->SetSeatMode(EFBMatchSeatMode::Online);
			FrontEnd->ClearPendingBout();
			FrontEnd->SetOpenFighterSelect(true);
			FrontEnd->SetCareerFighterSelect(false);
		}
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ClearPlayMode();
		}
	}
}

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

	TArray<TSharedPtr<FInternetAddr>> Addresses;
	FString Fallback;
	if (Sockets->GetLocalAdapterAddresses(Addresses))
	{
		for (const TSharedPtr<FInternetAddr>& Addr : Addresses)
		{
			if (!Addr.IsValid() || !Addr->IsValid())
			{
				continue;
			}
			const FString Host = Addr->ToString(false);
			if (Host.IsEmpty() || Host == TEXT("127.0.0.1") || Host == TEXT("0.0.0.0") || Host.Contains(TEXT(":")))
			{
				continue;
			}
			if (IsTailscaleAddress(Host))
			{
				return Host;
			}
			if (Fallback.IsEmpty())
			{
				Fallback = Host;
			}
		}
	}
	if (!Fallback.IsEmpty())
	{
		return Fallback;
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
			FString::Printf(TEXT("Share %s — the other player pastes that address."), *GetShareToken()));
	}

	return ActiveRoom;
}

FString UFBPrivateRoomSubsystem::GetShareToken() const
{
	const FString Host = ActiveRoom.HostAddress.IsEmpty() ? GetPreferredHostAddress() : ActiveRoom.HostAddress;
	const int32 Port = ActiveRoom.Port > 0 ? ActiveRoom.Port : 7777;
	return FString::Printf(TEXT("%s:%d"), *Host, Port);
}

bool UFBPrivateRoomSubsystem::IsEosSignedIn() const
{
	IOnlineSubsystem* EOS = IOnlineSubsystem::Get(TEXT("EOS"));
	if (!EOS)
	{
		return false;
	}
	const IOnlineIdentityPtr Identity = EOS->GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

bool UFBPrivateRoomSubsystem::TryJoinToken(const FString& Token, FString& OutError)
{
	const FString Trimmed = Token.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		OutError = TEXT("Enter the host address, or paste it.");
		return false;
	}
	const bool bLooksLikeAddress = Trimmed.Contains(TEXT(".")) || Trimmed.Contains(TEXT(":"));
	if (!bLooksLikeAddress)
	{
		if (!IsEosSignedIn())
		{
			OutError = TEXT("A short code needs EOS, and this build is not signed in. Paste the host address instead.");
		}
		else
		{
			OutError = TEXT("EOS is signed in, but this build joins by host address. Paste host:port.");
		}
		return false;
	}
	if (!JoinByAddress(Trimmed, 7777))
	{
		OutError = TEXT("That address could not be used. Use host:port, for example 100.64.0.2:7777.");
		return false;
	}
	OutError.Reset();
	return true;
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

	PreparePrivateTravel(GetGameInstance());
	CreatePrivateRoom();
	ActiveRoom.Port = Port > 0 ? Port : 7777;
	FPlatformApplicationMisc::ClipboardCopy(*GetShareToken());

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
	PreparePrivateTravel(GetGameInstance());

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


