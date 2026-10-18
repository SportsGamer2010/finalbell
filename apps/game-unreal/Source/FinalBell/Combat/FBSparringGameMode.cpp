#include "Combat/FBSparringGameMode.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBSparringPlayerController.h"
#include "Combat/FBCombatTypes.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Camera/FBBroadcastCamera.h"
#include "Training/FBHeavyBag.h"
#include "Session/FBSessionSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

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
	UE_LOG(LogTemp, Log, TEXT("Final Bell sparring ready — choose mode: 1v1 AI / Multiplayer / Training."));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			if (Session->HasChosenPlayMode())
			{
				ApplyPlayMode(Session->GetPlayMode());
			}
		}
	}

	if (bAutoBuildP1Arena)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.5f, false);
	}
}

void AFBSparringGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (bAutoBuildP1Arena)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.35f, false);
	}
}

void AFBSparringGameMode::ApplyPlayMode(EFBPlayMode Mode)
{
	if (!HasAuthority() || Mode == EFBPlayMode::Unset)
	{
		return;
	}
	ActivePlayMode = Mode;
	bArenaBuilt = false;
	TryBuildArenaAndStart();
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

int32 AFBSparringGameMode::CountHumanBoxers() const
{
	int32 Count = 0;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It && It->IsPlayerControlled())
		{
			++Count;
		}
	}
	return Count;
}

void AFBSparringGameMode::DestroyAllAiOpponents()
{
	// Ring_P1 already contains a placed boxer. The game mode then spawns the
	// possessed pawn, and sparring spawns one opponent. Anything not controlled
	// by a player is removed here so that third body cannot stay in the ring.
	TArray<AFBBoxerCharacter*> ToDestroy;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (Boxer && !Boxer->IsPlayerControlled())
		{
			ToDestroy.Add(Boxer);
		}
	}
	for (AFBBoxerCharacter* Boxer : ToDestroy)
	{
		if (Boxer)
		{
			UE_LOG(LogTemp, Warning, TEXT("FinalBell: removing extra fighter %s"), *Boxer->GetName());
			Boxer->Destroy();
		}
	}
}

void AFBSparringGameMode::EnforceRoster()
{
	const bool bKeepOneOpponent = ActivePlayMode == EFBPlayMode::VersusAI && GetNumPlayers() < 2 && CountHumanBoxers() < 2;
	AFBBoxerCharacter* KeptOpponent = nullptr;
	TArray<AFBBoxerCharacter*> Extras;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer || Boxer->IsPlayerControlled())
		{
			continue;
		}
		if (bKeepOneOpponent && !KeptOpponent)
		{
			KeptOpponent = Boxer;
			continue;
		}
		Extras.Add(Boxer);
	}
	for (AFBBoxerCharacter* Boxer : Extras)
	{
		if (Boxer)
		{
			UE_LOG(LogTemp, Warning, TEXT("FinalBell: removing extra fighter %s"), *Boxer->GetName());
			Boxer->Destroy();
		}
	}
}

void AFBSparringGameMode::DestroyTrainingBag()
{
	TArray<AActor*> Bags;
	for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
	{
		Bags.Add(*It);
	}
	for (AActor* Bag : Bags)
	{
		if (Bag)
		{
			Bag->Destroy();
		}
	}
}

AFBHeavyBag* AFBSparringGameMode::EnsureTrainingBag()
{
	for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<AFBHeavyBag>(FVector(220.f, 0.f, 160.f), FRotator::ZeroRotator, Params);
}

void AFBSparringGameMode::PlaceFightersForMatch()
{
	const int32 HumanCount = CountHumanBoxers();
	const int32 ConnectedPlayers = GetNumPlayers();

	// Resolve mode: explicit choice wins; otherwise infer from player count.
	EFBPlayMode Mode = ActivePlayMode;
	if (Mode == EFBPlayMode::Unset)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
			{
				Mode = Session->GetPlayMode();
			}
		}
	}
	if (Mode == EFBPlayMode::Unset)
	{
		Mode = (ConnectedPlayers >= 2 || HumanCount >= 2) ? EFBPlayMode::Multiplayer : EFBPlayMode::VersusAI;
	}
	ActivePlayMode = Mode;

	const bool bWantAi = (Mode == EFBPlayMode::VersusAI) && bSpawnAiIfSolo;
	const bool bWantBag = (Mode == EFBPlayMode::Training);

	// Clear the placed map boxer and any previous opponent before deciding the roster.
	DestroyAllAiOpponents();
	if (!bWantBag)
	{
		DestroyTrainingBag();
	}

	AFBBoxerCharacter* Player = nullptr;
	AFBBoxerCharacter* Opponent = nullptr;
	AFBBroadcastCamera* UnusedCam = nullptr;

	if (bWantBag)
	{
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, UnusedCam, false);
		DestroyAllAiOpponents();
		EnsureTrainingBag();
		if (UnusedCam)
		{
			UnusedCam->Destroy();
		}
		TArray<AFBBoxerCharacter*> Humans;
		for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
		{
			if (*It && It->IsPlayerControlled())
			{
				Humans.Add(*It);
			}
		}
		if (Humans.Num() >= 1)
		{
			Humans[0]->SetActorLocation(FVector(-180.f, 0.f, Humans[0]->GetStandingHalfHeight() + 6.f));
			Humans[0]->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
		}
	}
	else if (bWantAi && HumanCount < 2 && ConnectedPlayers < 2)
	{
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, UnusedCam, true);
		if (UnusedCam)
		{
			UnusedCam->Destroy();
		}
	}
	else
	{
		// Multiplayer / two humans: ring only, no AI.
		AFBBoxerCharacter* DummyA = nullptr;
		AFBBoxerCharacter* DummyB = nullptr;
		AFBBroadcastCamera* DummyCam = nullptr;
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), DummyA, DummyB, DummyCam, false);
		DestroyAllAiOpponents();
		if (DummyCam)
		{
			DummyCam->Destroy();
		}

		TArray<AFBBoxerCharacter*> Humans;
		for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
		{
			if (*It && It->IsPlayerControlled())
			{
				Humans.Add(*It);
			}
		}
		if (Humans.Num() >= 1)
		{
			Humans[0]->SetActorLocation(FVector(-95.f, 0.f, Humans[0]->GetStandingHalfHeight() + 6.f));
			Humans[0]->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
		}
		if (Humans.Num() >= 2)
		{
			Humans[1]->SetActorLocation(FVector(95.f, 0.f, Humans[1]->GetStandingHalfHeight() + 6.f));
			Humans[1]->ApplyBasicSkin(EFBCornerSkin::RedCorner);
		}
	}

	// One opponent in solo sparring. None beside a second human or the heavy bag.
	EnforceRoster();

	UE_LOG(LogTemp, Warning, TEXT("FinalBell PlaceFighters mode=%d humans=%d players=%d"),
		(int32)Mode, CountHumanBoxers(), ConnectedPlayers);
	bArenaBuilt = true;
}

void AFBSparringGameMode::TryBuildArenaAndStart()
{
	if (!HasAuthority())
	{
		return;
	}

	PlaceFightersForMatch();

	if (bAutoStartBout && Phase == EFBBoutPhase::PreBout && ActivePlayMode != EFBPlayMode::Unset)
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
	UE_LOG(LogTemp, Log, TEXT("Final Bell: bout started (round 1/%d) mode=%d."), TotalRounds, (int32)ActivePlayMode);
}

void AFBSparringGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && bArenaBuilt)
	{
		if (GetNumPlayers() >= 2 || CountHumanBoxers() >= 2)
		{
			ActivePlayMode = EFBPlayMode::Multiplayer;
		}
		EnforceRoster();
	}

	if (!HasAuthority() || Phase == EFBBoutPhase::PreBout || Phase == EFBBoutPhase::Complete)
	{
		return;
	}

	if (ActivePlayMode == EFBPlayMode::Training)
	{
		return; // no round clock in bag work
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
