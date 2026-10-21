#include "Combat/FBCombatComponent.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBSparringGameMode.h"
#include "Training/FBHeavyBag.h"
#include "Net/UnrealNetwork.h"

UFBCombatComponent::UFBCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFBCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFBCombatComponent, BurstEnergy);
	DOREPLIFETIME(UFBCombatComponent, Fatigue);
	DOREPLIFETIME(UFBCombatComponent, GuardState);
	DOREPLIFETIME(UFBCombatComponent, ActiveAttack);
	DOREPLIFETIME(UFBCombatComponent, AttackPhase);
	DOREPLIFETIME(UFBCombatComponent, bActiveBodyShot);
	DOREPLIFETIME(UFBCombatComponent, bActivePowerShot);
	DOREPLIFETIME(UFBCombatComponent, LastProcessedSequence);
	DOREPLIFETIME(UFBCombatComponent, LastImpact);
}

void UFBCombatComponent::OnRep_CombatVisual()
{
	OnAttackPhaseChanged.Broadcast(AttackPhase);
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Boxer->RefreshCombatPresentation();
	}
}

void UFBCombatComponent::OnRep_LastImpact()
{
	OnImpactResolved.Broadcast(LastImpact);
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Boxer->HandleImpactPresentation(LastImpact);
	}
}

void UFBCombatComponent::SetPhase(EFBAttackPhase NewPhase, float Duration)
{
	AttackPhase = NewPhase;
	PhaseTimeRemaining = Duration;
	OnAttackPhaseChanged.Broadcast(AttackPhase);
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Boxer->RefreshCombatPresentation();
	}
}

void UFBCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (AttackPhase != EFBAttackPhase::Idle)
	{
		PhaseTimeRemaining -= DeltaTime;
		const bool bWindowOver = PhaseTimeRemaining <= 0.f;
		if (AttackPhase == EFBAttackPhase::Active && !bResolvedThisAttack)
		{
			ResolveAgainstOpponent(PendingOpponent.Get(), bWindowOver);
		}
		if (bWindowOver)
		{
			if (AttackPhase == EFBAttackPhase::Startup)
			{
				ActiveWindowStartTime = ServerClock();
				bHasGloveSample = false;
				SetPhase(EFBAttackPhase::Active, ActiveWindowSeconds * ActiveTimeScale);
				bResolvedThisAttack = false;
			}
			else if (AttackPhase == EFBAttackPhase::Active)
			{
				float Recovery = RecoverySeconds * ActiveTimeScale;
				if (bLastContactMissed)
				{
					if (const AFBBoxerCharacter* ReflexBoxer = Cast<AFBBoxerCharacter>(GetOwner()))
					{
						Recovery *= ReflexBoxer->GetCombatDerived().ReflexWhiff;
					}
					Recovery = FMath::Max(RecoverySeconds * 0.75f, Recovery);
				}
				SetPhase(EFBAttackPhase::Recovery, Recovery);
			}
			else if (AttackPhase == EFBAttackPhase::Recovery)
			{
				ActiveAttack = EFBAttackId::None;
				bActiveBodyShot = false;
				bActivePowerShot = false;
				bActiveTiredShot = false;
				SetPhase(EFBAttackPhase::Idle, 0.f);
				PendingOpponent = nullptr;
				ApplyDesiredDefense();
				if (bHasBufferedAttack)
				{
					const FFBCombatIntent Queued = BufferedIntent;
					AActor* QueuedOpponent = BufferedOpponent.Get();
					ClearBufferedAttack();
					if (BoutAllowsNewAttack() && CanAfford(Queued.Attack, Queued.bPowerPunch))
					{
						bActivePrecisionCone = Queued.bPrecisionCone;
						BeginAttack(Queued.Attack, QueuedOpponent, Queued.bBodyTarget, Queued.bPowerPunch);
					}
				}
			}
		}
	}

	// Body damage takes the wind. Head damage does not, because that job belongs to stun.
	const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner());
	const float RecoveryScale = Boxer ? Boxer->GetBurstRecoveryScale() : 1.f;
	const float RecoverableCeiling = FMath::Max(10.f, MaxBurstEnergy - Fatigue);
	if (PunchRegenDelay > 0.f)
	{
		PunchRegenDelay = FMath::Max(0.f, PunchRegenDelay - DeltaTime);
	}
	// The count owns the tank while a fighter is down. Standing, even behind a guard, still breathes.
	if (!Boxer || !Boxer->IsKnockedDown())
	{
		float Regen = EnergyRecoverPerSecond;
		if (Boxer)
		{
			Regen *= Boxer->GetCombatDerived().StaminaRegen;
		}
		const bool bEligible = EnergyRegenEligible(AttackPhase, GuardState, PunchRegenDelay);
		const float Rate = StaminaRegenRate(Regen * RecoveryScale, GuardState, bEligible);
		BurstEnergy = AdvanceRecoverableEnergy(
			BurstEnergy,
			RecoverableCeiling,
			Rate,
			DeltaTime,
			true);
	}
}

bool UFBCombatComponent::CanAfford(EFBAttackId Attack, bool bPowerShot) const
{
	const float Cost = CostFor(Attack, bPowerShot);
	if (BurstEnergy >= Cost)
	{
		return true;
	}
	// A gassed fighter can still jab. Power and heavier shots wait until they fit.
	return Attack == EFBAttackId::Jab && !bPowerShot && BurstEnergy >= TiredJabFloor;
}

float UFBCombatComponent::CostFor(EFBAttackId Attack, bool bPowerShot) const
{
	float Base = 0.f;
	switch (Attack)
	{
	case EFBAttackId::Jab: Base = JabCost; break;
	case EFBAttackId::Cross: Base = CrossCost; break;
	case EFBAttackId::LeadHook: Base = LeadHookCost; break;
	case EFBAttackId::RearHook: Base = RearHookCost; break;
	case EFBAttackId::LeadUppercut: Base = LeadUppercutCost; break;
	case EFBAttackId::RearUppercut: Base = RearUppercutCost; break;
	default: return 0.f;
	}
	if (bPowerShot)
	{
		Base *= PowerCostMultiplier;
	}
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Base *= Boxer->GetCombatDerived().EnduranceCost;
	}
	return FMath::Max(0.4f, Base);
}

float UFBCombatComponent::DamageFor(EFBAttackId Attack) const
{
	switch (Attack)
	{
	case EFBAttackId::Jab: return JabDamage;
	case EFBAttackId::Cross: return CrossDamage;
	case EFBAttackId::LeadHook: return LeadHookDamage;
	case EFBAttackId::RearHook: return RearHookDamage;
	case EFBAttackId::LeadUppercut: return LeadUppercutDamage;
	case EFBAttackId::RearUppercut: return RearUppercutDamage;
	default: return 0.f;
	}
}

void UFBCombatComponent::BeginAttack(EFBAttackId Attack, AActor* Opponent, bool bBodyShot, bool bPowerShot)
{
	ActiveAttack = Attack;
	ActiveAttackInstanceId = NextAttackInstanceId++;
	bActiveBodyShot = bBodyShot;
	bActivePowerShot = bPowerShot;
	PendingOpponent = Opponent;
	DefenderLocationAtStart = Opponent ? Opponent->GetActorLocation() : FVector::ZeroVector;
	bResolvedThisAttack = false;
	DamagedDefendersThisAttack.Reset();
	bHasGloveSample = false;
	const float Cost = CostFor(Attack, bPowerShot);
	bActiveTiredShot = BurstEnergy < Cost;
	// A tired jab stays above empty so it does not start the count. A shot that fits can still spend the tank out.
	const float Spend = bActiveTiredShot ? FMath::Min(Cost, FMath::Max(0.f, BurstEnergy - 1.f)) : Cost;
	BurstEnergy = FMath::Max(0.f, BurstEnergy - Spend);
	PunchRegenDelay = FFBFighterBalance::RegenDelaySeconds;
	ActiveTimeScale = 1.f;
	if (const AFBBoxerCharacter* SlowBoxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		const float PunchSpeed = FMath::Max(0.5f, SlowBoxer->GetCombatDerived().PunchSpeed);
		ActiveTimeScale = SlowBoxer->GetActionTimeScale() / PunchSpeed;
	}
	SetPhase(EFBAttackPhase::Startup, StartupSeconds * ActiveTimeScale);

	if (Attack == EFBAttackId::LeadHook || Attack == EFBAttackId::RearHook)
	{
		if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
		{
			Boxer->BeginHookFacingLock(StartupSeconds + ActiveWindowSeconds + 0.12f);
		}
	}

	if (BurstEnergy <= 0.05f)
	{
		AbortAttack();
		if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
		{
			if (UWorld* World = GetWorld())
			{
				if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
				{
					Mode->NotifyEnergyDepleted(Boxer);
				}
			}
		}
	}
}

void UFBCombatComponent::AbortAttack()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}
	ActiveAttack = EFBAttackId::None;
	ActiveAttackInstanceId = 0;
	bActiveBodyShot = false;
	bActivePowerShot = false;
	bActiveTiredShot = false;
	bResolvedThisAttack = true;
	PendingOpponent = nullptr;
	bHasGloveSample = false;
	ClearBufferedAttack();
	SetPhase(EFBAttackPhase::Idle, 0.f);
	ApplyDesiredDefense();
}

void UFBCombatComponent::RestoreBurstEnergy(float Amount)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}
	BurstEnergy = FMath::Clamp(Amount, 0.f, MaxBurstEnergy);
}

void UFBCombatComponent::RestoreBetweenRound()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	float Recovery = 1.f;
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Recovery = Boxer->GetCombatDerived().RoundRecovery;
	}
	float Energy = BurstEnergy;
	float NextFatigue = Fatigue;
	ApplyBetweenRoundRest(Energy, NextFatigue, MaxBurstEnergy, Recovery);
	Fatigue = NextFatigue;
	RestoreBurstEnergy(Energy);
}

void UFBCombatComponent::ResetForRematch()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	AbortAttack();
	Fatigue = 0.f;
	bDesiredGuardPending = false;
	bDesiredBody = false;
	bDesiredPower = false;
	GuardState = EFBGuardState::Open;
	GuardStateSince = -1.f;
	RestoreBurstEnergy(MaxBurstEnergy);
}

void UFBCombatComponent::RememberDesiredDefense(const FFBCombatIntent& Intent)
{
	bDesiredBody = Intent.bBodyTarget;
	bDesiredPower = Intent.bPowerPunch;
	// A punch intent must not drop a guard the player is still holding.
	// The release arrives as its own intent with no attack.
	if (!IntentUpdatesGuard(Intent))
	{
		return;
	}
	bDesiredGuardPending = true;
	bDesiredGuardHeld = Intent.bGuardHeld;
	DesiredGuard = Intent.bGuardHeld ? Intent.Guard : EFBGuardState::Open;
}

void UFBCombatComponent::ApplyDesiredDefense()
{
	if (!bDesiredGuardPending || AttackPhase != EFBAttackPhase::Idle)
	{
		return;
	}
	bDesiredGuardPending = false;
	const EFBGuardState NewGuard = bDesiredGuardHeld ? DesiredGuard : EFBGuardState::Open;
	if (NewGuard == GuardState)
	{
		return;
	}
	GuardState = NewGuard;
	GuardStateSince = ServerClock();
	OnRep_CombatVisual();
}

void UFBCombatComponent::ClearBufferedAttack()
{
	bHasBufferedAttack = false;
	BufferedIntent = FFBCombatIntent();
	BufferedOpponent = nullptr;
}

bool UFBCombatComponent::BoutAllowsNewAttack() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	const AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>();
	if (!Mode)
	{
		return true;
	}
	return Mode->AllowsNewAttacks();
}

bool UFBCombatComponent::DidDefenderRollUnder(const AActor* Attacker, const AFBBoxerCharacter* Defender) const
{
	if (!Attacker || !Defender)
	{
		return false;
	}

	FVector PunchDir = Attacker->GetActorForwardVector().GetSafeNormal2D();
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(Attacker))
	{
		const FVector Locked = Boxer->GetHookLockDirection().GetSafeNormal2D();
		if (!Locked.IsNearlyZero())
		{
			PunchDir = Locked;
		}
	}

	const FVector Delta = Defender->GetActorLocation() - DefenderLocationAtStart;
	const FVector Lateral = Delta - PunchDir * FVector::DotProduct(Delta, PunchDir);
	float Need = SlipLateralUU;
	if (Defender)
	{
		Need /= FMath::Max(0.5f, Defender->GetCombatDerived().ReflexWeave);
	}
	return Lateral.Size2D() >= Need;
}

float UFBCombatComponent::ServerClock() const
{
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		return Boxer->GetSyncedTimeSeconds();
	}
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

bool UFBCombatComponent::IsHeldDefenseOnTime(float ActiveWindowStart) const
{
	if (GuardState == EFBGuardState::Open || GuardStateSince < 0.f)
	{
		return false;
	}
	return GuardStateSince <= ActiveWindowStart;
}

EFBIntentResult UFBCombatComponent::ServerTryProcessIntent(const FFBCombatIntent& Intent, AActor* Opponent)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return EFBIntentResult::RejectedNoAuthority;
	}

	if (AFBBoxerCharacter* Self = Cast<AFBBoxerCharacter>(OwnerActor))
	{
		if (Self->IsKnockedDown())
		{
			return EFBIntentResult::RejectedDown;
		}
	}
	if (const AFBBoxerCharacter* Defender = Cast<AFBBoxerCharacter>(Opponent))
	{
		if (Defender->IsKnockedDown() && Intent.Attack != EFBAttackId::None)
		{
			return EFBIntentResult::RejectedDown;
		}
	}

	if (Intent.Sequence <= LastProcessedSequence)
	{
		return EFBIntentResult::RejectedSequence;
	}

	if (Intent.SchemaVersion != TEXT("1.0.0"))
	{
		return EFBIntentResult::RejectedSchema;
	}

	LastProcessedSequence = Intent.Sequence;
	RememberDesiredDefense(Intent);
	if (AttackPhase == EFBAttackPhase::Idle)
	{
		ApplyDesiredDefense();
	}

	if (Intent.Attack == EFBAttackId::None)
	{
		if (Intent.RingAction != EFBRingAction::None)
		{
			if (!BoutAllowsNewAttack())
			{
				return EFBIntentResult::RejectedBreak;
			}
			if (AFBBoxerCharacter* Self = Cast<AFBBoxerCharacter>(OwnerActor))
			{
				if (UWorld* World = GetWorld())
				{
					if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
					{
						Mode->HandleRingAction(Self, Intent, Opponent);
					}
				}
			}
		}
		if (bHasBufferedAttack)
		{
			BufferedIntent.bBodyTarget = bDesiredBody;
			BufferedIntent.bPowerPunch = bDesiredPower;
		}
		return EFBIntentResult::Accepted;
	}

	if (!BoutAllowsNewAttack())
	{
		return EFBIntentResult::RejectedBreak;
	}

	if (InComboBufferTail(AttackPhase, PhaseTimeRemaining, ComboBufferSeconds))
	{
		if (bHasBufferedAttack)
		{
			return EFBIntentResult::RejectedPhase;
		}
		if (!CanAfford(Intent.Attack, Intent.bPowerPunch))
		{
			return EFBIntentResult::RejectedEnergy;
		}
		BufferedIntent = Intent;
		BufferedOpponent = Opponent;
		bHasBufferedAttack = true;
		return EFBIntentResult::Buffered;
	}

	if (AttackPhase != EFBAttackPhase::Idle)
	{
		return EFBIntentResult::RejectedPhase;
	}

	if (!CanAfford(Intent.Attack, Intent.bPowerPunch))
	{
		return EFBIntentResult::RejectedEnergy;
	}

	bActivePrecisionCone = Intent.bPrecisionCone;
	BeginAttack(Intent.Attack, Opponent, Intent.bBodyTarget, Intent.bPowerPunch);
	return EFBIntentResult::Accepted;
}

void UFBCombatComponent::ResolveAgainstOpponent(AActor* Opponent, bool bCommitMiss)
{
	if (bResolvedThisAttack || ActiveAttack == EFBAttackId::None)
	{
		return;
	}
	// A hurt puncher sits down on the shot with less weight. The scale is the
	// server's. The intent never carries it.
	const AFBBoxerCharacter* AttackingBoxer = Cast<AFBBoxerCharacter>(GetOwner());
	const float OutputScale = AttackingBoxer ? AttackingBoxer->GetOutgoingDamageScale() : 1.f;
	const float HandPower = AttackingBoxer ? AttackingBoxer->GetHandPower(ActiveAttack) : 1.f;
	float Exposure = (bActivePowerShot ? 1.4f : 1.f) * (bActiveBodyShot ? 0.85f : 1.f);
	const float TiredScale = bActiveTiredShot ? TiredPunchScale : 1.f;
	const float ShotScale = Exposure * OutputScale * HandPower * TiredScale;

	FFBImpactEvent Impact;
	Impact.AttackInstanceId = ActiveAttackInstanceId;
	Impact.Attack = ActiveAttack;
	Impact.AttackerStaminaCost = CostFor(ActiveAttack, bActivePowerShot);
	Impact.Region = bActiveBodyShot ? EFBHitRegion::Body : EFBHitRegion::Head;

	auto Publish = [&]()
	{
		if (Impact.AttackInstanceId == 0)
		{
			Impact.AttackInstanceId = NextAttackInstanceId++;
			ResolvedAttackInstanceIds.Add(Impact.AttackInstanceId);
		}
		bResolvedThisAttack = true;
		LastImpact = Impact;
		OnRep_LastImpact();
	};

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Opponent)
	{
		Impact.Result = EFBContactResult::Miss;
		AddMissFatigue(FatiguePerMiss);
		Publish();
		return;
	}

	if (AFBHeavyBag* Bag = Cast<AFBHeavyBag>(Opponent))
	{
		const float Duration = FMath::Max(0.01f, ActiveWindowSeconds * ActiveTimeScale);
		const float Alpha = FMath::Clamp(1.f - (PhaseTimeRemaining / Duration), 0.f, 1.f);
		const FVector PunchCenter = AttackingBoxer
			? AttackingBoxer->GetGlovePoint(ActiveAttack, Alpha)
			: OwnerActor->GetActorLocation();
		const FVector PunchFrom = bHasGloveSample
			? GloveSample
			: (AttackingBoxer ? AttackingBoxer->GetGlovePoint(ActiveAttack, FMath::Max(0.f, Alpha - 0.05f)) : PunchCenter);
		GloveSample = PunchCenter;
		bHasGloveSample = true;
		const float PunchRadius = AttackingBoxer
			? AttackingBoxer->GetPunchVolumeRadius(ActiveAttack)
			: 40.f;
		const FVector PunchMid = (PunchFrom + PunchCenter) * 0.5f;
		if (!Bag->OverlapsPunch(PunchCenter, PunchRadius)
			&& !Bag->OverlapsPunch(PunchFrom, PunchRadius)
			&& !Bag->OverlapsPunch(PunchMid, PunchRadius))
		{
			if (!bCommitMiss)
			{
				return;
			}
			Impact.Result = EFBContactResult::Miss;
			AddMissFatigue(FatiguePerMiss);
		}
		else
		{
			Impact.Result = EFBContactResult::CleanHit;
			Impact.DamageApplied = DamageFor(ActiveAttack) * ShotScale;
			Bag->ApplyStrike(Impact.DamageApplied, PunchCenter);
		}
		Publish();
		return;
	}

	AFBBoxerCharacter* Defender = Cast<AFBBoxerCharacter>(Opponent);
	if (!Defender)
	{
		Impact.Result = EFBContactResult::Miss;
		AddMissFatigue(FatiguePerMiss);
		Publish();
		return;
	}

	const float Dist = FVector::Dist(OwnerActor->GetActorLocation(), Defender->GetActorLocation());
	const FVector ToDef = (Defender->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(OwnerActor->GetActorForwardVector().GetSafeNormal2D(), ToDef);
	const bool bInReach = Dist <= MaxReachUU && Dist >= MinReachUU;
	const bool bHook = ActiveAttack == EFBAttackId::LeadHook || ActiveAttack == EFBAttackId::RearHook;
	UFBCombatComponent* DefenderCombat = Defender->GetCombatComponent();
	const bool bRolledUnder = bHook && DidDefenderRollUnder(OwnerActor, Defender);

	if (!bInReach)
	{
		if (!bCommitMiss)
		{
			return;
		}
		Impact.Result = EFBContactResult::Miss;
		AddMissFatigue(FatiguePerMiss);
		Publish();
		UE_LOG(LogTemp, Log, TEXT("FinalBell miss attack=%d dist=%.1f facing=%.2f"), (int32)ActiveAttack, Dist, FacingDot);
		return;
	}

	AFBBoxerCharacter* AttackerBoxer = Cast<AFBBoxerCharacter>(OwnerActor);
	bool bBlockedByGuard = false;
	bool bCollisionHit = false;
	if (AttackerBoxer)
	{
		const float Duration = FMath::Max(0.01f, ActiveWindowSeconds * ActiveTimeScale);
		const float Alpha = FMath::Clamp(1.f - (PhaseTimeRemaining / Duration), 0.f, 1.f);
		const FVector To = AttackerBoxer->GetGlovePoint(ActiveAttack, Alpha);
		const FVector From = bHasGloveSample
			? GloveSample
			: AttackerBoxer->GetGlovePoint(ActiveAttack, FMath::Max(0.f, Alpha - 0.05f));
		GloveSample = To;
		bHasGloveSample = true;
		const bool bGuardCounts = DefenderCombat && DefenderCombat->IsHeldDefenseOnTime(ActiveWindowStartTime);
		bCollisionHit = AttackerBoxer->SweepGlove(
			Defender,
			From,
			To,
			AttackerBoxer->GetPunchVolumeRadius(ActiveAttack),
			bActiveBodyShot,
			bGuardCounts,
			bBlockedByGuard);
	}

	// Holding L2 is not a miss. A hook misses here only when the glove no longer
	// touches them and they stepped off the punch line. A completed planted
	// slip, lean, or roll is checked after the sweep.
	if (bHook && bRolledUnder && !bCollisionHit)
	{
		Impact.Result = EFBContactResult::Miss;
		Impact.bSlipped = true;
		AddMissFatigue(FatiguePerMiss * 0.65f);
		if (AttackerBoxer)
		{
			AttackerBoxer->ExtendHookFacingLock(0.6f);
		}
		Defender->GrantCreatedAngle(0.6f);
		Publish();
		UE_LOG(LogTemp, Log, TEXT("FinalBell slip-under hook=%d rolled=1"), (int32)ActiveAttack);
		return;
	}

	if (!bCollisionHit || FacingDot < 0.2f)
	{
		if (!bCommitMiss)
		{
			return;
		}
		Impact.Result = EFBContactResult::Miss;
		AddMissFatigue(FatiguePerMiss);
		Publish();
		UE_LOG(LogTemp, Log, TEXT("FinalBell miss attack=%d dist=%.1f facing=%.2f collision=%d"),
			(int32)ActiveAttack, Dist, FacingDot, bCollisionHit ? 1 : 0);
		return;
	}

	// Slip, roll, and lean move bones only. A head shot misses when that pose was
	// already up. A pose that ended early, or started after the window, does not.
	// Body shots still land. The capsule and the punch sphere stay where they are.
	if (!bActiveBodyShot && Defender->IsHeadEvasionOnTime(ActiveWindowStartTime))
	{
		Impact.Result = EFBContactResult::Miss;
		Impact.bSlipped = true;
		AddMissFatigue(FatiguePerMiss * 0.65f);
		Defender->GrantCreatedAngle(0.6f);
		Publish();
		UE_LOG(LogTemp, Log, TEXT("FinalBell head evasion attack=%d"), (int32)ActiveAttack);
		return;
	}

	const bool bBlocked = bBlockedByGuard && Defender->GetBlockHealth() > 0.5f;
	if (bBlockedByGuard && Defender->GetBlockHealth() <= 0.5f)
	{
		bCollisionHit = true;
	}
	const float AngleMul = AttackerBoxer ? AttackerBoxer->GetAnglePowerMultiplier() : 1.f;
	Impact.bAngleBonus = AngleMul > 1.01f;
	float Combined = Exposure * AngleMul;
	if (Combined > 1.f)
	{
		Combined = FMath::Min(Combined, FFBFighterBalance::ExposureCap);
	}
	float CleanDamage = DamageFor(ActiveAttack) * Combined * OutputScale * HandPower;
	if (bActivePrecisionCone)
	{
		CleanDamage *= 1.06f;
	}

	if (bBlocked)
	{
		Impact.Result = EFBContactResult::Blocked;
		Impact.bGuardIntercepted = true;
		Defender->NoteBlockedImpact(CleanDamage);
		bLastContactMissed = false;
		Impact.DamageApplied = CleanDamage * 0.15f;
		if (Defender->IsGuardVulnerable())
		{
			if (UWorld* World = GetWorld())
			{
				if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
				{
					Mode->NotifyGuardBroken();
				}
			}
		}
	}
	else
	{
		Impact.Result = EFBContactResult::CleanHit;
		Impact.DamageApplied = CleanDamage;
		bLastContactMissed = false;
	}

	if (AttackerBoxer && AttackerBoxer->IsInClinch())
	{
		Impact.DamageApplied *= 0.58f;
	}
	if (Impact.AttackInstanceId == 0)
	{
		Impact.AttackInstanceId = NextAttackInstanceId++;
		ResolvedAttackInstanceIds.Add(Impact.AttackInstanceId);
	}
	bResolvedThisAttack = true;
	const int32 DefenderId = static_cast<int32>(Defender->GetUniqueID());
	if (DamagedDefendersThisAttack.Contains(DefenderId))
	{
		return;
	}
	DamagedDefendersThisAttack.Add(DefenderId);
	Defender->ApplyAuthoritativeDamage(Impact.DamageApplied, Impact.AttackInstanceId, Impact.Region, Impact.Result == EFBContactResult::CleanHit);
	if (UWorld* World = GetWorld())
	{
		if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
		{
			Mode->NoteImpact(AttackerBoxer, Impact);
		}
	}
	LastImpact = Impact;
	OnRep_LastImpact();
	UE_LOG(LogTemp, Log, TEXT("FinalBell impact %s region=%s dmg=%.1f angle=%.2f output=%.2f head=%.1f body=%.1f stun=%.1f"),
		Impact.Result == EFBContactResult::CleanHit ? TEXT("HIT") :
		Impact.Result == EFBContactResult::Blocked ? TEXT("BLOCK") : TEXT("MISS"),
		Impact.Region == EFBHitRegion::Body ? TEXT("body") : TEXT("head"),
		Impact.DamageApplied, AngleMul, OutputScale,
		Defender->GetHeadDamage(), Defender->GetBodyDamage(), Defender->GetStun());
}

void UFBCombatComponent::AddMissFatigue(float Amount)
{
	float Scale = 1.f;
	if (const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		Scale = Boxer->GetCombatDerived().ConditioningFatigue;
	}
	Fatigue = FMath::Min(80.f, Fatigue + Amount * Scale);
	bLastContactMissed = true;
}

