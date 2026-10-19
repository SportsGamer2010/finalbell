#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FBMenuGameMode.generated.h"

UCLASS()
class FINALBELL_API AFBMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFBMenuGameMode();

	virtual void BeginPlay() override;
};
