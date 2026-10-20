#include "Combat/FBSparringGameMode.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBStylePolicy.h"
#include "Ring/FBRingBounds.h"
#include "Combat/FBSparringGameState.h"
#include "Combat/FBSparringPlayerController.h"
#include "Combat/FBCombatTypes.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Camera/FBBroadcastCamera.h"
#include "Training/FBHeavyBag.h"
#include "Session/FBSessionSubsystem.h"
#include "UI/FBGameInstance.h"
#include "Combat/FBFighterSave.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AFBSparringGameMode::AFBSparringGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AFBBoxerCharacter::StaticClass();
	PlayerControllerClass = AFBSparringPlayerController::StaticClass();
	GameStateClass = AFBSparringGameState::StaticClass();
	bUseSeamlessTravel = false;
}

void AFBSparringGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		if (FrontEnd->WantsFrontEndBoot())
		{
			FrontEnd->OpenFrontEnd(this);
			return;
		}
	}
	FString RulesError;
	if (!FFBFighterBalance::SelfCheck(RulesError))
	{
		UE_LOG(LogTemp, Error, TEXT("CompetitiveFair self-check failed: %s"), *RulesError);
	}
	UE_LOG(LogTemp, Log, TEXT("Final Bell sparring ready — 1v1 Sparring (AI) / 1v1 Multiplayer / Heavy Bag Training / 1v1 Career. Ruleset CompetitiveFair v%d."), FFBFighterBalance::RulesetVersion);

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
	if (bAutoBuildP1Arena && !bArenaBuilt)
	{
		GetWorldTimerManager().SetTimer(ArenaBuildTimer, this, &AFBSparringGameMode::TryBuildArenaAndStart, 0.35f, false);
	}
}

void AFBSparringGameMode::RestartPlayer(AController* NewPlayer)
{
	const bool bSoloAi = ActivePlayMode == EFBPlayMode::VersusAI || ActivePlayMode == EFBPlayMode::Career;
	if (bSoloAi && NewPlayer && BoutRequester.IsValid() && NewPlayer != BoutRequester.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
		{
			if (!PC->IsInState(NAME_Spectating))
			{
				PC->StartSpectatingOnly();
			}
		}
		return;
	}
	Super::RestartPlayer(NewPlayer);
}

void AFBSparringGameMode::ApplyPlayMode(EFBPlayMode Mode)
{
	if (!HasAuthority() || Mode == EFBPlayMode::Unset)
	{
		return;
	}
	bAwaitingFighterConfirm = false;
	bPresentationPaused = false;
	ResetBoutState();
	ActivePlayMode = Mode;
	bArenaBuilt = false;
	TryBuildArenaAndStart();
}

void AFBSparringGameMode::SetAwaitingFighterConfirm(bool bAwaiting)
{
	if (!HasAuthority())
	{
		return;
	}
	bAwaitingFighterConfirm = bAwaiting;
	if (bAwaiting)
	{
		GetWorldTimerManager().ClearTimer(ArenaBuildTimer);
		ResetBoutState();
		ActivePlayMode = EFBPlayMode::Unset;
		PushBoutState();
	}
}

void AFBSparringGameMode::AbandonBoutForMenu()
{
	if (!HasAuthority())
	{
		return;
	}
	bAwaitingFighterConfirm = false;
	bPresentationPaused = false;
	GetWorldTimerManager().ClearTimer(ArenaBuildTimer);
	ResetBoutState();
	ActivePlayMode = EFBPlayMode::Unset;
	bArenaBuilt = false;
	PushBoutState();
}

void AFBSparringGameMode::RequestRematch()
{
	if (!HasAuthority() || ActivePlayMode == EFBPlayMode::Unset || ActivePlayMode == EFBPlayMode::Training)
	{
		return;
	}
	if (!bResultFinalized && Phase != EFBBoutPhase::Complete)
	{
		return;
	}
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It)
		{
			It->AuthorityPrepareForRematch();
		}
	}
	ResetBoutState();
	SendFightersToCorners();
	StartBout();
}

void AFBSparringGameMode::SendFightersToCorners()
{
	if (ActivePlayMode == EFBPlayMode::Training)
	{
		return;
	}
	AFBRingBounds* Ring = nullptr;
	for (TActorIterator<AFBRingBounds> It(GetWorld()); It; ++It)
	{
		Ring = *It;
		break;
	}
	if (!Ring)
	{
		return;
	}
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer)
		{
			continue;
		}
		const UCapsuleComponent* Capsule = Boxer->GetCapsuleComponent();
		const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 34.f;
		FVector Location = FVector::ZeroVector;
		FRotator Facing = FRotator::ZeroRotator;
		if (!Ring->GetCornerAnchor(Boxer->CornerSkin, Radius, Location, Facing))
		{
			continue;
		}
		Location.Z = Boxer->GetStandingHalfHeight() + 6.f;
		Boxer->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Boxer->SetActorRotation(Facing);
		if (UCharacterMovementComponent* Move = Boxer->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
	}
}

bool AFBSparringGameMode::AllowsNewAttacks() const
{
	if (ActivePlayMode == EFBPlayMode::Training)
	{
		return !bResultFinalized;
	}
	if (bKnockdownActive || bResultFinalized)
	{
		return false;
	}
	if (Phase == EFBBoutPhase::BetweenRounds || Phase == EFBBoutPhase::Complete)
	{
		return false;
	}
	if (Phase == EFBBoutPhase::RoundActive && PhaseTimeRemaining <= 0.f)
	{
		return false;
	}
	return true;
}

bool AFBSparringGameMode::AnyGloveInFlight() const
{
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		const UFBCombatComponent* Combat = It->GetCombatComponent();
		if (!Combat)
		{
			continue;
		}
		const EFBAttackPhase AttackPhase = Combat->GetAttackPhase();
		if (AttackPhase == EFBAttackPhase::Startup || AttackPhase == EFBAttackPhase::Active)
		{
			return true;
		}
	}
	return false;
}

void AFBSparringGameMode::SetPresentationPaused(bool bPaused)
{
	if (!HasAuthority())
	{
		return;
	}
	bPresentationPaused = bPaused;
}

bool AFBSparringGameMode::HadRosterRefusal(FString& OutWhy) const
{
	if (LastRosterRefusal.IsEmpty())
	{
		return false;
	}
	OutWhy = LastRosterRefusal;
	return true;
}

void AFBSparringGameMode::ResetBoutState()
{
	BreakClinch(TEXT(""));
	HideReferee();
	SetRefereeCameras(false, nullptr);
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer)
		{
			continue;
		}
		Boxer->SetKnockedDown(false);
		Boxer->AuthoritySetClinch(false);
	}
	Phase = EFBBoutPhase::PreBout;
	CurrentRound = 0;
	PhaseTimeRemaining = 0.f;
	bResultFinalized = false;
	bKnockdownActive = false;
	CountRemaining = 0.f;
	AlignSeconds = 0.f;
	AiGetUpDelay = 0.f;
	DownedBoxer = nullptr;
	BlueLedger = FFBCornerLedger();
	RedLedger = FFBCornerLedger();
	BuiltCard = FFBOfficialCard();
	RefereeCall.Reset();
	RefereeCallUntil = 0.f;
	TelemetryClean = 0;
	TelemetryMiss = 0;
	TelemetryBlock = 0;
	TelemetryBody = 0;
	TelemetryGuardBreaks = 0;
	TelemetryDowns = 0;
	LastRosterRefusal.Reset();
	PushBoutState();
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

void AFBSparringGameMode::NoteBoutRequester(AController* Requester)
{
	BoutRequester = Requester;
}

void AFBSparringGameMode::BenchExtraHumansForSoloAi()
{
	const bool bSoloAi = ActivePlayMode == EFBPlayMode::VersusAI || ActivePlayMode == EFBPlayMode::Career;
	if (!bSoloAi || !GetWorld())
	{
		return;
	}
	AController* Keep = BoutRequester.Get();
	if (!Keep)
	{
		Keep = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
	if (!Keep)
	{
		return;
	}

	TArray<APlayerController*> Extra;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC != Keep)
		{
			Extra.Add(PC);
		}
	}
	for (APlayerController* PC : Extra)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (Pawn->IsA(AFBBoxerCharacter::StaticClass()))
			{
				UE_LOG(LogTemp, Warning, TEXT("FinalBell: benching extra PIE pawn %s so 1v1 AI stays two fighters"), *Pawn->GetName());
				PC->UnPossess();
				Pawn->Destroy();
			}
		}
		if (!PC->IsInState(NAME_Spectating))
		{
			PC->StartSpectatingOnly();
		}
		if (APawn* Spec = PC->GetSpectatorPawn())
		{
			if (!Spec->IsHidden())
			{
				Spec->SetActorHiddenInGame(true);
				Spec->SetActorEnableCollision(false);
				Spec->SetActorLocation(FVector(0.f, 0.f, -4000.f));
			}
		}
	}
}

void AFBSparringGameMode::EnforceRoster()
{
	// Sparring and career keep one AI even when a second PIE client is in the
	// world. That client is not the selected premade. Dropping the AI here is
	// what made a legal pair report that the opponent never entered the ring.
	const bool bSoloAi = ActivePlayMode == EFBPlayMode::VersusAI || ActivePlayMode == EFBPlayMode::Career;
	const bool bKeepOneOpponent = bSoloAi;
	AFBBoxerCharacter* KeptOpponent = nullptr;
	TArray<AFBBoxerCharacter*> Extras;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer || Boxer->IsPlayerControlled())
		{
			continue;
		}
		if (bKeepOneOpponent && Boxer->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			if (KeptOpponent && !KeptOpponent->ActorHasTag(TEXT("FB_P1Opponent")))
			{
				Extras.Add(KeptOpponent);
			}
			KeptOpponent = Boxer;
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
	AFBHeavyBag* Bag = nullptr;
	UFBP1ArenaBootstrap::EnsureBagRoom(GetWorld(), Bag);
	return Bag;
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

	const bool bWantAi = (Mode == EFBPlayMode::VersusAI || Mode == EFBPlayMode::Career) && bSpawnAiIfSolo;
	const bool bWantBag = (Mode == EFBPlayMode::Training);

	// Clear the placed map boxer and any previous opponent before deciding the roster.
	DestroyAllAiOpponents();
	if (!bWantBag)
	{
		DestroyTrainingBag();
		UFBP1ArenaBootstrap::DestroyBagRoom(GetWorld());
	}

	AFBBoxerCharacter* Player = nullptr;
	AFBBoxerCharacter* Opponent = nullptr;
	AFBBroadcastCamera* UnusedCam = nullptr;

	if (bWantBag)
	{
		// The bout ring stays where it is. Training stands in the bag room north of that gym.
		DestroyAllAiOpponents();
		AFBHeavyBag* Bag = EnsureTrainingBag();
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
			const FFBBagRoomLayout Room = UFBP1ArenaBootstrap::GetBagRoomLayout();
			const FVector BagCenter = Bag ? Bag->GetStrikeCenter() : FVector(0.f, Room.BagY, Room.FloorTopZ + 150.f);
			const float Half = Humans[0]->GetStandingHalfHeight();
			const FVector Stand(BagCenter.X, BagCenter.Y - Room.StandOff, Room.FloorTopZ + Half);
			Humans[0]->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
			Humans[0]->SetActorRotation(FRotator(0.f, 90.f, 0.f));
			UE_LOG(LogTemp, Warning, TEXT("FinalBell training stand (%.0f, %.0f, %.0f) feetZ=%.0f off bag by %.0f"),
				Stand.X, Stand.Y, Stand.Z, Room.FloorTopZ, Room.StandOff);
			Humans[0]->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
		}
	}
	else if (bWantAi)
	{
		// Editor PIE logs in a second human. That pawn is not the selected
		// premade. Leave it in the ring and the bout shows three boxers.
		BenchExtraHumansForSoloAi();
		// A second connected player used to skip this spawn and then delete
		// every AI. The selected premade never got a body, so the bout said
		// the opponent did not enter the ring.
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), Player, Opponent, UnusedCam, true, true);
		if (!Opponent || Opponent->IsPlayerControlled())
		{
			Opponent = nullptr;
			for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
			{
				if (*It && !It->IsPlayerControlled())
				{
					Opponent = *It;
					break;
				}
			}
		}
		if (Opponent)
		{
			if (!Opponent->ActorHasTag(TEXT("FB_P1Opponent")))
			{
				Opponent->Tags.Add(TEXT("FB_P1Opponent"));
			}
			if (!Opponent->GetController())
			{
				Opponent->SpawnDefaultController();
			}
			Opponent->ApplyBasicSkin(EFBCornerSkin::RedCorner);
		}
		AFBBoxerCharacter* CameraPlayer = Player;
		if (!CameraPlayer && BoutRequester.IsValid())
		{
			CameraPlayer = Cast<AFBBoxerCharacter>(BoutRequester->GetPawn());
		}
		if (UnusedCam)
		{
			UnusedCam->SetFighters(CameraPlayer, Opponent);
			UnusedCam->ForcePossessedView();
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
			DummyCam->SetFighters(DummyA, DummyB);
			DummyCam->ForcePossessedView();
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
			Humans[0]->ApplyBasicSkin(EFBCornerSkin::BlueCorner);
		}
		if (Humans.Num() >= 2)
		{
			Humans[1]->ApplyBasicSkin(EFBCornerSkin::RedCorner);
		}
	}

	// One opponent in solo sparring. None beside a second human or the heavy bag.
	EnforceRoster();
	if (!bWantBag)
	{
		SendFightersToCorners();
	}

	ApplyPendingBuilds();
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It && It->IsLocallyControlled())
		{
			It->RefreshFightCamera();
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("FinalBell PlaceFighters mode=%d humans=%d players=%d"),
		(int32)Mode, CountHumanBoxers(), ConnectedPlayers);
	bArenaBuilt = true;
}

void AFBSparringGameMode::ApplyPendingBuilds()
{
	if (ActivePlayMode != EFBPlayMode::VersusAI && ActivePlayMode != EFBPlayMode::Career)
	{
		return;
	}
	LastRosterRefusal.Reset();
	UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance());
	if (!FrontEnd || !FrontEnd->HasPendingBout())
	{
		return;
	}
	AFBBoxerCharacter* Human = nullptr;
	if (AController* Requester = BoutRequester.Get())
	{
		Human = Cast<AFBBoxerCharacter>(Requester->GetPawn());
	}
	AFBBoxerCharacter* Ai = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (!*It)
		{
			continue;
		}
		if (!Human && It->IsPlayerControlled() && !It->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			Human = *It;
		}
		if (It->IsPlayerControlled())
		{
			continue;
		}
		if (!Ai || It->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			Ai = *It;
		}
	}
	if (!Ai)
	{
		AFBBoxerCharacter* SpawnedPlayer = nullptr;
		AFBBroadcastCamera* SpawnedCam = nullptr;
		UFBP1ArenaBootstrap::EnsureArena(GetWorld(), SpawnedPlayer, Ai, SpawnedCam, true, true);
		if (Ai && Ai->IsPlayerControlled())
		{
			Ai = nullptr;
		}
		if (Ai)
		{
			if (!Ai->GetController())
			{
				Ai->SpawnDefaultController();
			}
			if (SpawnedCam)
			{
				SpawnedCam->SetFighters(Human ? Human : SpawnedPlayer, Ai);
				SpawnedCam->ForcePossessedView();
			}
		}
	}
	FString Why;
	if (!Human)
	{
		LastRosterRefusal = TEXT("The ring is not ready yet.");
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused a bout: no player fighter."));
		return;
	}
	if (!Human->ApplyFighterRecord(FrontEnd->GetPendingPlayer(), Why))
	{
		LastRosterRefusal = Why.IsEmpty() ? TEXT("The player build was refused.") : Why;
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused player build: %s"), *LastRosterRefusal);
		return;
	}
	if (!Ai)
	{
		LastRosterRefusal = TEXT("The opponent did not enter the ring.");
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused a bout: no opponent."));
		return;
	}
	if (!Ai->ApplyFighterRecord(FrontEnd->GetPendingOpponent(), Why))
	{
		LastRosterRefusal = Why.IsEmpty() ? TEXT("The opponent build was refused.") : Why;
		UE_LOG(LogTemp, Warning, TEXT("FinalBell refused opponent build: %s"), *LastRosterRefusal);
	}
}

void AFBSparringGameMode::TryBuildArenaAndStart()
{
	if (!HasAuthority() || bAwaitingFighterConfirm)
	{
		return;
	}
	if (ActivePlayMode == EFBPlayMode::Unset)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
			{
				if (Session->HasChosenPlayMode())
				{
					ActivePlayMode = Session->GetPlayMode();
				}
			}
		}
	}
	if (ActivePlayMode == EFBPlayMode::Unset)
	{
		return;
	}
	if (bArenaBuilt)
	{
		return;
	}

	PlaceFightersForMatch();

	if (!LastRosterRefusal.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell did not start the bout: %s"), *LastRosterRefusal);
		return;
	}

	if (bAutoStartBout && Phase == EFBBoutPhase::PreBout && ActivePlayMode != EFBPlayMode::Unset)
	{
		StartBout();
	}
}

UClass* AFBSparringGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return DefaultPawnClass;
}

void AFBSparringGameMode::EnsureConfirmingPlayerPossessesFighter()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(BoutRequester.Get());
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}
	if (!PC || Cast<AFBBoxerCharacter>(PC->GetPawn()))
	{
		return;
	}

	AFBBoxerCharacter* Human = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!Boxer || Boxer->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			continue;
		}
		AController* HeldBy = Boxer->GetController();
		if (HeldBy && HeldBy != PC && HeldBy->IsPlayerController())
		{
			continue;
		}
		Human = Boxer;
		break;
	}
	if (!Human)
	{
		UE_LOG(LogTemp, Warning, TEXT("FinalBell: confirming player has no fighter at the bell."));
		return;
	}
	if (AController* Other = Human->GetController())
	{
		if (Other != PC)
		{
			Other->UnPossess();
		}
	}
	PC->Possess(Human);
}

void AFBSparringGameMode::StartBout()
{
	if (!HasAuthority() || Phase != EFBBoutPhase::PreBout)
	{
		return;
	}
	CurrentRound = 1;
	SendFightersToCorners();
	EnsureConfirmingPlayerPossessesFighter();
	Phase = EFBBoutPhase::RoundActive;
	PhaseTimeRemaining = RoundDurationSeconds;
	bResultFinalized = false;
	bKnockdownActive = false;
	CountRemaining = 0.f;
	AlignSeconds = 0.f;
	BlueLedger = FFBCornerLedger();
	RedLedger = FFBCornerLedger();
	BuiltCard = FFBOfficialCard();
	DownedBoxer = nullptr;
	HideReferee();
	if (ActivePlayMode == EFBPlayMode::Training)
	{
		UE_LOG(LogTemp, Log, TEXT("Final Bell: heavy bag training started. No bout clock."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Final Bell: 3:00 bout started. mode=%d."), (int32)ActivePlayMode);
	}
	PushBoutState();
}

void AFBSparringGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPresentationPaused)
	{
		return;
	}

	if (HasAuthority() && bArenaBuilt && !bAwaitingFighterConfirm)
	{
		// An explicit sparring, career, or bag choice stays. Two PIE clients
		// used to flip the mode to multiplayer and delete the AI after Fight.
		if (ActivePlayMode == EFBPlayMode::Unset && (GetNumPlayers() >= 2 || CountHumanBoxers() >= 2))
		{
			ActivePlayMode = EFBPlayMode::Multiplayer;
		}
		if (ActivePlayMode == EFBPlayMode::VersusAI || ActivePlayMode == EFBPlayMode::Career)
		{
			BenchExtraHumansForSoloAi();
		}
		EnforceRoster();
	}

	if (!HasAuthority() || Phase == EFBBoutPhase::PreBout || Phase == EFBBoutPhase::Complete)
	{
		PushBoutState();
		return;
	}

	if (ActivePlayMode == EFBPlayMode::Training)
	{
		TickClinch(DeltaSeconds);
		PushBoutState();
		return;
	}

	if (RefereeCallUntil > 0.f && GetWorld() && GetWorld()->GetTimeSeconds() >= RefereeCallUntil)
	{
		RefereeCall.Reset();
		RefereeCallUntil = 0.f;
	}
	TickClinch(DeltaSeconds);

	// The referee's count is its own clock. The 3:00 waits so a late knockdown
	// still gets the full ten seconds.
	if (bKnockdownActive)
	{
		LeftTargetDegrees = FMath::UnwindDegrees(LeftTargetDegrees + 78.f * DeltaSeconds);
		RightTargetDegrees = FMath::UnwindDegrees(RightTargetDegrees - 112.f * DeltaSeconds);
		CountRemaining -= DeltaSeconds;

		if (AFBBoxerCharacter* Downed = DownedBoxer.Get())
		{
			if (!Downed->IsPlayerControlled())
			{
				AiGetUpDelay += DeltaSeconds;
				const float CourageDelay = 3.2f * Downed->GetCombatDerived().CourageHold;
				if (LedgerFor(Downed).Knockdowns < 2 && AiGetUpDelay >= CourageDelay)
				{
					CompleteGetUp();
				}
			}
		}

		if (bKnockdownActive && CountRemaining <= 0.f)
		{
			FinishKnockout();
		}
	}
	else
	{
		TickRingGeneralship(DeltaSeconds);
		PhaseTimeRemaining -= DeltaSeconds;
		if (PhaseTimeRemaining <= 0.f)
		{
			PhaseTimeRemaining = 0.f;
			if (!AnyGloveInFlight())
			{
				AdvancePhase();
			}
		}
	}

	PushBoutState();
}

void AFBSparringGameMode::AdvancePhase()
{
	if (Phase == EFBBoutPhase::RoundActive)
	{
		if (CurrentRound >= TotalRounds)
		{
			SendFightersToCorners();
			Phase = EFBBoutPhase::Complete;
			FinalizeResultOnce();
			return;
		}
		Phase = EFBBoutPhase::BetweenRounds;
		PhaseTimeRemaining = RestDurationSeconds;
		RestoreBetweenRounds();
		SendFightersToCorners();
		return;
	}

	if (Phase == EFBBoutPhase::BetweenRounds)
	{
		++CurrentRound;
		Phase = EFBBoutPhase::RoundActive;
		PhaseTimeRemaining = RoundDurationSeconds;
		SendFightersToCorners();
	}
}

void AFBSparringGameMode::FinalizeResultOnce()
{
	if (bResultFinalized)
	{
		return;
	}
	bResultFinalized = true;
	if (BuiltCard.Result == EFBBoutResult::None)
	{
		BuildDecisionCard();
	}
	Phase = EFBBoutPhase::Complete;
	bKnockdownActive = false;
	BreakClinch(TEXT(""));
	HideReferee();
	SetRefereeCameras(false, nullptr);
	const FString Why = FFBFighterBalance::ExplainBout(TelemetryClean, TelemetryMiss, TelemetryBlock, TelemetryBody, TelemetryGuardBreaks, TelemetryDowns);
	BuiltCard.DeductionNote = BuiltCard.DeductionNote.IsEmpty() ? Why : BuiltCard.DeductionNote + TEXT("\n") + Why;
	UE_LOG(LogTemp, Log, TEXT("CompetitiveFair v%d clean=%d miss=%d block=%d body=%d guardBreak=%d down=%d | %s"),
		FFBFighterBalance::RulesetVersion, TelemetryClean, TelemetryMiss, TelemetryBlock, TelemetryBody, TelemetryGuardBreaks, TelemetryDowns, *Why);
	AwardCareerFromCard();
	UE_LOG(LogTemp, Log, TEXT("Final Bell: bout finalized — %s %s"), *BuiltCard.Decision, *BuiltCard.Winner);
	PushBoutState();
}

void AFBSparringGameMode::PushBoutState()
{
	AFBSparringGameState* BoutState = GetGameState<AFBSparringGameState>();
	if (!BoutState)
	{
		return;
	}
	BoutState->BoutSecondsRemaining = PhaseTimeRemaining;
	BoutState->bRoundRunning = Phase == EFBBoutPhase::RoundActive
		&& !bKnockdownActive
		&& !bResultFinalized
		&& ActivePlayMode != EFBPlayMode::Training;
	BoutState->bKnockdown = bKnockdownActive;
	BoutState->CountRemaining = CountRemaining;
	BoutState->LeftTargetDegrees = LeftTargetDegrees;
	BoutState->RightTargetDegrees = RightTargetDegrees;
	BoutState->DownedBoxer = DownedBoxer.Get();
	BoutState->bCardVisible = bResultFinalized;
	BoutState->OfficialCard = BuiltCard;
	BoutState->RefereeCall = RefereeCall;
}

void AFBSparringGameMode::NotifyEnergyDepleted(AFBBoxerCharacter* Boxer)
{
	if (!HasAuthority() || !Boxer || bKnockdownActive || bResultFinalized)
	{
		return;
	}
	if (Phase != EFBBoutPhase::RoundActive || ActivePlayMode == EFBPlayMode::Training)
	{
		return;
	}
	if (Boxer->IsKnockedDown())
	{
		return;
	}
	BeginKnockdown(Boxer);
}

void AFBSparringGameMode::NoteImpact(AFBBoxerCharacter* Attacker, const FFBImpactEvent& Impact)
{
	if (!HasAuthority() || !Attacker || Phase != EFBBoutPhase::RoundActive)
	{
		return;
	}
	if (Impact.Result == EFBContactResult::Miss)
	{
		++TelemetryMiss;
		return;
	}
	if (Impact.Result == EFBContactResult::CleanHit)
	{
		++TelemetryClean;
		if (Impact.Region == EFBHitRegion::Body)
		{
			++TelemetryBody;
		}
	}
	else if (Impact.Result == EFBContactResult::Blocked)
	{
		++TelemetryBlock;
	}
	FFBCornerLedger& Ledger = LedgerFor(Attacker);
	Ledger.PunchCredit += (Impact.Result == EFBContactResult::CleanHit) ? 1.f : 0.25f;
	if (Impact.Region == EFBHitRegion::Body)
	{
		Ledger.BodyDealt += Impact.DamageApplied;
	}
	else
	{
		Ledger.HeadDealt += Impact.DamageApplied;
	}
}

void AFBSparringGameMode::SubmitGetUpAim(APlayerController* From, FVector2D LeftStick, FVector2D RightStick)
{
	if (!HasAuthority() || !bKnockdownActive || !From)
	{
		return;
	}
	AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(From->GetPawn());
	if (!Boxer || Boxer != DownedBoxer.Get())
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (FMath::IsNearlyEqual(Now, LastAimServerTime))
	{
		return;
	}
	LastAimServerTime = Now;

	const float AimWindow = 24.f * Boxer->GetCombatDerived().CourageAngle;
	auto IsOnMark = [AimWindow](const FVector2D& Stick, float TargetDegrees)
	{
		if (Stick.Size() < 0.58f)
		{
			return false;
		}
		const float Angle = FMath::RadiansToDegrees(FMath::Atan2(Stick.Y, Stick.X));
		return FMath::Abs(FMath::FindDeltaAngleDegrees(Angle, TargetDegrees)) <= AimWindow;
	};

	const float Step = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	if (IsOnMark(LeftStick, LeftTargetDegrees) && IsOnMark(RightStick, RightTargetDegrees))
	{
		AlignSeconds += Step;
	}
	else
	{
		AlignSeconds = 0.f;
	}

	if (AlignSeconds >= 0.32f * Boxer->GetCombatDerived().CourageHold)
	{
		CompleteGetUp();
	}
}

FFBCornerLedger& AFBSparringGameMode::LedgerFor(const AFBBoxerCharacter* Boxer)
{
	if (Boxer && Boxer->CornerSkin == EFBCornerSkin::RedCorner)
	{
		return RedLedger;
	}
	return BlueLedger;
}

void AFBSparringGameMode::BeginKnockdown(AFBBoxerCharacter* Boxer)
{
	if (!Boxer || ActivePlayMode == EFBPlayMode::Training)
	{
		return;
	}
	bKnockdownActive = true;
	CountRemaining = 10.f;
	AlignSeconds = 0.f;
	LastAimServerTime = -1.f;
	AiGetUpDelay = 0.f;
	LeftTargetDegrees = 25.f;
	RightTargetDegrees = 200.f;
	DownedBoxer = Boxer;
	BreakClinch(TEXT(""));
	Boxer->SetKnockedDown(true);
	if (UFBCombatComponent* Combat = Boxer->GetCombatComponent())
	{
		Combat->AbortAttack();
	}
	LedgerFor(Boxer).Knockdowns += 1;
	ShowReferee(Boxer);
	SetRefereeCameras(true, Boxer);
	++TelemetryDowns;
	UE_LOG(LogTemp, Log, TEXT("FinalBell knockdown. count starts at 10."));
	PushBoutState();
}

void AFBSparringGameMode::CompleteGetUp()
{
	if (AFBBoxerCharacter* Boxer = DownedBoxer.Get())
	{
		if (UFBCombatComponent* Combat = Boxer->GetCombatComponent())
		{
			Combat->RestoreBurstEnergy(46.f);
		}
		Boxer->SetKnockedDown(false);
		Boxer->ConsumeDazeStun();
	}
	bKnockdownActive = false;
	CountRemaining = 0.f;
	AlignSeconds = 0.f;
	DownedBoxer = nullptr;
	HideReferee();
	SetRefereeCameras(false, nullptr);
	UE_LOG(LogTemp, Log, TEXT("FinalBell beat the count."));
	PushBoutState();
}

void AFBSparringGameMode::FinishKnockout()
{
	AFBBoxerCharacter* Loser = DownedBoxer.Get();
	AFBBoxerCharacter* Winner = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It && *It != Loser)
		{
			Winner = *It;
			break;
		}
	}
	BuildKnockoutCard(Winner);
	FinalizeResultOnce();
}

float AFBSparringGameMode::JudgePoints(const FFBCornerLedger& Ledger, int32 JudgeIndex) const
{
	// Same three pillars. The weights are what let a card split.
	switch (JudgeIndex)
	{
	case 0:
		// Harris: punch count.
		return Ledger.Generalship * 0.12f + Ledger.Knockdowns * 6.5f + Ledger.PunchCredit * 1.35f;
	case 1:
		// Cole: ring generalship.
		return Ledger.Generalship * 0.42f + Ledger.Knockdowns * 7.f + Ledger.PunchCredit * 0.70f;
	default:
		// Reed: knockdowns.
		return Ledger.Generalship * 0.22f + Ledger.Knockdowns * 12.f + Ledger.PunchCredit * 0.85f;
	}
}

int32 AFBSparringGameMode::ScoreLoser(const FFBCornerLedger& Winner, const FFBCornerLedger& Loser, float Margin) const
{
	(void)Winner;
	if (Loser.Knockdowns >= 2)
	{
		return 7;
	}
	if (Loser.Knockdowns > 0 || Margin > 8.f)
	{
		return 8;
	}
	return 9;
}

void AFBSparringGameMode::BuildDecisionCard()
{
	BuiltCard = FFBOfficialCard();
	BuiltCard.Result = EFBBoutResult::Decision;
	BuiltCard.BlueGeneralship = BlueLedger.Generalship;
	BuiltCard.RedGeneralship = RedLedger.Generalship;
	BuiltCard.BlueKnockdowns = BlueLedger.Knockdowns;
	BuiltCard.RedKnockdowns = RedLedger.Knockdowns;
	BuiltCard.BluePunches = BlueLedger.PunchCredit;
	BuiltCard.RedPunches = RedLedger.PunchCredit;
	const TCHAR* Names[] = { TEXT("HARRIS"), TEXT("COLE"), TEXT("REED") };
	int32 BlueWins = 0;
	int32 RedWins = 0;
	int32 Draws = 0;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float BluePoints = JudgePoints(BlueLedger, Index);
		const float RedPoints = JudgePoints(RedLedger, Index);
		const float Diff = BluePoints - RedPoints;
		FFBJudgeRow Row;
		Row.Name = Names[Index];
		if (FMath::Abs(Diff) < 1.6f)
		{
			Row.BlueScore = 10;
			Row.RedScore = 10;
			++Draws;
		}
		else if (Diff > 0.f)
		{
			Row.BlueScore = 10;
			Row.RedScore = ScoreLoser(BlueLedger, RedLedger, Diff);
			++BlueWins;
		}
		else
		{
			Row.RedScore = 10;
			Row.BlueScore = ScoreLoser(RedLedger, BlueLedger, -Diff);
			++RedWins;
		}
		Row.BlueScore = FMath::Max(6, Row.BlueScore - BlueLedger.PointDeductions);
		Row.RedScore = FMath::Max(6, Row.RedScore - RedLedger.PointDeductions);
		BuiltCard.BlueTotal += Row.BlueScore;
		BuiltCard.RedTotal += Row.RedScore;
		BuiltCard.Judges.Add(Row);
	}

	if (BlueLedger.PointDeductions > 0 || RedLedger.PointDeductions > 0)
	{
		BuiltCard.DeductionNote = FString::Printf(
			TEXT("DEDUCTED   BLUE -%d    RED -%d"),
			BlueLedger.PointDeductions,
			RedLedger.PointDeductions);
	}

	if (BlueWins == 3)
	{
		BuiltCard.Decision = TEXT("UNANIMOUS DECISION");
		BuiltCard.Winner = TEXT("BLUE CORNER");
	}
	else if (RedWins == 3)
	{
		BuiltCard.Decision = TEXT("UNANIMOUS DECISION");
		BuiltCard.Winner = TEXT("RED CORNER");
	}
	else if (BlueWins == 2 && RedWins == 1)
	{
		BuiltCard.Decision = TEXT("SPLIT DECISION");
		BuiltCard.Winner = TEXT("BLUE CORNER");
	}
	else if (RedWins == 2 && BlueWins == 1)
	{
		BuiltCard.Decision = TEXT("SPLIT DECISION");
		BuiltCard.Winner = TEXT("RED CORNER");
	}
	else if (BlueWins == 2 && Draws == 1)
	{
		BuiltCard.Decision = TEXT("MAJORITY DECISION");
		BuiltCard.Winner = TEXT("BLUE CORNER");
	}
	else if (RedWins == 2 && Draws == 1)
	{
		BuiltCard.Decision = TEXT("MAJORITY DECISION");
		BuiltCard.Winner = TEXT("RED CORNER");
	}
	else if (BuiltCard.BlueTotal > BuiltCard.RedTotal)
	{
		BuiltCard.Decision = TEXT("SPLIT DECISION");
		BuiltCard.Winner = TEXT("BLUE CORNER");
	}
	else if (BuiltCard.RedTotal > BuiltCard.BlueTotal)
	{
		BuiltCard.Decision = TEXT("SPLIT DECISION");
		BuiltCard.Winner = TEXT("RED CORNER");
	}
	else
	{
		BuiltCard.Decision = TEXT("DRAW");
		BuiltCard.Winner = TEXT("DRAW");
	}
}

void AFBSparringGameMode::BuildKnockoutCard(AFBBoxerCharacter* Winner)
{
	BuiltCard = FFBOfficialCard();
	BuiltCard.Result = EFBBoutResult::Knockout;
	BuiltCard.Decision = TEXT("KNOCKOUT");
	if (!Winner)
	{
		BuiltCard.Winner = TEXT("NO CONTEST");
		return;
	}
	BuiltCard.Winner = (Winner->CornerSkin == EFBCornerSkin::RedCorner) ? TEXT("RED CORNER") : TEXT("BLUE CORNER");
}

void AFBSparringGameMode::ShowReferee(AFBBoxerCharacter* Downed)
{
	HideReferee();
	if (!Downed || !GetWorld())
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Cube || !Sphere || !Cylinder)
	{
		return;
	}

	const FVector Forward = Downed->GetActorForwardVector().GetSafeNormal2D();
	FVector Feet = Downed->GetActorLocation();
	Feet.Z = 0.f;
	const FVector Stand = Feet + Forward * 165.f;

	auto Paint = [](UStaticMeshComponent* Mesh, const FLinearColor& Color)
	{
		if (!Mesh)
		{
			return;
		}
		UMaterialInterface* Base = Mesh->GetMaterial(0);
		if (!Base)
		{
			Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Base))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
		}
	};

	auto SpawnPart = [&](UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, AActor* Parent) -> AStaticMeshActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Loc, Forward.Rotation(), Params);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->Tags.Add(TEXT("FB_Referee"));
		if (UStaticMeshComponent* Comp = Actor->GetStaticMeshComponent())
		{
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetStaticMesh(Mesh);
			Comp->SetWorldScale3D(Scale);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Paint(Comp, Color);
		}
		if (Parent)
		{
			Actor->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
		}
		return Actor;
	};

	const FLinearColor Shirt(0.92f, 0.92f, 0.90f);
	const FLinearColor Slacks(0.08f, 0.08f, 0.09f);
	const FLinearColor Skin(0.62f, 0.45f, 0.34f);
	AStaticMeshActor* Torso = SpawnPart(Cube, Stand + FVector(0.f, 0.f, 118.f), FVector(0.42f, 0.28f, 0.55f), Shirt, nullptr);
	RefereeActor = Torso;
	SpawnPart(Cylinder, Stand + FVector(0.f, 0.f, 52.f), FVector(0.28f, 0.28f, 0.55f), Slacks, Torso);
	SpawnPart(Sphere, Stand + FVector(0.f, 0.f, 168.f), FVector(0.28f), Skin, Torso);
	SpawnPart(Cylinder, Stand + Forward * 18.f + FVector(0.f, 0.f, 125.f), FVector(0.08f, 0.08f, 0.42f), Shirt, Torso);
}

void AFBSparringGameMode::HideReferee()
{
	if (!GetWorld())
	{
		RefereeActor = nullptr;
		return;
	}
	TArray<AActor*> ToDestroy;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("FB_Referee")))
		{
			ToDestroy.Add(*It);
		}
	}
	for (AActor* Actor : ToDestroy)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	RefereeActor = nullptr;
}

void AFBSparringGameMode::SetRefereeCameras(bool bEnable, AFBBoxerCharacter* Downed)
{
	FVector Eye = FVector::ZeroVector;
	FVector Look = FVector::ZeroVector;
	if (bEnable && Downed)
	{
		const FVector Forward = Downed->GetActorForwardVector().GetSafeNormal2D();
		FVector Feet = Downed->GetActorLocation();
		Feet.Z = 0.f;
		Eye = Feet + Forward * 28.f + FVector(0.f, 0.f, 38.f);
		Look = Feet + Forward * 165.f + FVector(0.f, 0.f, 162.f);
	}

	for (TActorIterator<AFBBroadcastCamera> It(GetWorld()); It; ++It)
	{
		if (bEnable)
		{
			It->SetRefereeView(Eye, Look);
		}
		else
		{
			It->ClearRefereeView();
		}
	}
}

void AFBSparringGameMode::NotifyGuardBroken()
{
	++TelemetryGuardBreaks;
	SetRefereeCall(TEXT("GUARD BROKEN"), 1.4f);
}

void AFBSparringGameMode::RestoreBetweenRounds()
{
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		UFBCombatComponent* Combat = Boxer ? Boxer->GetCombatComponent() : nullptr;
		if (!Combat)
		{
			continue;
		}
		Combat->RestoreBetweenRound();
	}
}

void AFBSparringGameMode::AwardCareerFromCard()
{
	UFBFighterRosterSave* Roster = UFBFighterRosterSave::LoadOrCreate();
	if (!Roster)
	{
		return;
	}
	const bool bBlueWon = BuiltCard.Winner.Contains(TEXT("BLUE"));
	const bool bNoWinner = BuiltCard.Winner.Contains(TEXT("DRAW"))
		|| BuiltCard.Winner.Contains(TEXT("NO CONTEST"))
		|| BuiltCard.Winner.IsEmpty();
	bool bChanged = false;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (!*It || It->GetFighterDisplayName().IsEmpty() || It->GetFighterRecord().bPremade)
		{
			continue;
		}
		FFBFighterRecord* Saved = Roster->FindMutable(It->GetFighterRecord().Id);
		if (!Saved)
		{
			continue;
		}
		const bool bThisBlue = It->CornerSkin == EFBCornerSkin::BlueCorner;
		const bool bWon = !bNoWinner && ((bThisBlue && bBlueWon) || (!bThisBlue && !bBlueWon && !BuiltCard.Winner.IsEmpty()));
		Saved->CareerXP += 25;
		if (bNoWinner)
		{
			++Saved->Draws;
		}
		else if (bWon)
		{
			++Saved->Wins;
		}
		else
		{
			++Saved->Losses;
		}
		Saved->bFoughtOnce = true;
		Saved->RetrainBudget = FFBFighterBalance::RetrainCap;
		bChanged = true;
	}
	if (bChanged)
	{
		Roster->WriteToDisk();
	}
}

void AFBSparringGameMode::SetRefereeCall(const FString& Call, float Seconds)
{
	if (Call.IsEmpty())
	{
		return;
	}
	RefereeCall = Call;
	RefereeCallUntil = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + Seconds;
	PushBoutState();
}

void AFBSparringGameMode::DeductPoint(AFBBoxerCharacter* Boxer, const TCHAR* Reason)
{
	if (!Boxer || !Reason)
	{
		return;
	}
	LedgerFor(Boxer).PointDeductions += 1;
	const TCHAR* Corner = (Boxer->CornerSkin == EFBCornerSkin::RedCorner) ? TEXT("RED") : TEXT("BLUE");
	SetRefereeCall(FString::Printf(TEXT("POINT DEDUCTION — %s — %s"), Corner, Reason), 3.2f);
	UE_LOG(LogTemp, Log, TEXT("FinalBell point deduction %s (%s)"), Corner, Reason);
}

void AFBSparringGameMode::BreakClinch(const TCHAR* Call)
{
	if (AFBBoxerCharacter* A = ClinchA.Get())
	{
		A->AuthoritySetClinch(false);
	}
	if (AFBBoxerCharacter* B = ClinchB.Get())
	{
		B->AuthoritySetClinch(false);
	}
	ClinchA = nullptr;
	ClinchB = nullptr;
	ClinchInitiator = nullptr;
	ClinchSeconds = 0.f;
	if (Call && Call[0] != 0)
	{
		SetRefereeCall(Call);
	}
}

void AFBSparringGameMode::TickRingGeneralship(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	AFBBoxerCharacter* Blue = nullptr;
	AFBBoxerCharacter* Red = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (!*It)
		{
			continue;
		}
		if (It->CornerSkin == EFBCornerSkin::RedCorner)
		{
			Red = *It;
		}
		else if (!Blue)
		{
			Blue = *It;
		}
	}
	if (!Blue || !Red)
	{
		return;
	}

	const float Distance = FVector::Dist2D(Blue->GetActorLocation(), Red->GetActorLocation());
	FVector Center = FVector::ZeroVector;
	const bool bHaveCenter = FFBStylePolicy::FindRingCenter(GetWorld(), Center);
	const FFBStylePolicy BluePolicy = FFBStylePolicy::ForStyle(Blue->GetFighterRecord().Style);
	const FFBStylePolicy RedPolicy = FFBStylePolicy::ForStyle(Red->GetFighterRecord().Style);

	auto Award = [&](AFBBoxerCharacter* Self, AFBBoxerCharacter* Other, const FFBStylePolicy& Mine, const FFBStylePolicy& Theirs)
	{
		const float Range = FFBStylePolicy::RangeFit(Distance, Mine);
		const float OpponentRange = FFBStylePolicy::RangeFit(Distance, Theirs);
		const float Dictate = Range * (1.f - OpponentRange);
		float CenterScore = 0.f;
		if (bHaveCenter)
		{
			const float MyRadius = FVector::Dist2D(Self->GetActorLocation(), Center);
			const float OtherRadius = FVector::Dist2D(Other->GetActorLocation(), Center);
			CenterScore = FMath::Clamp((OtherRadius - MyRadius) / 200.f, 0.f, 1.f);
		}
		const float Sample = 0.45f * Range + 0.35f * Dictate + 0.20f * CenterScore;
		LedgerFor(Self).Generalship += DeltaSeconds * Sample;
	};

	Award(Blue, Red, BluePolicy, RedPolicy);
	Award(Red, Blue, RedPolicy, BluePolicy);
}

void AFBSparringGameMode::TickClinch(float DeltaSeconds)
{
	AFBBoxerCharacter* A = ClinchA.Get();
	AFBBoxerCharacter* B = ClinchB.Get();
	if (!A || !B)
	{
		return;
	}
	if (A->IsKnockedDown() || B->IsKnockedDown())
	{
		BreakClinch(TEXT(""));
		return;
	}

	ClinchSeconds += DeltaSeconds;
	if (ClinchSeconds >= 4.6f)
	{
		AFBBoxerCharacter* Who = ClinchInitiator.Get();
		const bool bExcessive = Who && Who->AuthorityClinchStarts() >= 2;
		BreakClinch(TEXT("BREAK"));
		if (bExcessive)
		{
			DeductPoint(Who, TEXT("EXCESSIVE CLINCHING"));
		}
		return;
	}

	FVector Dir = (B->GetActorLocation() - A->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		Dir = A->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Mid = (A->GetActorLocation() + B->GetActorLocation()) * 0.5f;
	A->SetActorLocation(FVector(Mid.X, Mid.Y, A->GetActorLocation().Z) - Dir * 48.f, false, nullptr, ETeleportType::TeleportPhysics);
	B->SetActorLocation(FVector(Mid.X, Mid.Y, B->GetActorLocation().Z) + Dir * 48.f, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = A->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (UCharacterMovementComponent* Move = B->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
}

void AFBSparringGameMode::AddClinchSteer(AFBBoxerCharacter* Boxer, float Axis)
{
	if (!Boxer || !AreClinching() || FMath::Abs(Axis) < 0.48f)
	{
		return;
	}
	if (Boxer != ClinchA.Get() && Boxer != ClinchB.Get())
	{
		return;
	}

	const float Step = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	Boxer->AuthorityAddClinchMeter(FMath::Abs(Axis) * 70.f * Step);
	if (Boxer->GetClinchTurnMeter() < 100.f)
	{
		return;
	}

	Boxer->AuthorityResetClinchMeter();
	AFBBoxerCharacter* A = ClinchA.Get();
	AFBBoxerCharacter* B = ClinchB.Get();
	if (!A || !B)
	{
		return;
	}
	FVector Dir = (B->GetActorLocation() - A->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		Dir = A->GetActorForwardVector().GetSafeNormal2D();
	}
	Dir = Dir.RotateAngleAxis(FMath::Sign(Axis) * 75.f, FVector::UpVector);
	const FVector Mid = (A->GetActorLocation() + B->GetActorLocation()) * 0.5f;
	A->SetActorLocation(FVector(Mid.X, Mid.Y, A->GetActorLocation().Z) - Dir * 48.f, false, nullptr, ETeleportType::TeleportPhysics);
	B->SetActorLocation(FVector(Mid.X, Mid.Y, B->GetActorLocation().Z) + Dir * 48.f, false, nullptr, ETeleportType::TeleportPhysics);
	SetRefereeCall(FString::Printf(TEXT("TURN  %d"), Boxer->GetClinchTurnsLanded()));
}

void AFBSparringGameMode::HandleRingAction(AFBBoxerCharacter* Boxer, const FFBCombatIntent& Intent, AActor* Opponent)
{
	if (!HasAuthority() || !Boxer || bResultFinalized || Boxer->IsKnockedDown())
	{
		return;
	}
	AFBBoxerCharacter* Other = Cast<AFBBoxerCharacter>(Opponent);

	switch (Intent.RingAction)
	{
	case EFBRingAction::StanceSwitch:
		Boxer->AuthorityToggleStance();
		break;
	case EFBRingAction::LooseToggle:
		Boxer->AuthorityToggleLoose();
		break;
	case EFBRingAction::Taunt:
		Boxer->AuthorityStartTaunt();
		break;
	case EFBRingAction::Slip:
		Boxer->AuthoritySlip(Intent.MoveAxis);
		break;
	case EFBRingAction::Push:
	{
		if (!Other)
		{
			break;
		}
		const float Dist = FVector::Dist2D(Boxer->GetActorLocation(), Other->GetActorLocation());
		if (Dist > 155.f)
		{
			break;
		}
		if (AreClinching())
		{
			BreakClinch(TEXT("BREAK"));
		}
		const FVector Away = (Other->GetActorLocation() - Boxer->GetActorLocation()).GetSafeNormal2D();
		Other->LaunchCharacter(Away * 520.f * Other->GetMoveSpeedScale() + FVector(0.f, 0.f, 40.f), true, true);
		break;
	}
	case EFBRingAction::Headbutt:
	case EFBRingAction::LowBlow:
	{
		if (!Other)
		{
			break;
		}
		const float Dist = FVector::Dist2D(Boxer->GetActorLocation(), Other->GetActorLocation());
		if (Dist > 130.f)
		{
			SetRefereeCall(TEXT("NO FOUL — OUT OF RANGE"));
			break;
		}
		const bool bLow = Intent.RingAction == EFBRingAction::LowBlow;
		Other->ApplyAuthoritativeDamage(bLow ? 9.f : 5.f, NextFoulId++, bLow ? EFBHitRegion::Body : EFBHitRegion::Head);
		const int32 Fouls = Boxer->AuthorityNoteFoul();
		if (Fouls >= 2)
		{
			DeductPoint(Boxer, bLow ? TEXT("LOW BLOW") : TEXT("HEADBUTT"));
		}
		else
		{
			SetRefereeCall(bLow ? TEXT("WARNING — LOW BLOW") : TEXT("WARNING — HEADBUTT"));
		}
		break;
	}
	case EFBRingAction::StartClinch:
	{
		if (!Other || AreClinching())
		{
			break;
		}
		const float Dist = FVector::Dist2D(Boxer->GetActorLocation(), Other->GetActorLocation());
		if (Dist > 165.f)
		{
			SetRefereeCall(TEXT("CLINCH MISSED"));
			break;
		}
		if (Boxer->AuthorityClinchStarts() >= 2)
		{
			DeductPoint(Boxer, TEXT("EXCESSIVE CLINCHING"));
			break;
		}
		Boxer->AuthorityClinchStarts() += 1;
		ClinchA = Boxer;
		ClinchB = Other;
		ClinchInitiator = Boxer;
		ClinchSeconds = 0.f;
		Boxer->AuthoritySetClinch(true);
		Other->AuthoritySetClinch(true);
		SetRefereeCall(TEXT("CLINCH"));
		break;
	}
	default:
		break;
	}
}
