#include "Combat/FBSparringGameMode.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBSparringPlayerController.h"
#include "Camera/FBBroadcastCamera.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "TimerManager.h"

AFBSparringGameMode::AFBSparringGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AFBBoxerCharacter::StaticClass();
	PlayerControllerClass = AFBSparringPlayerController::StaticClass();
	bUseSeamlessTravel = false;
}

void AFBSparringGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("Final Bell sparring mode ready (auto-arena=%s)."), bAutoBuildP1Arena ? TEXT("on") : TEXT("off"));

	if (bAutoBuildP1Arena)
	{
		// Delay so the default pawn has time to spawn on Entry map.
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.25f, false);
	}
}

void AFBSparringGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (bAutoBuildP1Arena && !bArenaBuilt)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.15f, false);
	}
}

void AFBSparringGameMode::TryBuildArenaAndStart()
{
	if (!HasAuthority() || bArenaBuilt || !bAutoBuildP1Arena)
	{
		return;
	}

	AFBBoxerCharacter* Player = nullptr;
	AFBBoxerCharacter* Opponent = nullptr;
	AFBBroadcastCamera* Camera = nullptr;
	UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, Camera);
	bArenaBuilt = (Player != nullptr || Opponent != nullptr);

	if (Camera)
	{
		Camera->ForcePossessedView();
	}

	if (bAutoStartBout && Phase == EFBBoutPhase::PreBout)
	{
		StartBout();
	}
}

UClass* AFBSparringGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return DefaultPawnClass;
}

void AFBSparringGameMode::StartBout()
{
	if (!HasAuthority() || Phase != EFBBoutPhase::PreBout)
	{
		return;
	}
	CurrentRound = 1;
	Phase = EFBBoutPhase::RoundActive;
	PhaseTimeRemaining = RoundDurationSeconds;
	bResultFinalized = false;
	UE_LOG(LogTemp, Log, TEXT("Final Bell: bout started (round 1/%d)."), TotalRounds);
}

void AFBSparringGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || Phase == EFBBoutPhase::PreBout || Phase == EFBBoutPhase::Complete)
	{
		return;
	}

	PhaseTimeRemaining -= DeltaSeconds;
	if (PhaseTimeRemaining <= 0.f)
	{
		AdvancePhase();
	}
}

void AFBSparringGameMode::AdvancePhase()
{
	if (Phase == EFBBoutPhase::RoundActive)
	{
		if (CurrentRound >= TotalRounds)
		{
			Phase = EFBBoutPhase::Complete;
			FinalizeResultOnce();
			return;
		}
		Phase = EFBBoutPhase::BetweenRounds;
		PhaseTimeRemaining = RestDurationSeconds;
		return;
	}

	if (Phase == EFBBoutPhase::BetweenRounds)
	{
		++CurrentRound;
		Phase = EFBBoutPhase::RoundActive;
		PhaseTimeRemaining = RoundDurationSeconds;
	}
}

void AFBSparringGameMode::FinalizeResultOnce()
{
	if (bResultFinalized)
	{
		return;
	}
	bResultFinalized = true;
	UE_LOG(LogTemp, Log, TEXT("Final Bell: bout finalized once (rounds=%d). Scorecards TBD P1+/P2."), TotalRounds);
}
