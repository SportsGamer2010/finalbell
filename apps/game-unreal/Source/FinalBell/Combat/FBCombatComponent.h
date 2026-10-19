// Final Bell â€” server-owned stamina/contact for jab/cross/guard (P1)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/FBCombatTypes.h"

class AActor;
class AFBBoxerCharacter;

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

	/** The last resolved punch. The AI reads a miss. It does not write the result. */
	const FFBImpactEvent& GetLastImpact() const { return LastImpact; }

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
	bool IsBodyShot() const { return bActiveBodyShot; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	bool IsPowerShot() const { return bActivePowerShot; }

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
	float LeadHookCost = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearHookCost = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float LeadUppercutCost = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearUppercutCost = 11.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float LeadHookDamage = 11.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearHookDamage = 13.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float LeadUppercutDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearUppercutDamage = 14.f;

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

	/** How far the defender must step off the punch line to count as rolling under a hook. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float SlipLateralUU = 40.f;

	FVector DefenderLocationAtStart = FVector::ZeroVector;

	bool DidDefenderRollUnder(const AActor* Attacker, const AFBBoxerCharacter* Defender) const;

	/**
	 * Why: a high guard or a weave is a hold. It has to be up before the glove window opens.
	 * What: true when this guard state began at or before ActiveWindowStart. Open is never on time.
	 * A guard released early is Open, so it fails. A hold that started long before still counts.
	 */
	bool IsHeldDefenseOnTime(float ActiveWindowStart) const;

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

	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bActiveBodyShot = false;

	UPROPERTY(ReplicatedUsing = OnRep_CombatVisual, BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bActivePowerShot = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	int32 LastProcessedSequence = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LastImpact, BlueprintReadOnly, Category = "FinalBell|Combat")
	FFBImpactEvent LastImpact;

	float PhaseTimeRemaining = 0.f;
	float ActiveTimeScale = 1.f;
	/** Server clock when the current guard state began. -1 until the first change. */
	float GuardStateSince = -1.f;
	/** Server clock when this punch's active window opened. */
	float ActiveWindowStartTime = -1.f;
	float PunchRegenDelay = 0.f;
	bool bLastContactMissed = false;
	bool bActivePrecisionCone = false;
	bool bResolvedThisAttack = false;
	int32 NextAttackInstanceId = 1;
	TSet<int32> ResolvedAttackInstanceIds;

	UFUNCTION()
	void OnRep_CombatVisual();

	UFUNCTION()
	void OnRep_LastImpact();
	void AddMissFatigue(float Amount);

	bool CanAfford(EFBAttackId Attack) const;
	float CostFor(EFBAttackId Attack) const;
	float DamageFor(EFBAttackId Attack) const;
	void SetPhase(EFBAttackPhase NewPhase, float Duration);
	void BeginAttack(EFBAttackId Attack, AActor* Opponent, bool bBodyShot, bool bPowerShot);
	/**
	 * Why: the glove is live for the whole active window, not one frame at the start.
	 * What: resolves a hit, a block, or a slip. A whiff waits until bCommitMiss, which is the end of the window.
	 */
	void ResolveAgainstOpponent(AActor* Opponent, bool bCommitMiss);
	float ServerClock() const;

public:
	/**
	 * Why: a knockdown cancels the punch that emptied the tank, so it cannot land from the canvas.
	 * What: returns the attack to idle on the server.
	 */
	void AbortAttack();

	/**
	 * Why: beating the count has to leave enough energy to stand and fight.
	 * What: sets burst energy on the server, clamped to the max.
	 */
	void RestoreBurstEnergy(float Amount);
	void RestoreBetweenRound();
};

