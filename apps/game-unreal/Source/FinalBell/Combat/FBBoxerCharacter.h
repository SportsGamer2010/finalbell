// Final Bell — the fighter you control.
// The body is the skinned mannequin. Guard and punches are bone poses on that mesh.
// Simple shapes are only a fallback if the mannequin fails to load.
// Punches are requests. The server decides whether they land.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/FBCombatTypes.h"
#include "FBBoxerCharacter.generated.h"

class UFBCombatComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class USkeletalMesh;
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

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Boxer")
	void ApplyPlaceholderAppearance(bool bIsOpponent);

	/** World-space capsule half height, including body scale. Use this as spawn Z. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Boxer")
	float GetStandingHalfHeight() const;

	/** Damage multiplier while this fighter holds an angle they just created. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Boxer")
	float GetAnglePowerMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "FinalBell|Boxer")
	bool IsHookFacingLocked() const;

	/** Direction the hook was thrown, held through a slip. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Boxer")
	FVector GetHookLockDirection() const;

	/** Punch volume overlaps the target. Bodies also block each other via the capsule. */
	bool DoesPunchConnect(EFBAttackId Attack, AActor* Target) const;

	void BeginHookFacingLock(float Seconds);
	void ExtendHookFacingLock(float Seconds);
	void GrantCreatedAngle(float Seconds);

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

	/** Largest angle off the opponent the stick can hold. They cannot turn their back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "0.0", ClampMax = "70.0"))
	float MaxFacingOffsetDegrees = 50.f;

	/** Heavyweight scale on the 180cm mannequin. 1.10 is about 6'6", in the Wilder range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "1.0", ClampMax = "1.6"))
	float BodyScale = 1.10f;

	/** Extra damage while the angle window from slipping a hook is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "1.0", ClampMax = "2.5"))
	float AnglePowerMultiplier = 1.5f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Combat")
	TObjectPtr<UFBCombatComponent> CombatComponent;

	/** Skinned body. Bone rotations are written each frame for the guard and the six punches. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UPoseableMeshComponent> PoseMesh;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadUpperArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadForearmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearUpperArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearForearmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadThighMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadShinMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearThighMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearShinMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> LeadShoeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Skin")
	TObjectPtr<UStaticMeshComponent> RearShoeMesh;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TObjectPtr<USkeletalMesh> MannyMesh;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TObjectPtr<USkeletalMesh> QuinnMesh;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TSubclassOf<UAnimInstance> UnarmedAnimClass;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float AccumulatedDamage = 0.f;

	UPROPERTY(Replicated)
	TArray<int32> AppliedAttackInstanceIds;

	int32 LocalIntentSequence = 0;
	float HitFlashTime = 0.f;
	bool bUsingMannequin = false;
	/** Reference pose locals for the mesh currently on PoseMesh. */
	TArray<FTransform> SkinnedRefLocals;
	bool bBodyModifierHeld = false;
	bool bPowerModifierHeld = false;
	float PunchStickCooldown = 0.f;
	float FacingYawOffset = 0.f;
	float FacingYawOffsetTarget = 0.f;
	float LastSentFacingOffset = 0.f;
	bool bPunchStickConsumed = false;
	float PunchPoseAlpha = 0.f;
	EFBAttackId DisplayAttack = EFBAttackId::None;
	bool bDisplayBody = false;

	UPROPERTY(Replicated)
	float FacingLockYaw = 0.f;

	UPROPERTY(Replicated)
	float FacingLockServerEndTime = -1.f;

	UPROPERTY(ReplicatedUsing = OnRep_AngleBonus)
	float AngleBonusServerEndTime = -1.f;

	UPROPERTY()
	TObjectPtr<AFBBroadcastCamera> PersonalOrbitCamera;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> PunchStickAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> JabAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> CrossAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LeadHookAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> RearHookAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LeadUppercutAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> RearUppercutAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> BlockAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> WeaveAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> BodyModAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> PowerModAction;

	UFUNCTION()
	void OnRep_CornerSkin();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnPunchStick(const FInputActionValue& Value);
	void OnPunchStickReleased();
	void OnTurnStick(const FInputActionValue& Value);
	void OnTurnStickReleased();
	void OnJab();
	void OnCross();
	void OnLeadHook();
	void OnRearHook();
	void OnLeadUppercut();
	void OnRearUppercut();
	void OnBlockStarted();
	void OnBlockCompleted();
	void OnWeaveStarted();
	void OnWeaveCompleted();
	void OnBodyModStarted();
	void OnBodyModCompleted();
	void OnPowerModStarted();
	void OnPowerModCompleted();

	void SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard);
	EFBAttackId ResolveUppercutVariant(EFBAttackId BaseHook) const;
	AFBBoxerCharacter* FindOpponent() const;
	AActor* FindCombatTarget() const;
	AFBBroadcastCamera* FindOrbitCamera() const;
	void EnsurePersonalCamera();
	void FaceOpponent(float DeltaSeconds);
	void UpdateFacingOffset(float DeltaSeconds);
	void ConfigureBodyCollision();
	float GetSyncedTimeSeconds() const;
	void EnsureRuntimeInput();

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingOffset(float OffsetDegrees);

	UFUNCTION()
	void OnRep_AngleBonus();
	void ApplyMappingContext();
	void TintMesh(UStaticMeshComponent* MeshComp, const FLinearColor& Color);
	void TintSkeletalMesh(const FLinearColor& Tint);
	void SetupMannequinVisuals();
	/** Loads the mannequin onto PoseMesh. Returns false when the mesh or the limb bones are missing. */
	bool TryActivateSkinnedMesh();
	/** Shows or hides the cylinder body. Gloves stay visible either way. */
	void HideShapeBody(bool bHideShapes);
	void AttachGlovesToCapsule();
	/** Reads the replicated punch phase into PunchPoseAlpha, DisplayAttack, and bDisplayBody. */
	void UpdateDisplayedPunch(float DeltaSeconds);
	/** Moves the limb shapes into the guard, or into the punch that is currently playing. */
	void UpdateBoxerPose(float DeltaSeconds);
	void ApplyShapePose();
	/** Writes the guard and the active punch onto the mannequin skeleton. */
	void ApplySkinnedPose();
	void AttachGlovesToHands();
	FName LeadHandSocket() const;
	FName RearHandSocket() const;
};
