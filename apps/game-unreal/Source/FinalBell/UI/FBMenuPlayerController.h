#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FBMenuPlayerController.generated.h"

class UFBMenuWidget;
class AFBMenuCamera;
struct FInputKeyEventArgs;

UCLASS()
class FINALBELL_API AFBMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFBMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

private:
	UPROPERTY()
	TObjectPtr<UFBMenuWidget> MenuWidget;

	UPROPERTY()
	TObjectPtr<AFBMenuCamera> MenuCamera;

	void FocusMenuUser();

	float StickRepeat = 0.f;
};
