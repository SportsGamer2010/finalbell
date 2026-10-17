// Final Bell — private challenge/room foundation (FB-ENH-06 / P1)
// Session transport: use UE Online Sessions / null OSS locally until EOS/Steam is configured.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FBPrivateRoomSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFBPrivateRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Network")
	FString RoomCode;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Network")
	FString HostAddress;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Network")
	int32 MaxPlayers = 2;

	UPROPERTY(BlueprintReadOnly, Category = "FinalBell|Network")
	bool bRanked = false;
};

/**
 * Local/dev private room helper. Does not claim public matchmaking or ranked.
 * Listen-server join via travel URL; dedicated + auth tokens are a P4 gate.
 */
UCLASS()
class FINALBELL_API UFBPrivateRoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Network")
	FFBPrivateRoomInfo CreatePrivateRoom();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Network")
	bool JoinPrivateRoom(const FString& RoomCode);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Network")
	FFBPrivateRoomInfo GetActiveRoom() const { return ActiveRoom; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Network")
	FString GetSessionTransportNotes() const;

protected:
	UPROPERTY()
	FFBPrivateRoomInfo ActiveRoom;

	FString GenerateRoomCode() const;
};
