// Final Bell — replicated bout clock, knockdown count, and the official card.
// The game mode lives only on the server. This state is what every screen reads.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBFighterBalance.h"
#include "FBSparringGameState.generated.h"

class AFBBoxerCharacter;

UCLASS()
class FINALBELL_API AFBSparringGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFBSparringGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seconds left on the round clock, or on the minute between rounds. The count pauses this. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	float BoutSecondsRemaining = 180.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 CurrentRound = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	int32 ScheduledRounds = 3;

	/** True during the minute in the corner. Punches are refused. Health and stamina have already been restored. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	bool bBetweenRounds = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	bool bRoundRunning = false;

	/** True while a fighter is on the canvas and the referee is counting. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	bool bKnockdown = false;

	/** Seconds left before the count of 10 expires. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	float CountRemaining = 0.f;

	/** Where the left circle's mark sits, in degrees. The server turns it. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	float LeftTargetDegrees = 0.f;

	/** Where the right circle's mark sits, in degrees. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	float RightTargetDegrees = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	TObjectPtr<AFBBoxerCharacter> DownedBoxer = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	bool bCardVisible = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	FFBOfficialCard OfficialCard;

	/** Short referee line: BREAK, WARNING, POINT DEDUCTION. Empty when he is quiet. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	FString RefereeCall;

	/** Bump when the lobby record layout changes. A guest with a different number is refused. */
	static constexpr int32 LobbyProtocol = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	int32 LobbyProtocolSeen = LobbyProtocol;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	FFBFighterRecord BluePick;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	FFBFighterRecord RedPick;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	bool bBlueReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	bool bRedReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Lobby")
	FString LobbyNotice;
};
