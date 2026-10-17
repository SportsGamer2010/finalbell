// Final Bell — boxer with basic skin, punch presentation, opponent-facing footwork
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/FBCombatTypes.h"
#include "FBBoxerCharacter.generated.h"

class UFBCombatComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
class AFBBroadcastCamera;
struct FInputActionValue;

UCLASS()
class FINALBELL_API AFBBoxerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFBBoxerCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
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

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void ApplyBasicSkin(EFBCornerSkin Corner);

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void RefreshCombatPresentation();

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void HandleImpactPresentation(const FFBImpactEvent& Impact);

	/** Compatibility with older arena bootstrap calls. */
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void ApplyPlaceholderAppearance(bool bIsOpponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "FinalBell|Boxer")
	EFBStance Stance = EFBStance::Orthodox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CornerSkin, Category = "FinalBell|Boxer")
	EFBCornerSkin CornerSkin = EFBCornerSkin::BlueCorner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float MoveSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float FaceTurnSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float PunchLockedTurnSpeed = 1.5f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Combat")
	TObjectPtr<UFBCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> TrunksMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadGloveMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearGloveMesh;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float AccumulatedDamage = 0.f;

	UPROPERTY(Replicated)
	TArray<int32> AppliedAttackInstanceIds;

	int32 LocalIntentSequence = 0;
	float HitFlashTime = 0.f;

	UPROPERTY()
	TObjectPtr<AFBBroadcastCamera> PersonalOrbitCamera;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> JabAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> CrossAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> GuardAction;

	UFUNCTION()
	void OnRep_CornerSkin();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnJab();
	void OnCross();
	void OnGuardStarted();
	void OnGuardCompleted();

	void SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard);
	AFBBoxerCharacter* FindOpponent() const;
	AFBBroadcastCamera* FindOrbitCamera() const;
	void EnsurePersonalCamera();
	void FaceOpponent(float DeltaSeconds);
	void EnsureRuntimeInput();
	void ApplyMappingContext();
	void TintMesh(UStaticMeshComponent* MeshComp, const FLinearColor& Color);
};
