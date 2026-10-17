// Final Bell — authoritative combat stub (standalone + listen/dedicated server)
#include "Combat/FBCombatComponent.h"
#include "Combat/FBBoxerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

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
	DOREPLIFETIME(UFBCombatComponent, LastProcessedSequence);
}

void UFBCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (ActiveWindowRemaining > 0.f)
	{
		ActiveWindowRemaining = FMath::Max(0.f, ActiveWindowRemaining - DeltaTime);
		if (ActiveWindowRemaining <= 0.f)
		{
			ActiveAttack = EFBAttackId::None;
		}
	}

	if (RecoveryRemaining > 0.f)
	{
		RecoveryRemaining = FMath::Max(0.f, RecoveryRemaining - DeltaTime);
	}

	const float RecoverableCeiling = FMath::Max(10.f, MaxBurstEnergy - Fatigue);
	if (ActiveAttack == EFBAttackId::None && RecoveryRemaining <= 0.f && GuardState == EFBGuardState::Open)
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

void UFBCombatComponent::BeginAttack(EFBAttackId Attack)
{
	ActiveAttack = Attack;
	ActiveWindowRemaining = ActiveWindowSeconds;
	RecoveryRemaining = RecoverySeconds;
	BurstEnergy = FMath::Max(0.f, BurstEnergy - CostFor(Attack));
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
		UE_LOG(LogTemp, Warning, TEXT("FinalBell: rejected CombatIntent schema %s"), *Intent.SchemaVersion);
		return false;
	}

	LastProcessedSequence = Intent.Sequence;
	GuardState = Intent.bGuardHeld ? Intent.Guard : EFBGuardState::Open;

	if (Intent.Attack == EFBAttackId::None)
	{
		return true;
	}

	if (RecoveryRemaining > 0.f || ActiveAttack != EFBAttackId::None)
	{
		return false;
	}

	if (!CanAfford(Intent.Attack))
	{
		return false;
	}

	BeginAttack(Intent.Attack);
	ResolveAgainstOpponent(Opponent);
	return true;
}

void UFBCombatComponent::ResolveAgainstOpponent(AActor* Opponent)
{
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
	if (!Defender || !GetOwner())
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		OnImpactResolved.Broadcast(Impact);
		return;
	}

	const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Defender->GetActorLocation());
	if (Dist > MaxReachUU)
	{
		Impact.Result = EFBContactResult::Miss;
		Fatigue = FMath::Min(80.f, Fatigue + FatiguePerMiss);
		OnImpactResolved.Broadcast(Impact);
		return;
	}

	UFBCombatComponent* DefenderCombat = Defender->GetCombatComponent();
	const bool bBlocked = DefenderCombat
		&& DefenderCombat->GetGuardState() != EFBGuardState::Open;

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
	OnImpactResolved.Broadcast(Impact);
}
