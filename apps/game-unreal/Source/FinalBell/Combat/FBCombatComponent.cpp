#include "Combat/FBCombatComponent.h"
#include "Combat/FBBoxerCharacter.h"
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
				ActiveAttack = EFBAttackId::None;
			}
			else if (AttackPhase == EFBAttackPhase::Recovery)
			{
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
	default: return 0.f;
	}
}

float UFBCombatComponent::DamageFor(EFBAttackId Attack) const
{
	switch (Attack)
	{
	case EFBAttackId::Jab: return JabDamage;
	case EFBAttackId::Cross: return CrossDamage;
	default: return 0.f;
	}
}

void UFBCombatComponent::BeginAttack(EFBAttackId Attack, AActor* Opponent)
{
	ActiveAttack = Attack;
	PendingOpponent = Opponent;
	bResolvedThisAttack = false;
	BurstEnergy = FMath::Max(0.f, BurstEnergy - CostFor(Attack));
	SetPhase(EFBAttackPhase::Startup, StartupSeconds);
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

	BeginAttack(Intent.Attack, Opponent);
	return true;
}

void UFBCombatComponent::ResolveAgainstOpponent(AActor* Opponent)
{
	if (bResolvedThisAttack || ActiveAttack == EFBAttackId::None)
	{
		return;
	}
	bResolvedThisAttack = true;

	FFBImpactEvent Impact;
	Impact.AttackInstanceId = NextAttackInstanceId++;
	Impact.Attack = ActiveAttack;
	Impact.AttackerStaminaCost = CostFor(ActiveAttack);

	if (ResolvedAttackInstanceIds.Contains(Impact.AttackInstanceId))
	{
		return;
	}
	ResolvedAttackInstanceIds.Add(Impact.AttackInstanceId);

	AFBBoxerCharacter* Defender = Cast<AFBBoxerCharacter>(Opponent);
	AActor* OwnerActor = GetOwner();
	if (!Defender || !OwnerActor)
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

	if (Dist > MaxReachUU || Dist < MinReachUU || FacingDot < 0.35f)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		LastImpact = Impact;
		OnRep_LastImpact();
		UE_LOG(LogTemp, Log, TEXT("FinalBell miss attack=%d dist=%.1f facing=%.2f"), (int32)ActiveAttack, Dist, FacingDot);
		return;
	}

	UFBCombatComponent* DefenderCombat = Defender->GetCombatComponent();
	const bool bBlocked = DefenderCombat && DefenderCombat->GetGuardState() != EFBGuardState::Open;

	if (bBlocked)
	{
		Impact.Result = EFBContactResult::Blocked;
		Impact.bGuardIntercepted = true;
		Impact.DamageApplied = DamageFor(ActiveAttack) * 0.15f;
	}
	else
	{
		Impact.Result = EFBContactResult::CleanHit;
		Impact.DamageApplied = DamageFor(ActiveAttack);
	}

	Defender->ApplyAuthoritativeDamage(Impact.DamageApplied, Impact.AttackInstanceId);
	LastImpact = Impact;
	OnRep_LastImpact();
	UE_LOG(LogTemp, Log, TEXT("FinalBell impact %s dmg=%.1f"),
		Impact.Result == EFBContactResult::CleanHit ? TEXT("HIT") :
		Impact.Result == EFBContactResult::Blocked ? TEXT("BLOCK") : TEXT("MISS"),
		Impact.DamageApplied);
}
