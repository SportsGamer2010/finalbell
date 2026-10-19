// Final Bell — replicated bout clock, knockdown count, and the official card.
// The game mode lives only on the server. This state is what every screen reads.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Combat/FBCombatTypes.h"
#include "FBSparringGameState.generated.h"

class AFBBoxerCharacter;

UCLASS()
class FINALBELL_API AFBSparringGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFBSparringGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seconds left on the 3:00 bout clock. The count pauses this. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Bout")
	float BoutSecondsRemaining = 180.f;

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
};
