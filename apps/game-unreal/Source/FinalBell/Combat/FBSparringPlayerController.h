#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FBSparringPlayerController.generated.h"

UCLASS()
class FINALBELL_API AFBSparringPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFBSparringPlayerController();

	virtual void BeginPlay() override;
};
