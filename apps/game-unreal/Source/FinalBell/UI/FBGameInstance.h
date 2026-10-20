// Packaged games open the front end. Editor PIE stays on the ring unless -ShowMenu is set.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Combat/FBFighterBalance.h"
#include "Combat/FBTacticalTypes.h"
#include "FBGameInstance.generated.h"

UCLASS()
class FINALBELL_API UFBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	bool WantsFrontEndBoot() const;
	bool IsAwaitingFrontEnd() const { return bAwaitingFrontEnd; }
	void OpenFrontEnd(UObject* ContextObject);
	static void TravelToFrontEnd(UObject* ContextObject);
	void NotifyFrontEndArrived();

	void SetOpenFighterSelect(bool bOpen);
	bool ConsumeOpenFighterSelect();
	void SetCareerFighterSelect(bool bCareer);
	bool ConsumeCareerFighterSelect();

	void SetPendingBout(const FFBFighterRecord& PlayerBuild, const FFBFighterRecord& OpponentBuild, EFBAIDifficulty Difficulty = EFBAIDifficulty::Club);
	void ClearPendingBout();
	bool HasPendingBout() const { return bPendingBout; }
	const FFBFighterRecord& GetPendingPlayer() const { return PendingPlayer; }
	const FFBFighterRecord& GetPendingOpponent() const { return PendingOpponent; }
	EFBAIDifficulty GetPendingDifficulty() const { return PendingDifficulty; }

	virtual void OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) override;

private:
	UPROPERTY()
	FFBFighterRecord PendingPlayer;

	UPROPERTY()
	FFBFighterRecord PendingOpponent;

	EFBAIDifficulty PendingDifficulty = EFBAIDifficulty::Club;

	bool bPendingBout = false;
	bool bOpenFighterSelect = false;
	bool bCareerFighterSelect = false;
	bool bFrontEndBootHandled = false;
	bool bAwaitingFrontEnd = false;
};
