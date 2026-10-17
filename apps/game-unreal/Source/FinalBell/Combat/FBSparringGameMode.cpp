#include "Combat/FBSparringGameMode.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBSparringPlayerController.h"
#include "Combat/FBCombatTypes.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Camera/FBBroadcastCamera.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
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
	UE_LOG(LogTemp, Log, TEXT("Final Bell sparring ready. Solo=AI dummy, 2 PIE players=human vs human."));

	if (bAutoBuildP1Arena)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.35f, false);
	}
}

void AFBSparringGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (bAutoBuildP1Arena)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.2f, false);
	}
}

AActor* AFBSparringGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);
	if (Starts.Num() > 0)
	{
		AActor* Chosen = Starts[NextStartIndex % Starts.Num()];
		++NextStartIndex;
		return Chosen;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AFBSparringGameMode::PlaceFightersForMatch()
{
	TArray<AFBBoxerCharacter*> Humans;
	AFBBoxerCharacter* AiDummy = nullptr;

	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer)
		{
			continue;
		}
		if (Boxer->ActorHasTag(TEXT("FB_P1Opponent")) && !Boxer->IsPlayerControlled())
		{
			AiDummy = Boxer;
		}
		else
		{
			Humans.Add(Boxer);
		}
	}

	const int32 HumanCount = Humans.Num();
	const bool bNeedAi = bSpawnAiIfSolo && HumanCount < 2;

	if (!bNeedAi && AiDummy)
	{
		AiDummy->Destroy();
		AiDummy = nullptr;
	}

	AFBBoxerCharacter* Player = nullptr;
	AFBBoxerCharacter* Opponent = nullptr;
	AFBBroadcastCamera* UnusedCam = nullptr;

	if (bNeedAi)
	{
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, UnusedCam, true);
		if (UnusedCam)
		{
			UnusedCam->Destroy(); // each human uses a personal orbit camera
		}
	}
	else
	{
		AFBBoxerCharacter* DummyA = nullptr;
		AFBBoxerCharacter* DummyB = nullptr;
		AFBBroadcastCamera* DummyCam = nullptr;
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), DummyA, DummyB, DummyCam, false);

		Humans.Reset();
		for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
		{
			if (*It && (*It)->IsPlayerControlled())
			{
				Humans.Add(*It);
			}
		}

		if (Humans.Num() >= 1)
		{
			Humans[0]->SetActorLocation(FVector(-220.f, 0.f, 96.f));
			Humans[0]->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
		}
		if (Humans.Num() >= 2)
		{
			Humans[1]->SetActorLocation(FVector(220.f, 0.f, 96.f));
			Humans[1]->ApplyBasicSkin(EFBCornerSkin::RedCorner);
		}
	}

	bArenaBuilt = true;
}

void AFBSparringGameMode::TryBuildArenaAndStart()
{
	if (!HasAuthority())
	{
		return;
	}

	PlaceFightersForMatch();

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
	UE_LOG(LogTemp, Log, TEXT("Final Bell: bout finalized once (rounds=%d)."), TotalRounds);
}
