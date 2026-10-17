// Final Bell — server-owned stamina/contact for jab/cross/guard (P1)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/FBCombatTypes.h"
#include "FBCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFBOnImpactResolved, const FFBImpactEvent&, Impact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFBOnAttackPhaseChanged, EFBAttackPhase, Phase);

UCLASS(ClassGroup = (FinalBell), meta = (BlueprintSpawnableComponent))
class FINALBELL_API UFBCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFBCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Combat")
	bool ServerTryProcessIntent(const FFBCombatIntent& Intent, AActor* Opponent);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetBurstEnergy() const { return BurstEnergy; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetFatigue() const { return Fatigue; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	EFBGuardState GetGuardState() const { return GuardState; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	EFBAttackId GetActiveAttack() const { return ActiveAttack; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	EFBAttackPhase GetAttackPhase() const { return AttackPhase; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	bool IsAttackCommitted() const { return AttackPhase != EFBAttackPhase::Idle; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	bool IsAttackActive() const { return AttackPhase == EFBAttackPhase::Active; }

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Combat")
	FFBOnImpactResolved OnImpactResolved;

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Combat")
	FFBOnAttackPhaseChanged OnAttackPhaseChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float MaxBurstEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float JabCost = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float CrossCost = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float JabDamage = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float CrossDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float MaxReachUU = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float MinReachUU = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float StartupSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float ActiveWindowSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RecoverySeconds = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float EnergyRecoverPerSecond = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float FatiguePerMiss = 1.5f;

	/** Pending opponent used when startup completes (authority only). */
	TWeakObjectPtr<AActor> PendingOpponent;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BurstEnergy = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float Fatigue = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBGuardState GuardState = EFBGuardState::Open;

	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBAttackId ActiveAttack = EFBAttackId::None;

	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBAttackPhase AttackPhase = EFBAttackPhase::Idle;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	int32 LastProcessedSequence = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LastImpact, BlueprintReadOnly, Category = "FinalBell|Combat")
	FFBImpactEvent LastImpact;

	float PhaseTimeRemaining = 0.f;
	bool bResolvedThisAttack = false;
	int32 NextAttackInstanceId = 1;
	TSet<int32> ResolvedAttackInstanceIds;

	UFUNCTION()
	void OnRep_CombatVisual();

	UFUNCTION()
	void OnRep_LastImpact();

	bool CanAfford(EFBAttackId Attack) const;
	float CostFor(EFBAttackId Attack) const;
	float DamageFor(EFBAttackId Attack) const;
	void SetPhase(EFBAttackPhase NewPhase, float Duration);
	void BeginAttack(EFBAttackId Attack, AActor* Opponent);
	void ResolveAgainstOpponent(AActor* Opponent);
};
