#include "Combat/FBBoxerAIController.h"
#include "Combat/FBBoxerCharacter.h"
#include "Math/RotationMatrix.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBFighterBalance.h"
#include "Combat/FBSparringGameMode.h"
#include "Combat/FBStylePolicy.h"
#include "EngineUtils.h"

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
}

namespace
{
	int32 ReadRating(const AFBBoxerCharacter* Boxer, EFBAttributeId Id)
	{
		const TArray<int32>& Ratings = Boxer->GetFighterRecord().Ratings;
		const int32 Index = static_cast<int32>(Id);
		return Ratings.IsValidIndex(Index) ? Ratings[Index] : FFBFighterBalance::RatingMin;
	}
}

AFBBoxerAIController::AFBBoxerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
}

void AFBBoxerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NextSequence = 1;
	ThinkDelay = 0.55f;
	AnswerUntil = -1.f;
	PrevOppPhase = EFBAttackPhase::Idle;
	bRoundLatched = false;

	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(InPawn))
	{
		const FString Style = Boxer->GetFighterRecord().Style;
		UE_LOG(LogTemp, Log, TEXT("FinalBell AI %s fights as %s."),
			*Boxer->GetFighterDisplayName(),
			Style.IsEmpty() ? TEXT("a mix") : *Style);
	}
}

void AFBBoxerAIController::Offer(
	AFBBoxerCharacter* Self,
	AFBBoxerCharacter* Opponent,
	EFBAttackId Attack,
	bool bGuardHeld,
	EFBGuardState Guard,
	bool bBody,
	bool bPower,
	EFBRingAction Action,
	FVector2D MoveAxis)
{
	UFBCombatComponent* Combat = Self ? Self->GetCombatComponent() : nullptr;
	if (!Self || !Combat || !Self->HasAuthority() || Self->IsKnockedDown())
	{
		return;
	}
	if (Opponent && Opponent->IsKnockedDown() && Attack != EFBAttackId::None)
	{
		return;
	}

	FFBCombatIntent Intent;
	Intent.Sequence = NextSequence++;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = Attack;
	Intent.bGuardHeld = bGuardHeld;
	Intent.Guard = Guard;
	Intent.SchemaVersion = TEXT("1.0.0");
	Intent.bBodyTarget = bBody;
	Intent.bPowerPunch = bPower;
	Intent.RingAction = Action;
	Intent.MoveAxis = MoveAxis;
	if (Attack != EFBAttackId::None)
	{
		const float Accuracy = FFBFighterBalance::Normalized(ReadRating(Self, EFBAttributeId::Accuracy));
		Intent.bPrecisionCone = FMath::FRand() < FMath::Lerp(0.12f, 0.82f, Accuracy);
	}
	Combat->ServerTryProcessIntent(Intent, Opponent);
}

void AFBBoxerAIController::StepFeet(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy) const
{
	if (!Self || !Opponent || Self->IsKnockedDown() || Self->IsInClinch())
	{
		return;
	}

	const FVector MyLoc = Self->GetActorLocation();
	const FVector OppLoc = Opponent->GetActorLocation();
	const FVector ToOpp = (OppLoc - MyLoc).GetSafeNormal2D();
	if (ToOpp.IsNearlyZero())
	{
		return;
	}

	if (Opponent->IsKnockedDown())
	{
		const float Dist = FVector::Dist2D(MyLoc, OppLoc);
		if (Dist < 170.f)
		{
			Self->AddMovementInput(-ToOpp, 1.f);
		}
		return;
	}

	const float Dist = FVector::Dist2D(MyLoc, OppLoc);
	const FVector Right = FRotationMatrix(ToOpp.Rotation()).GetUnitAxis(EAxis::Y);
	FVector Desired = FVector::ZeroVector;

	if (Dist > Policy.RangeMax)
	{
		Desired += ToOpp * Policy.Chase;
	}
	else if (Dist < Policy.RangeMin)
	{
		const float Back = Policy.Shell >= 0.5f ? 0.35f : 1.f;
		Desired -= ToOpp * Back;
	}

	if (Policy.Circle > 0.f)
	{
		Desired += Right * Policy.Circle;
	}
	if (Policy.SouthpawStep > 0.f)
	{
		Desired -= Right * Policy.SouthpawStep;
	}

	FVector Center = FVector::ZeroVector;
	if (Policy.CutRing > 0.f && FFBStylePolicy::FindRingCenter(GetWorld(), Center))
	{
		const FVector Off = OppLoc - Center;
		const float Radius = Off.Size2D();
		if (Radius > 50.f)
		{
			const FVector Dir = Off.GetSafeNormal2D();
			const FVector CutPoint = OppLoc - Dir * FMath::Min((Policy.RangeMin + Policy.RangeMax) * 0.5f, Radius);
			Desired += (CutPoint - MyLoc).GetSafeNormal2D() * Policy.CutRing;
		}
	}
	else if (Policy.Circle > 0.2f && FFBStylePolicy::FindRingCenter(GetWorld(), Center))
	{
		const float MyRadius = FVector::Dist2D(MyLoc, Center);
		if (MyRadius > 260.f)
		{
			Desired += (Center - MyLoc).GetSafeNormal2D() * 0.8f;
		}
	}

	if (!Desired.IsNearlyZero())
	{
		Self->AddMovementInput(Desired.GetSafeNormal2D(), 1.f);
	}
}

void AFBBoxerAIController::ReactToPunch(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy)
{
	UFBCombatComponent* SelfCombat = Self->GetCombatComponent();
	UFBCombatComponent* OppCombat = Opponent ? Opponent->GetCombatComponent() : nullptr;
	if (!SelfCombat || !OppCombat || SelfCombat->IsAttackCommitted())
	{
		return;
	}

	const EFBAttackId Incoming = OppCombat->GetActiveAttack();
	const bool bHook = Incoming == EFBAttackId::LeadHook || Incoming == EFBAttackId::RearHook;
	const float Reflex = FFBFighterBalance::Normalized(ReadRating(Self, EFBAttributeId::Reflexes));
	const float Guard = FFBFighterBalance::Normalized(ReadRating(Self, EFBAttributeId::Guarding));

	if (Policy.Shell >= 0.5f)
	{
		Offer(Self, Opponent, EFBAttackId::None, true, EFBGuardState::High, false, false, EFBRingAction::None, FVector2D::ZeroVector);
		return;
	}

	if (bHook && Reflex > 0.45f && Policy.Slip > 0.15f)
	{
		Offer(Self, Opponent, EFBAttackId::None, true, EFBGuardState::Weave, false, false, EFBRingAction::None, FVector2D::ZeroVector);
		Self->BeginDefenseSlip(FVector2D(0.f, 1.f));
		return;
	}

	if (Policy.Slip > 0.f && Reflex > 0.25f && FMath::FRand() < Policy.Slip * 0.55f)
	{
		if (FMath::FRand() < 0.5f)
		{
			const float Side = FMath::FRand() < 0.5f ? -1.f : 1.f;
			Offer(Self, Opponent, EFBAttackId::None, false, EFBGuardState::Open, false, false, EFBRingAction::Slip, FVector2D(Side, 0.f));
		}
		else
		{
			Offer(Self, Opponent, EFBAttackId::None, false, EFBGuardState::Open, false, false, EFBRingAction::Slip, FVector2D(0.f, -1.f));
		}
		return;
	}

	// A low guard rating is the weakness. They only shell when the rating can hold one.
	if (Guard > 0.55f && FMath::FRand() < 0.22f)
	{
		Offer(Self, Opponent, EFBAttackId::None, true, EFBGuardState::High, false, false, EFBRingAction::None, FVector2D::ZeroVector);
	}
}

void AFBBoxerAIController::Throw(AFBBoxerCharacter* Self, AFBBoxerCharacter* Opponent, const FFBStylePolicy& Policy)
{
	struct FChoice
	{
		EFBAttackId Id;
		float Weight;
	};
	const FChoice Choices[] = {
		{ EFBAttackId::Jab, Policy.Jab },
		{ EFBAttackId::Cross, Policy.Straight },
		{ EFBAttackId::LeadHook, Policy.LeadHook },
		{ EFBAttackId::RearHook, Policy.RearHook },
		{ EFBAttackId::LeadUppercut, Policy.LeadUppercut },
		{ EFBAttackId::RearUppercut, Policy.RearUppercut },
	};

	float Weights[UE_ARRAY_COUNT(Choices)];
	float Sum = 0.f;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Choices); ++Index)
	{
		Weights[Index] = FMath::Max(0.f, Choices[Index].Weight) * Self->GetHandPower(Choices[Index].Id);
		Sum += Weights[Index];
	}

	EFBAttackId Attack = EFBAttackId::Jab;
	if (Sum > 0.f)
	{
		float Roll = FMath::FRand() * Sum;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Choices); ++Index)
		{
			Roll -= Weights[Index];
			if (Roll <= 0.f)
			{
				Attack = Choices[Index].Id;
				break;
			}
		}
	}

	UFBCombatComponent* Combat = Self->GetCombatComponent();
	const float Energy = Combat ? Combat->GetBurstEnergy() : 0.f;
	bool bPower = Policy.PowerChance > 0.f && Energy >= 28.f && FMath::FRand() < Policy.PowerChance;
	const bool bBody = Policy.BodyChance > 0.f && FMath::FRand() < Policy.BodyChance;
	const bool bShell = Policy.Shell >= 0.5f;
	Offer(Self, Opponent, Attack, bShell, bShell ? EFBGuardState::High : EFBGuardState::Open, bBody, bPower, EFBRingAction::None, FVector2D::ZeroVector);
}

void AFBBoxerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
		return;
	}
	if (Mode->IsPresentationPaused() || Mode->GetPhase() != EFBBoutPhase::RoundActive)
	{
		bRoundLatched = false;
		return;
	}
	if (!bRoundLatched)
	{
		bRoundLatched = true;
		ThinkDelay = 0.6f;
		AnswerUntil = -1.f;
		PrevOppPhase = EFBAttackPhase::Idle;
	}

	AFBBoxerCharacter* Opponent = OtherBoxer(Self);
	if (!Opponent || Opponent == Self)
	{
		return;
	}

	const FFBStylePolicy Policy = FFBStylePolicy::ForStyle(Self->GetFighterRecord().Style);
	UFBCombatComponent* SelfCombat = Self->GetCombatComponent();
	UFBCombatComponent* OppCombat = Opponent->GetCombatComponent();
	if (!SelfCombat || !OppCombat)
	{
		return;
	}

	StepFeet(Self, Opponent, Policy);

	if (Self->IsKnockedDown() || Self->IsInClinch() || Opponent->IsKnockedDown())
	{
		PrevOppPhase = OppCombat->GetAttackPhase();
		return;
	}

	const EFBAttackPhase OppPhase = OppCombat->GetAttackPhase();
	const float Now = World->GetTimeSeconds();
	if (PrevOppPhase == EFBAttackPhase::Idle && (OppPhase == EFBAttackPhase::Startup || OppPhase == EFBAttackPhase::Active))
	{
		ReactToPunch(Self, Opponent, Policy);
	}
	if (PrevOppPhase == EFBAttackPhase::Active && OppPhase != EFBAttackPhase::Active)
	{
		const EFBContactResult Result = OppCombat->GetLastImpact().Result;
		if (Result == EFBContactResult::Miss || Result == EFBContactResult::Blocked)
		{
			AnswerUntil = Now + 0.55f;
		}
	}
	PrevOppPhase = OppPhase;

	const bool bIncoming = OppPhase == EFBAttackPhase::Startup || OppPhase == EFBAttackPhase::Active;
	if (Policy.Shell >= 0.5f && bIncoming)
	{
		return;
	}

	ThinkDelay -= DeltaSeconds;
	if (ThinkDelay > 0.f || SelfCombat->IsAttackCommitted())
	{
		return;
	}

	const bool bWalksThrough = Policy.CutRing >= 0.5f || Policy.RangeMax <= 125.f;
	if (bIncoming && !bWalksThrough)
	{
		ThinkDelay = 0.12f;
		return;
	}

	const float Energy = SelfCombat->GetBurstEnergy();
	const float MaxEnergy = FMath::Max(1.f, SelfCombat->MaxBurstEnergy);
	const float RestAt = Policy.RestFraction * Self->GetCombatDerived().EnduranceCost;
	if (Energy < 14.f || Energy / MaxEnergy < RestAt)
	{
		ThinkDelay = 0.25f;
		if (Policy.Shell >= 0.5f || Energy < 14.f)
		{
			Offer(Self, Opponent, EFBAttackId::None, true, EFBGuardState::High, false, false, EFBRingAction::None, FVector2D::ZeroVector);
		}
		return;
	}

	const bool bAnswering = Now <= AnswerUntil;
	if (!bAnswering && Policy.WaitForMiss > 0.f && FMath::FRand() < Policy.WaitForMiss)
	{
		ThinkDelay = 0.28f;
		if (Policy.Shell >= 0.5f && SelfCombat->GetGuardState() != EFBGuardState::High)
		{
			Offer(Self, Opponent, EFBAttackId::None, true, EFBGuardState::High, false, false, EFBRingAction::None, FVector2D::ZeroVector);
		}
		return;
	}

	const float Dist = FVector::Dist2D(Self->GetActorLocation(), Opponent->GetActorLocation());
	const bool bInBand = Dist >= Policy.RangeMin && Dist <= Policy.RangeMax;
	const bool bAnswerRange = Dist >= 80.f && Dist <= 210.f;
	if (bAnswering)
	{
		if (!bAnswerRange)
		{
			ThinkDelay = 0.12f;
			return;
		}
		AnswerUntil = -1.f;
	}
	else if (!bInBand)
	{
		ThinkDelay = 0.12f;
		return;
	}

	const float Speed = FMath::Max(0.5f, Self->GetCombatDerived().PunchSpeed);
	ThinkDelay = Policy.SecondsBetweenPunches / Speed;
	Throw(Self, Opponent, Policy);
}
