// Final Bell — placeholder boxer with footwork, stance, combat component (P1)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/FBCombatTypes.h"
#include "FBBoxerCharacter.generated.h"

class UFBCombatComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class FINALBELL_API AFBBoxerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFBBoxerCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	virtual void OnRep_PlayerState() override;

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Combat")
	UFBCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Combat")
	void ApplyAuthoritativeDamage(float Amount, int32 AttackInstanceId);

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetAccumulatedDamage() const { return AccumulatedDamage; }

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCombatIntent(const FFBCombatIntent& Intent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "FinalBell|Boxer")
	EFBStance Stance = EFBStance::Orthodox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float MoveSpeedScale = 1.f;

	/** Temporary colored body so P1 is visible without authored fighter meshes. */
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void ApplyPlaceholderAppearance(bool bIsOpponent);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Combat")
	TObjectPtr<UFBCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Boxer")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Boxer")
	TObjectPtr<UStaticMeshComponent> PlaceholderHead;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float AccumulatedDamage = 0.f;

	UPROPERTY(Replicated)
	TArray<int32> AppliedAttackInstanceIds;

	int32 LocalIntentSequence = 0;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> JabAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> CrossAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> GuardAction;

	void Move(const FInputActionValue& Value);
	void OnJab();
	void OnCross();
	void OnGuardStarted();
	void OnGuardCompleted();

	void SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard);
	AFBBoxerCharacter* FindOpponent() const;
	void EnsureRuntimeInput();
	void ApplyMappingContext();
};
