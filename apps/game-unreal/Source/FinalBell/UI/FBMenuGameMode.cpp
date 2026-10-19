#include "UI/FBMenuGameMode.h"
#include "UI/FBMenuPlayerController.h"
#include "UI/FBGameInstance.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Session/FBSessionSubsystem.h"
#include "Training/FBHeavyBag.h"
#include "EngineUtils.h"
#include "GameFramework/SpectatorPawn.h"

AFBMenuGameMode::AFBMenuGameMode()
{
	PlayerControllerClass = AFBMenuPlayerController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = true;
}

void AFBMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		FrontEnd->NotifyFrontEndArrived();
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->UnlockTravel();
		}
	}

	// Ring props and overhead lights only. Placeholder fighters stay hidden.
	AFBBoxerCharacter* Player = nullptr;
	AFBBoxerCharacter* Opponent = nullptr;
	AFBBroadcastCamera* BroadcastCamera = nullptr;
	UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, BroadcastCamera, false);

	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		It->SetActorTickEnabled(false);
	}
	for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		It->SetActorTickEnabled(false);
	}
	for (TActorIterator<AFBBroadcastCamera> It(GetWorld()); It; ++It)
	{
		It->SetActorTickEnabled(false);
		It->SetActorHiddenInGame(true);
	}
}
