#include "Combat/FBBoxerAIController.h"

#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBFighterBalance.h"
#include "Combat/FBSparringGameMode.h"
#include "Combat/FBStylePolicy.h"
#include "Combat/FBTacticalBrain.h"
#include "Ring/FBRingBounds.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDebugAI(
	TEXT("fb.DebugAI"),
	0,
	TEXT("1 prints tactical AI state, style, difficulty, energy, range, scores, and the intent result. 0 is silent. This is not the bout HUD."));

namespace
{
	AFBBoxerCharacter* OtherBoxer(const AFBBoxerCharacter* Self)
	{
		if (!Self)
		{
			return nullptr;
		}
		const bool bSelfIsOpponent = Self->ActorHasTag(TEXT("FB_P1Opponent"));
		AFBBoxerCharacter* Fallback = nullptr;
		for (TActorIterator<AFBBoxerCharacter> It(Self->GetWorld()); It; ++It)
		{
			AFBBoxerCharacter* Other = *It;
			if (!Other || Other == Self)
			{
				continue;
			}
			const bool bOtherIsOpponent = Other->ActorHasTag(TEXT("FB_P1Opponent"));
			if (bSelfIsOpponent != bOtherIsOpponent)
			{
				return Other;
			}
			if (!Fallback)
			{
				Fallback = Other;
			}
		}
		return Fallback;
	}

	int32 ReadRating(const AFBBoxerCharacter* Boxer, EFBAttributeId Id)
	{
		const TArray<int32>& Ratings = Boxer->GetFighterRecord().Ratings;
		const int32 Index = static_cast<int32>(Id);
		return Ratings.IsValidIndex(Index) ? Ratings[Index] : FFBFighterBalance::RatingMin;
	}

	void FillCosts(const UFBCombatComponent* Combat, FFBTacticalView& View)
	{
		const EFBAttackId Ids[] = {
			EFBAttackId::Jab, EFBAttackId::Cross, EFBAttackId::LeadHook,
			EFBAttackId::RearHook, EFBAttackId::LeadUppercut, EFBAttackId::RearUppercut
		};
		for (EFBAttackId Id : Ids)
		{
			const int32 Index = static_cast<int32>(Id);
			if (Combat && Index >= 0 && Index < UE_ARRAY_COUNT(View.AttackCost))
			{
				View.AttackCost[Index] = Combat->GetAttackCost(Id);
			}
		}
	}
}

AFBBoxerAIController::AFBBoxerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
}

void AFBBoxerAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetTactics(TEXT("EndPlay"));
	delete Brain;
	Brain = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AFBBoxerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NextSequence = 1;
	if (!Brain)
	{
		Brain = new FFBTacticalBrain();
	}
	Brain->SetTuning(Tuning);
	Brain->SetDifficulty(EFBAIDifficulty::Club);
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(InPawn))
	{
		const FString Style = Boxer->GetFighterRecord().Style;
		const int32 Seed = static_cast<int32>(GetTypeHash(Style.IsEmpty() ? Boxer->GetFighterDisplayName() : Style) & 0x7fffffff);
		if (Brain)
		{
			Brain->SetSeed(Seed == 0 ? 1 : Seed);
			Brain->SetStyle(FFBStylePolicy::ForStyle(Style), Boxer->Stance);
			Brain->SetTuning(Tuning);
		}
		UE_LOG(LogTemp, Log, TEXT("FinalBell AI %s fights as %s."),
			*Boxer->GetFighterDisplayName(),
			Style.IsEmpty() ? TEXT("a mix") : *Style);
	}
	PushDifficultyFromMode();
	ResetTactics(TEXT("Possess"));
}

void AFBBoxerAIController::OnUnPossess()
{
	ResetTactics(TEXT("Unpossess"));
	Super::OnUnPossess();
}

void AFBBoxerAIController::SetDifficulty(EFBAIDifficulty Difficulty)
{
	if (!Brain)
	{
		Brain = new FFBTacticalBrain();
		Brain->SetSeed(1);
	}
	Brain->SetTuning(Tuning);
	Brain->SetDifficulty(Difficulty);
}

void AFBBoxerAIController::PushDifficultyFromMode()
{
	const UWorld* World = GetWorld();
	const AFBSparringGameMode* Mode = World ? World->GetAuthGameMode<AFBSparringGameMode>() : nullptr;
	SetDifficulty(Mode ? Mode->GetAiDifficulty() : EFBAIDifficulty::Club);
}

void AFBBoxerAIController::ResetTactics(const TCHAR* Reason)
{
	UnbindAll();
	DecisionLeft = 0.f;
	FailTime = 0.f;
	LastMoveBucket = INDEX_NONE;
	SmoothedMove = FVector::ZeroVector;
	MoveGoal = FVector::ZeroVector;
	bRoundLatched = false;
	bDownLatched = false;
	LastIntentText = TEXT("none");
	if (Brain)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		Brain->Reset(Reason, Now);
		Brain->SetTuning(Tuning);
	}
	if (const AFBBoxerCharacter* Self = Cast<AFBBoxerCharacter>(GetPawn()))
	{
		LastLocation = Self->GetActorLocation();
	}
}

void AFBBoxerAIController::UnbindAll()
{
	if (UFBCombatComponent* Combat = BoundOpponentCombat.Get())
	{
		Combat->OnAttackPhaseChanged.RemoveDynamic(this, &AFBBoxerAIController::HandleOpponentPhase);
		Combat->OnImpactResolved.RemoveDynamic(this, &AFBBoxerAIController::HandleOpponentImpact);
	}
	if (UFBCombatComponent* Combat = BoundSelfCombat.Get())
	{
		Combat->OnImpactResolved.RemoveDynamic(this, &AFBBoxerAIController::HandleOwnImpact);
	}
	BoundOpponent = nullptr;
	BoundOpponentCombat = nullptr;
	BoundSelfCombat = nullptr;
}

void AFBBoxerAIController::BindPair(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent)
{
	if (!Self || !Opponent || !Brain)
	{
		return;
	}
	if (BoundOpponent.Get() == Opponent && BoundSelfCombat.Get() == Self->GetCombatComponent())
	{
		return;
	}
	UnbindAll();
	Brain->Reset(TEXT("Opponent"), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	Brain->SetStyle(FFBStylePolicy::ForStyle(Self->GetFighterRecord().Style), Self->Stance);
	BoundOpponent = Opponent;
	if (UFBCombatComponent* OppCombat = Opponent->GetCombatComponent())
	{
		BoundOpponentCombat = OppCombat;
		OppCombat->OnAttackPhaseChanged.AddDynamic(this, &AFBBoxerAIController::HandleOpponentPhase);
		OppCombat->OnImpactResolved.AddDynamic(this, &AFBBoxerAIController::HandleOpponentImpact);
	}
	if (UFBCombatComponent* SelfCombat = Self->GetCombatComponent())
	{
		BoundSelfCombat = SelfCombat;
		SelfCombat->OnImpactResolved.AddDynamic(this, &AFBBoxerAIController::HandleOwnImpact);
	}
}

void AFBBoxerAIController::HandleOpponentPhase(EFBAttackPhase Phase)
{
	if (!Brain || Phase != EFBAttackPhase::Startup)
	{
		return;
	}
	AFBBoxerCharacter* Self = Cast<AFBBoxerCharacter>(GetPawn());
	AFBBoxerCharacter* Opponent = BoundOpponent.Get();
	UFBCombatComponent* OppCombat = BoundOpponentCombat.Get();
	if (!Self || !Opponent || !OppCombat)
	{
		return;
	}
	const FVector To = (Opponent->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal2D();
	const float Facing = FVector::DotProduct(Self->GetActorForwardVector().GetSafeNormal2D(), To);
	const float Distance = FVector::Dist2D(Self->GetActorLocation(), Opponent->GetActorLocation());
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Brain->NoteAttackCue(
		Opponent->GetUniqueID(),
		OppCombat->GetActiveAttackInstanceId(),
		OppCombat->GetActiveAttack(),
		OppCombat->IsBodyShot(),
		OppCombat->IsPowerShot(),
		Now,
		Facing,
		Distance);
}

void AFBBoxerAIController::HandleOpponentImpact(const FFBImpactEvent& Impact)
{
	AFBBoxerCharacter* Opponent = BoundOpponent.Get();
	if (!Brain || !Opponent)
	{
		return;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Brain->NoteImpact(Opponent->GetUniqueID(), Impact.AttackInstanceId, Impact, Now);
}

void AFBBoxerAIController::HandleOwnImpact(const FFBImpactEvent& Impact)
{
	if (!Brain)
	{
		return;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Brain->NoteOwnImpact(Impact, Now);
}

EFBIntentResult AFBBoxerAIController::Submit(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBActionChoice& Action)
{
	UFBCombatComponent* Combat = Self ? Self->GetCombatComponent() : nullptr;
	if (!Self || !Combat || !Brain || !Action.bSubmit || !Self->HasAuthority() || Self->IsKnockedDown())
	{
		return EFBIntentResult::RejectedPhase;
	}
	if (Opponent && Opponent->IsKnockedDown() && Action.Attack != EFBAttackId::None)
	{
		return EFBIntentResult::RejectedDown;
	}
	FFBCombatIntent Intent;
	Intent.Sequence = NextSequence++;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = Action.Attack;
	Intent.bGuardHeld = Action.bGuardHeld;
	Intent.Guard = Action.Guard;
	Intent.SchemaVersion = TEXT("1.0.0");
	Intent.bBodyTarget = Action.bBody;
	Intent.bPowerPunch = Action.bPower;
	Intent.RingAction = Action.RingAction;
	Intent.MoveAxis = Action.MoveAxis;
	if (Action.Attack != EFBAttackId::None)
	{
		const float Accuracy = FFBFighterBalance::Normalized(ReadRating(Self, EFBAttributeId::Accuracy));
		const int32 Roll = static_cast<int32>(GetTypeHash(Intent.Sequence) % 100);
		const int32 Need = FMath::RoundToInt(FMath::Lerp(Brain->GetProfile().ErrorRate < 0.2f ? 20.f : 12.f, Brain->GetProfile().ErrorRate < 0.2f ? 70.f : 55.f, Accuracy));
		Intent.bPrecisionCone = Roll < Need;
	}
	const EFBIntentResult Result = Combat->ServerTryProcessIntent(Intent, Opponent);
	Brain->NoteIntent(Result, Action.Kind);
	LastIntentText = FString::Printf(TEXT("%s -> %d"), *Action.Label, static_cast<int32>(Result));
	return Result;
}

void AFBBoxerAIController::ApplyFootwork(AFBBoxerCharacter* Self, const FVector& Desired, bool bScaleDown)
{
	if (!Self || Self->IsKnockedDown() || Self->IsInClinch())
	{
		return;
	}
	FVector Direction = FVector::ZeroVector;
	float Intensity = 0.f;
	FFBTactical::SeparateFootwork(Desired, Direction, Intensity);
	if (bScaleDown)
	{
		Intensity *= Tuning.CommittedMoveScale;
	}
	if (Intensity > 0.02f && !Direction.IsNearlyZero())
	{
		Self->AddMovementInput(Direction, Intensity);
	}
}

void AFBBoxerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Brain)
	{
		return;
	}

	AFBBoxerCharacter* Self = Cast<AFBBoxerCharacter>(GetPawn());
	if (!Self || !Self->HasAuthority() || Self->IsPlayerControlled())
	{
		return;
	}

	UWorld* World = GetWorld();
	AFBSparringGameMode* Mode = World ? World->GetAuthGameMode<AFBSparringGameMode>() : nullptr;
	if (!Mode)
	{
		return;
	}
	const EFBPlayMode PlayMode = Mode->GetPlayMode();
	if (PlayMode != EFBPlayMode::VersusAI && PlayMode != EFBPlayMode::Career)
	{
		if (BoundOpponentCombat.IsValid())
		{
			ResetTactics(TEXT("Mode"));
		}
		return;
	}
	if (Mode->IsPresentationPaused() || Mode->GetPhase() != EFBBoutPhase::RoundActive)
	{
		if (bRoundLatched)
		{
			ResetTactics(Mode->IsPresentationPaused() ? TEXT("Paused") : TEXT("Phase"));
		}
		bRoundLatched = false;
		return;
	}
	if (!bRoundLatched)
	{
		bRoundLatched = true;
		PushDifficultyFromMode();
		Brain->SetStyle(FFBStylePolicy::ForStyle(Self->GetFighterRecord().Style), Self->Stance);
		Brain->SetTuning(Tuning);
		Brain->Reset(TEXT("Bell"), World->GetTimeSeconds());
		SmoothedMove = FVector::ZeroVector;
		DecisionLeft = 0.f;
	}

	AFBBoxerCharacter* Opponent = OtherBoxer(Self);
	if (!Opponent || Opponent == Self)
	{
		if (BoundOpponentCombat.IsValid())
		{
			ResetTactics(TEXT("NoOpponent"));
			bRoundLatched = true;
		}
		return;
	}
	BindPair(Self, Opponent);

	UFBCombatComponent* SelfCombat = Self->GetCombatComponent();
	UFBCombatComponent* OppCombat = Opponent->GetCombatComponent();
	if (!SelfCombat || !OppCombat)
	{
		return;
	}

	if (Self->IsKnockedDown())
	{
		if (!bDownLatched)
		{
			bDownLatched = true;
			UnbindAll();
			Brain->Reset(TEXT("Knockdown"), World->GetTimeSeconds());
			Brain->SetStyle(FFBStylePolicy::ForStyle(Self->GetFighterRecord().Style), Self->Stance);
			Brain->SetTuning(Tuning);
			SmoothedMove = FVector::ZeroVector;
			MoveGoal = FVector::ZeroVector;
		}
	}
	else
	{
		bDownLatched = false;
	}

	const float Now = World->GetTimeSeconds();
	const FVector MyLoc = Self->GetActorLocation();
	const FVector OppLoc = Opponent->GetActorLocation();
	const FVector ToOpp = (OppLoc - MyLoc).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToOpp).GetSafeNormal2D();
	float Clearance = 1000.f;
	FVector Safer = FVector::ZeroVector;
	for (TActorIterator<AFBRingBounds> It(World); It; ++It)
	{
		const float Radius = Self->GetCapsuleComponent() ? Self->GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;
		It->QueryFootwork(MyLoc, Radius, Clearance, Safer);
		break;
	}

	FFBTacticalView View;
	View.Now = Now;
	View.bHasOpponent = true;
	View.bAiMode = true;
	View.bRoundActive = true;
	View.bPaused = false;
	View.bKnockedDown = Self->IsKnockedDown();
	View.bOpponentDown = Opponent->IsKnockedDown();
	View.bClinch = Self->IsInClinch();
	View.bAttackCommitted = SelfCombat->IsAttackCommitted();
	View.bComboBufferOpen = SelfCombat->IsComboBufferOpen();
	View.bFacingLocked = Self->IsHookFacingLocked();
	View.Distance = FVector::Dist2D(MyLoc, OppLoc);
	View.FacingDot = FVector::DotProduct(Self->GetActorForwardVector().GetSafeNormal2D(), ToOpp);
	const FVector Relative = Opponent->GetVelocity() - Self->GetVelocity();
	View.ClosingSpeed = ToOpp.IsNearlyZero() ? 0.f : -FVector::DotProduct(Relative, ToOpp);
	View.Energy = SelfCombat->GetBurstEnergy();
	View.EnergyMax = SelfCombat->MaxBurstEnergy;
	View.Fatigue = SelfCombat->GetFatigue();
	View.Ceiling = SelfCombat->GetRecoverableCeiling();
	View.BlockHealth = Self->GetBlockHealth();
	View.BlockMax = Self->GetBlockHealthMax();
	View.Stun = Self->GetStun();
	View.JabCost = SelfCombat->GetAttackCost(EFBAttackId::Jab);
	View.PunchRegenDelay = SelfCombat->GetPunchRegenDelay();
	View.MinReach = SelfCombat->MinReachUU;
	View.MaxReach = SelfCombat->MaxReachUU;
	View.StartupSeconds = OppCombat->StartupSeconds;
	View.RopeClearance = Clearance;
	View.OwnGuard = SelfCombat->GetGuardState();
	View.SeenGuard = OppCombat->GetGuardState();
	View.ToOpponent = ToOpp;
	View.OpponentVelocity = Opponent->GetVelocity();
	View.SaferDirection = Safer;
	View.Right = Right;
	View.bLastMoveFailed = FailTime >= Tuning.MoveFailSeconds;
	View.FailedMoveBucket = View.bLastMoveFailed ? LastMoveBucket : INDEX_NONE;
	FillCosts(SelfCombat, View);

	FFBActionChoice Buffered;
	if (Brain->TryBufferBeat(View, Buffered))
	{
		Submit(Self, Opponent, Buffered);
		View.bAttackCommitted = SelfCombat->IsAttackCommitted();
		View.bComboBufferOpen = SelfCombat->IsComboBufferOpen();
		View.Energy = SelfCombat->GetBurstEnergy();
	}

	FFBActionChoice Reaction;
	if (!Self->IsKnockedDown() && !Self->IsInClinch() && Brain->ConsumeReadyDefense(Now, OppCombat->StartupSeconds, Reaction))
	{
		if (Reaction.Attack == EFBAttackId::None)
		{
			Submit(Self, Opponent, Reaction);
		}
	}

	DecisionLeft -= DeltaSeconds;
	FFBTacticalDecision Decision;
	const bool bDecide = DecisionLeft <= 0.f;
	if (bDecide)
	{
		Decision = Brain->Decide(View);
		const float Interval = FFBTactical::ClampDecisionInterval(Brain->GetProfile().DecisionInterval, Tuning);
		DecisionLeft = Interval;
		MoveGoal = Decision.MoveDesired;
		if (Decision.Action.bSubmit && Decision.Action.Attack == EFBAttackId::None)
		{
			Submit(Self, Opponent, Decision.Action);
		}
		else if (Decision.Action.bSubmit && Decision.Action.Attack != EFBAttackId::None && (!View.bAttackCommitted || View.bComboBufferOpen))
		{
			Submit(Self, Opponent, Decision.Action);
		}
		LastMoveBucket = Decision.MoveBucket;
	}
	SmoothedMove = FMath::VInterpTo(SmoothedMove, MoveGoal, DeltaSeconds, Tuning.MoveEase);

	const bool bCommittedMove = SelfCombat->IsAttackCommitted() && !SelfCombat->IsComboBufferOpen();
	ApplyFootwork(Self, SmoothedMove, bCommittedMove || Self->IsHookFacingLocked());

	const float Moved = FVector::Dist2D(MyLoc, LastLocation);
	FVector Dir = FVector::ZeroVector;
	float Intensity = 0.f;
	FFBTactical::SeparateFootwork(SmoothedMove, Dir, Intensity);
	if (Intensity > 0.45f && Moved < 6.f * DeltaSeconds && !Self->IsKnockedDown() && !Self->IsInClinch())
	{
		FailTime += DeltaSeconds;
	}
	else
	{
		FailTime = 0.f;
	}
	LastLocation = Self->GetActorLocation();

	if (CVarDebugAI.GetValueOnGameThread() != 0 && (bDecide || Brain->GetState() != LastLoggedState) && Now - LastDebugTime > 0.45f)
	{
		LastDebugTime = Now;
		LastLoggedState = Brain->GetState();
		if (!bDecide)
		{
			Decision.State = Brain->GetState();
			Decision.Reason = Brain->GetReason();
			Decision.MoveDesired = SmoothedMove;
		}
		const FString Line = Brain->DebugLine(View, Decision) + TEXT(" | intent ") + LastIntentText;
		UE_LOG(LogTemp, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(0x464241, 1.2f, FColor::Silver, Line);
		}
	}
}
