// Final Bell — server-owned stamina/contact stub for jab/cross/guard (P1)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/FBCombatTypes.h"
#include "FBCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFBOnImpactResolved, const FFBImpactEvent&, Impact);

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
	bool IsAttackActive() const { return ActiveAttack != EFBAttackId::None && ActiveWindowRemaining > 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "FinalBell|Combat")
	FFBOnImpactResolved OnImpactResolved;

	/** Tunable costs — data-driven later; exposed for P1 playtest. */
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
	float MaxReachUU = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float ActiveWindowSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float RecoverySeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float EnergyRecoverPerSecond = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Combat|Tuning")
	float FatiguePerMiss = 1.5f;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BurstEnergy = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float Fatigue = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBGuardState GuardState = EFBGuardState::Open;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	EFBAttackId ActiveAttack = EFBAttackId::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	int32 LastProcessedSequence = 0;

	float ActiveWindowRemaining = 0.f;
	float RecoveryRemaining = 0.f;
	int32 NextAttackInstanceId = 1;
	TSet<int32> ResolvedAttackInstanceIds;

	bool CanAfford(EFBAttackId Attack) const;
	float CostFor(EFBAttackId Attack) const;
	float DamageFor(EFBAttackId Attack) const;
	void BeginAttack(EFBAttackId Attack);
	void ResolveAgainstOpponent(AActor* Opponent);
};
