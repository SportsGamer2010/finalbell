// Final Bell — selected play mode (AI / MP / Training)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/FBCombatTypes.h"
#include "FBSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFBOnPlayModeChosen, EFBPlayMode, Mode);

UCLASS()
class FINALBELL_API UFBSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Session")
	void ChoosePlayMode(EFBPlayMode Mode);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Session")
	EFBPlayMode GetPlayMode() const { return PlayMode; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Session")
	bool HasChosenPlayMode() const { return PlayMode != EFBPlayMode::Unset; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Session")
	void ClearPlayMode();

	UFUNCTION(BlueprintPure, Category = "FinalBell|Session")
	bool IsReturnToMenuAllowed() const { return bReturnToMenuAllowed; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Session")
	void SetReturnToMenuAllowed(bool bAllowed);

	bool TryLockTravel();
	void UnlockTravel();

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Session")
	FFBOnPlayModeChosen OnPlayModeChosen;

protected:
	UPROPERTY()
	EFBPlayMode PlayMode = EFBPlayMode::Unset;

	bool bReturnToMenuAllowed = false;
	bool bTravelLocked = false;
};
