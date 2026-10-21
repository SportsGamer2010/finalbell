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
	EFBIntentResult ServerTryProcessIntent(const FFBCombatIntent& Intent, AActor* Opponent);

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

	float GetPhaseTimeRemaining() const { return PhaseTimeRemaining; }
	float GetPunchRegenDelay() const { return PunchRegenDelay; }
	float GetRecoverableCeiling() const { return FMath::Max(10.f, MaxBurstEnergy - Fatigue); }
	int32 GetActiveAttackInstanceId() const { return ActiveAttackInstanceId; }
	float GetAttackCost(EFBAttackId Attack) const { return CostFor(Attack); }

	bool IsComboBufferOpen() const
	{
		return IsComboBufferWindow(AttackPhase, PhaseTimeRemaining, ComboBufferSeconds, bHasBufferedAttack);
	}

	/**
	 * The pause bonus starts once the punch has finished and the short delay has run out.
	 * A high guard does not block that pause. GuardRegenScale slows it instead.
	 * Holding a guard used to freeze the tank, so a normal round ran out of punches.
	 */
	static bool EnergyRegenEligible(EFBAttackPhase Phase, EFBGuardState Guard, float PunchRegenDelay)
	{
		(void)Guard;
		return Phase == EFBAttackPhase::Idle && PunchRegenDelay <= 0.f;
	}

	/** Hands down get the full pause. A held guard still refills, at this share of the bonus. */
	static constexpr float GuardRegenFraction = 0.70f;

	/** Share of the base rate that returns even during a punch, so a round of boxing does not stick at empty. */
	static constexpr float TrickleRegenFraction = 0.62f;

	/** A jab still throws at this energy. Heavier shots wait until they fit in the tank. */
	static constexpr float TiredJabFloor = 1.5f;

	/** A jab thrown below its normal cost lands lighter. It does not empty the tank. */
	static constexpr float TiredPunchScale = 0.72f;

	/** Power punches spend this much more gas than the same shot thrown normally. */
	static constexpr float PowerCostMultiplier = 1.45f;

	static float GuardRegenScale(EFBGuardState Guard)
	{
		return Guard == EFBGuardState::Open ? 1.f : GuardRegenFraction;
	}

	/**
	 * Why: boxing happens behind a guard. Regen that required the hands down
	 * emptied the tank in the first minute and then refused every punch.
	 * What: a trickle always runs. The pause bonus adds on when eligible, slower behind a guard.
	 */
	static float StaminaRegenRate(float BasePerSecond, EFBGuardState Guard, bool bEligible)
	{
		const float Trickle = FMath::Max(0.f, BasePerSecond) * TrickleRegenFraction;
		if (!bEligible)
		{
			return Trickle;
		}
		return Trickle + FMath::Max(0.f, BasePerSecond) * GuardRegenScale(Guard);
	}

	/**
	 * Why: the minute between rounds has to leave enough gas to fight, without wiping fatigue.
	 * What: cuts fatigue, then lifts energy to 82% of the new ceiling when it is below that.
	 * A tank that is already higher stays there. The ceiling still caps it.
	 */
	static void ApplyBetweenRoundRest(float& Energy, float& Fatigue, float MaxEnergy, float RoundRecovery)
	{
		const float Cut = FMath::Clamp(0.55f * RoundRecovery, 0.f, 0.75f);
		Fatigue = FMath::Max(0.f, Fatigue * (1.f - Cut));
		const float Ceiling = FMath::Max(12.f, MaxEnergy - Fatigue);
		const float Target = Ceiling * 0.82f;
		Energy = FMath::Clamp(FMath::Max(Energy, Target), 0.f, Ceiling);
	}

	static float AdvanceRecoverableEnergy(float Energy, float Ceiling, float RegenPerSecond, float DeltaSeconds, bool bEligible)
	{
		if (!bEligible || DeltaSeconds <= 0.f)
		{
			return Energy;
		}
		return FMath::Min(Ceiling, Energy + RegenPerSecond * DeltaSeconds);
	}

	/** A punch that omits the guard flag must not drop a guard that is still held. */
	static bool IntentUpdatesGuard(const FFBCombatIntent& Intent)
	{
		return !(Intent.Attack != EFBAttackId::None && !Intent.bGuardHeld);
	}

	static bool InComboBufferTail(EFBAttackPhase Phase, float TimeRemaining, float BufferSeconds)
	{
		return Phase == EFBAttackPhase::Recovery && TimeRemaining > 0.f && TimeRemaining <= BufferSeconds;
	}

	static bool IsComboBufferWindow(EFBAttackPhase Phase, float TimeRemaining, float BufferSeconds, bool bAlreadyBuffered)
	{
		return InComboBufferTail(Phase, TimeRemaining, BufferSeconds) && !bAlreadyBuffered;
	}

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Combat")
	FFBOnImpactResolved OnImpactResolved;

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Combat")
	FFBOnAttackPhaseChanged OnAttackPhaseChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float MaxBurstEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float JabCost = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float CrossCost = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float JabDamage = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float CrossDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float LeadHookCost = 3.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearHookCost = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float LeadUppercutCost = 3.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RearUppercutCost = 4.2f;

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

	/** One queued punch is accepted only in this tail of recovery, then it expires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float ComboBufferSeconds = 0.125f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float EnergyRecoverPerSecond = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float FatiguePerMiss = 0.7f;

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

	/** This punch was thrown below its normal cost. It spends less and lands lighter. */
	bool bActiveTiredShot = false;

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
	int32 ActiveAttackInstanceId = 0;
	TSet<int32> ResolvedAttackInstanceIds;
	/** Defenders already damaged by this attack. One hit per attacker, attack, and defender. */
	TSet<int32> DamagedDefendersThisAttack;

	bool bDesiredGuardPending = false;
	bool bDesiredGuardHeld = false;
	EFBGuardState DesiredGuard = EFBGuardState::Open;
	bool bDesiredBody = false;
	bool bDesiredPower = false;

	bool bHasBufferedAttack = false;
	FFBCombatIntent BufferedIntent;
	TWeakObjectPtr<AActor> BufferedOpponent;

	FVector GloveSample = FVector::ZeroVector;
	float GloveSampleAlpha = 0.f;
	bool bHasGloveSample = false;

	UFUNCTION()
	void OnRep_CombatVisual();

	UFUNCTION()
	void OnRep_LastImpact();
	void AddMissFatigue(float Amount);

	bool CanAfford(EFBAttackId Attack, bool bPowerShot = false) const;
	float CostFor(EFBAttackId Attack, bool bPowerShot = false) const;
	float DamageFor(EFBAttackId Attack) const;
	void SetPhase(EFBAttackPhase NewPhase, float Duration);
	void BeginAttack(EFBAttackId Attack, AActor* Opponent, bool bBodyShot, bool bPowerShot);
	void RememberDesiredDefense(const FFBCombatIntent& Intent);
	void ApplyDesiredDefense();
	void ClearBufferedAttack();
	bool BoutAllowsNewAttack() const;
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

	/** Clears the punch, the buffer, fatigue, and the gas tank for another bell. */
	void ResetForRematch();
};

