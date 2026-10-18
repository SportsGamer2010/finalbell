#include "Combat/FBCombatComponent.h"
#include "Combat/FBBoxerCharacter.h"
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
		if (PhaseTimeRemaining <= 0.f)
		{
			if (AttackPhase == EFBAttackPhase::Startup)
			{
				SetPhase(EFBAttackPhase::Active, ActiveWindowSeconds);
				bResolvedThisAttack = false;
				ResolveAgainstOpponent(PendingOpponent.Get());
			}
			else if (AttackPhase == EFBAttackPhase::Active)
			{
				SetPhase(EFBAttackPhase::Recovery, RecoverySeconds);
			}
			else if (AttackPhase == EFBAttackPhase::Recovery)
			{
				ActiveAttack = EFBAttackId::None;
				bActiveBodyShot = false;
				bActivePowerShot = false;
				SetPhase(EFBAttackPhase::Idle, 0.f);
				PendingOpponent = nullptr;
			}
		}
	}

	const float RecoverableCeiling = FMath::Max(10.f, MaxBurstEnergy - Fatigue);
	if (AttackPhase == EFBAttackPhase::Idle && GuardState == EFBGuardState::Open)
	{
		BurstEnergy = FMath::Min(RecoverableCeiling, BurstEnergy + EnergyRecoverPerSecond * DeltaTime);
	}
}

bool UFBCombatComponent::CanAfford(EFBAttackId Attack) const
{
	return BurstEnergy >= CostFor(Attack);
}

float UFBCombatComponent::CostFor(EFBAttackId Attack) const
{
	switch (Attack)
	{
	case EFBAttackId::Jab: return JabCost;
	case EFBAttackId::Cross: return CrossCost;
	case EFBAttackId::LeadHook: return LeadHookCost;
	case EFBAttackId::RearHook: return RearHookCost;
	case EFBAttackId::LeadUppercut: return LeadUppercutCost;
	case EFBAttackId::RearUppercut: return RearUppercutCost;
	default: return 0.f;
	}
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
	bActiveBodyShot = bBodyShot;
	bActivePowerShot = bPowerShot;
	PendingOpponent = Opponent;
	DefenderLocationAtStart = Opponent ? Opponent->GetActorLocation() : FVector::ZeroVector;
	bResolvedThisAttack = false;
	BurstEnergy = FMath::Max(0.f, BurstEnergy - CostFor(Attack));
	SetPhase(EFBAttackPhase::Startup, StartupSeconds);

	if (Attack == EFBAttackId::LeadHook || Attack == EFBAttackId::RearHook)
	{
		if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetOwner()))
		{
			Boxer->BeginHookFacingLock(StartupSeconds + ActiveWindowSeconds + 0.12f);
		}
	}
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
	return Lateral.Size2D() >= SlipLateralUU;
}

bool UFBCombatComponent::ServerTryProcessIntent(const FFBCombatIntent& Intent, AActor* Opponent)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	if (Intent.Sequence <= LastProcessedSequence)
	{
		return false;
	}

	if (Intent.SchemaVersion != TEXT("1.0.0"))
	{
		return false;
	}

	LastProcessedSequence = Intent.Sequence;

	// Cannot raise/lower guard mid-committed punch.
	if (AttackPhase == EFBAttackPhase::Idle)
	{
		GuardState = Intent.bGuardHeld ? Intent.Guard : EFBGuardState::Open;
		OnRep_CombatVisual();
	}

	if (Intent.Attack == EFBAttackId::None)
	{
		return true;
	}

	if (AttackPhase != EFBAttackPhase::Idle)
	{
		return false;
	}

	if (!CanAfford(Intent.Attack))
	{
		return false;
	}

	BeginAttack(Intent.Attack, Opponent, Intent.bBodyTarget, Intent.bPowerPunch);
	return true;
}

void UFBCombatComponent::ResolveAgainstOpponent(AActor* Opponent)
{
	if (bResolvedThisAttack || ActiveAttack == EFBAttackId::None)
	{
		return;
	}
	bResolvedThisAttack = true;
	const float ShotScale = (bActivePowerShot ? 1.4f : 1.f) * (bActiveBodyShot ? 0.85f : 1.f);

	FFBImpactEvent Impact;
	Impact.AttackInstanceId = NextAttackInstanceId++;
	Impact.Attack = ActiveAttack;
	Impact.AttackerStaminaCost = CostFor(ActiveAttack);

	if (ResolvedAttackInstanceIds.Contains(Impact.AttackInstanceId))
	{
		return;
	}
	ResolvedAttackInstanceIds.Add(Impact.AttackInstanceId);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Opponent)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		LastImpact = Impact;
		OnRep_LastImpact();
		return;
	}

	if (AFBHeavyBag* Bag = Cast<AFBHeavyBag>(Opponent))
	{
		const float Dist = FVector::Dist(OwnerActor->GetActorLocation(), Bag->GetStrikeCenter());
		if (Dist > MaxReachUU + 40.f || Dist < 40.f)
		{
			Impact.Result = EFBContactResult::Miss;
			Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		}
		else
		{
			Impact.Result = EFBContactResult::CleanHit;
			Impact.DamageApplied = DamageFor(ActiveAttack) * ShotScale;
			Bag->ApplyStrike(Impact.DamageApplied, OwnerActor->GetActorLocation());
		}
		LastImpact = Impact;
		OnRep_LastImpact();
		return;
	}

	AFBBoxerCharacter* Defender = Cast<AFBBoxerCharacter>(Opponent);
	if (!Defender)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		LastImpact = Impact;
		OnRep_LastImpact();
		return;
	}

	const float Dist = FVector::Dist(OwnerActor->GetActorLocation(), Defender->GetActorLocation());
	const FVector ToDef = (Defender->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(OwnerActor->GetActorForwardVector().GetSafeNormal2D(), ToDef);
	const bool bInReach = Dist <= MaxReachUU && Dist >= MinReachUU;
	const bool bHook = ActiveAttack == EFBAttackId::LeadHook || ActiveAttack == EFBAttackId::RearHook;
	UFBCombatComponent* DefenderCombat = Defender->GetCombatComponent();
	const bool bWeave = DefenderCombat && DefenderCombat->GetGuardState() == EFBGuardState::Weave;
	const bool bRolledUnder = bHook && DidDefenderRollUnder(OwnerActor, Defender);

	if (!bInReach)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		LastImpact = Impact;
		OnRep_LastImpact();
		UE_LOG(LogTemp, Log, TEXT("FinalBell miss attack=%d dist=%.1f facing=%.2f"), (int32)ActiveAttack, Dist, FacingDot);
		return;
	}

	AFBBoxerCharacter* AttackerBoxer = Cast<AFBBoxerCharacter>(OwnerActor);
	const bool bCollisionHit = AttackerBoxer && AttackerBoxer->DoesPunchConnect(ActiveAttack, Defender);

	// Weave ducks a hook. Stepping off the punch line only counts when the punch
	// volume no longer touches them, so stepping into the hook still gets hit.
	// The puncher keeps the facing they had, and the defender gets bonus power.
	if (bHook && (bWeave || (bRolledUnder && !bCollisionHit)))
	{
		Impact.Result = EFBContactResult::Miss;
		Impact.bSlipped = true;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss * 0.65f);
		if (AttackerBoxer)
		{
			AttackerBoxer->ExtendHookFacingLock(0.6f);
		}
		Defender->GrantCreatedAngle(0.6f);
		LastImpact = Impact;
		OnRep_LastImpact();
		UE_LOG(LogTemp, Log, TEXT("FinalBell slip-under hook=%d weave=%d rolled=%d"),
			(int32)ActiveAttack, bWeave ? 1 : 0, bRolledUnder ? 1 : 0);
		return;
	}

	if (!bCollisionHit || FacingDot < 0.2f)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		LastImpact = Impact;
		OnRep_LastImpact();
		UE_LOG(LogTemp, Log, TEXT("FinalBell miss attack=%d dist=%.1f facing=%.2f collision=%d"),
			(int32)ActiveAttack, Dist, FacingDot, bCollisionHit ? 1 : 0);
		return;
	}

	const EFBGuardState DefenderGuard = DefenderCombat ? DefenderCombat->GetGuardState() : EFBGuardState::Open;
	const bool bBlocked = DefenderGuard == EFBGuardState::High && !bActiveBodyShot;
	const float AngleMul = AttackerBoxer ? AttackerBoxer->GetAnglePowerMultiplier() : 1.f;
	Impact.bAngleBonus = AngleMul > 1.01f;

	if (bBlocked)
	{
		Impact.Result = EFBContactResult::Blocked;
		Impact.bGuardIntercepted = true;
		Impact.DamageApplied = DamageFor(ActiveAttack) * 0.15f * AngleMul * ShotScale;
	}
	else
	{
		Impact.Result = EFBContactResult::CleanHit;
		Impact.DamageApplied = DamageFor(ActiveAttack) * AngleMul * ShotScale;
	}

	Defender->ApplyAuthoritativeDamage(Impact.DamageApplied, Impact.AttackInstanceId);
	LastImpact = Impact;
	OnRep_LastImpact();
	UE_LOG(LogTemp, Log, TEXT("FinalBell impact %s dmg=%.1f angle=%.2f"),
		Impact.Result == EFBContactResult::CleanHit ? TEXT("HIT") :
		Impact.Result == EFBContactResult::Blocked ? TEXT("BLOCK") : TEXT("MISS"),
		Impact.DamageApplied, AngleMul);
}

