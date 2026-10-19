#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "FBGameViewportClient.generated.h"

/**
 * Multi-PIE gamepad routing: pad N drives PIE window N so two controllers
 * can fight at once without sharing ControllerId 0.
 */
UCLASS()
class FINALBELL_API UFBGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual ULocalPlayer* SetupInitialLocalPlayer(FString& OutError) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputAxis(const FInputKeyEventArgs& EventArgs) override;
	virtual void RemapControllerInput(FInputKeyEventArgs& InOutEventArgs) override;

	void BindPieLocalPlayerToGamepad();

private:
	bool RouteGamepadToMatchingPieViewport(const FInputKeyEventArgs& EventArgs, bool bAxis);
	void ClaimPadsForLocalPlayer();

	bool bPieBindingApplied = false;
};
