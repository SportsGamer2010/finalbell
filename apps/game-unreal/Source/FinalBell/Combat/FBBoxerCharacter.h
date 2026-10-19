// Final Bell — the fighter you control.
// The body is the skinned mannequin. Guard and punches are bone poses on that mesh.
// Simple shapes are only a fallback if the mannequin fails to load.
// Punches are requests. The server decides whether they land.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBFighterBalance.h"
#include "FBBoxerCharacter.generated.h"

class ULocalPlayer;
class USphereComponent;

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
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PawnClientRestart() override;
	virtual void OnRep_PlayerState() override;

	/** Adds or removes the fight mapping on the owning local player. A menu passes false. */
	void SetFightMappingActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "FinalBell|Combat")
	UFBCombatComponent* GetCombatComponent() const { return CombatComponent; }

	/**
	 * Adds one punch's damage on the server.
	 * Why: a client must not decide how hurt someone is. The same punch id is
	 * ignored the second time so one swing cannot land twice.
	 * Head fills the chin pool and adds more stun. Body fills the rib pool and
	 * adds less stun. Either pool stopping at its cap does not end the bout.
	 * A flash knockout is a later check with its own rules.
	 * What: stores the amount, raises stun, and refreshes the combined total.
	 * Returns immediately on a client or on a punch id that was already applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "FinalBell|Combat")
	float ApplyAuthoritativeDamage(float Amount, int32 AttackInstanceId, EFBHitRegion Region, bool bCountAsClean = true);

	/** Head plus body. Kept so older call sites still have one combined total. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetAccumulatedDamage() const { return AccumulatedDamage; }

	/** Lasting chin damage, from 0 up to MaxHeadDamage. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetHeadDamage() const { return HeadDamage; }

	/** Lasting rib damage, from 0 up to MaxBodyDamage. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetBodyDamage() const { return BodyDamage; }

	/** The wobble after a shot. Fades on the server. Separate from the lasting pools. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetStun() const { return Stun; }

	/**
	 * Why: a hurt body cannot sit down on a punch, and a stunned fighter is
	 * off balance for a moment. Lasting head damage stays on the record and
	 * does not permanently weaken later punches. The stun from that head shot does.
	 * What: 1 when fresh. Lower while body damage or stun is up, never below
	 * MinOutputScale. The server multiplies this into the damage it assigns.
	 */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetOutgoingDamageScale() const;

	/**
	 * Why: body shots take the wind. Head shots use stun instead of the gas tank.
	 * What: 1 when the body is fresh, and BodyRecoveryAtFull when body damage is full.
	 */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetBurstRecoveryScale() const;

	/**
	 * Why: a fighter who has taken a lot of body shots should be late with the next punch.
	 * What: 1 when the body is fresh, up to about 1.8 when the body pool is full.
	 * The capsule size does not change.
	 */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetActionTimeScale() const;

	/**
	 * Plays the flinch for a hit that already happened.
	 * Why: growing the skull on impact reads as a swelling head mesh, which is
	 * not how a punch lands. A readable flinch turns the waist and tucks the chin.
	 * What: sets a short yaw, fold, and head pitch. Tick eases them back to zero.
	 * This does not change damage, the capsule, or the punch spheres.
	 */
	void PlayHurtReaction(const FFBImpactEvent& Impact);

	/** True while this fighter is on the canvas. Movement and punches are refused. */
	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	bool IsKnockedDown() const { return bKnockedDown; }

	/**
	 * Why: only the server starts and ends a knockdown, then every screen copies the flag.
	 * What: lies the mesh down, or stands it back up. The capsule size stays the same.
	 */
	void SetKnockedDown(bool bDown);

	FVector2D GetCachedLeftStick() const { return CachedLeftStick; }
	FVector2D GetCachedRightStick() const { return CachedRightStick; }

	/** Server applies a ready CompetitiveFair build. Illegal builds are refused. */
	bool ApplyFighterRecord(const FFBFighterRecord& Record, FString& OutWhy);
	void PreviewCreatorLook(const FFBPresentation& Look);
	const FFBCombatDerived& GetCombatDerived() const { return CombatDerived; }
	bool HasCompetitiveBuild() const { return bHasCompetitiveBuild; }
	FString GetFighterDisplayName() const { return FighterRecord.DisplayName; }
	const FFBFighterRecord& GetFighterRecord() const { return FighterRecord; }
	float GetAccuracyCone() const { return CombatDerived.AccuracyCone; }
	float GetHandPower(EFBAttackId Attack) const;
	float GetBlockHealth() const { return BlockHealth; }
	float GetBlockHealthMax() const { return BlockHealthMax; }
	bool IsGuardVulnerable() const { return BlockHealth <= 0.5f; }
	void NoteBlockedImpact(float CleanDamage);
	void NoteInjuryChance(EFBHitRegion Region, bool bClean);
	float TakeRegionalDamage(float Amount, EFBHitRegion Region);
	void NoteHeadShock(float Amount);
	void NoteBodyShock(float Amount);
	float ConsumeDazeStun();

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	bool IsInClinch() const { return bInClinch; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	float GetClinchTurnMeter() const { return ClinchTurnMeter; }

	UFUNCTION(BlueprintPure, Category = "FinalBell|Combat")
	int32 GetClinchTurnsLanded() const { return ClinchTurnsLanded; }

	/** Server only. The referee owns the clinch, the client only asks to start one. */
	void AuthoritySetClinch(bool bDown);
	void AuthorityAddClinchMeter(float Amount);
	void AuthorityResetClinchMeter();
	int32& AuthorityClinchStarts() { return ClinchStarts; }

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCombatIntent(const FFBCombatIntent& Intent);

	/** Development feedback. Shipping builds ignore it. */
	UFUNCTION(Client, Unreliable)
	void ClientReportIntent(EFBIntentResult Result, EFBAttackId Attack);

	/** Bone roll only. The capsule stays. StickX is -1 for the fighter's left. */
	UFUNCTION(Server, Reliable)
	void ServerApplyShoulderRoll(float StickX);

	/** Points the ringside camera at this fighter and the opponent. Recovers if that camera was destroyed. */
	void RefreshFightCamera();

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

	/** Glove sphere for this attack. The heavy bag uses the same center. The radius is unchanged. */
	FVector GetPunchVolumeCenter(EFBAttackId Attack) const;
	float GetPunchVolumeRadius(EFBAttackId Attack) const;

	/** 0 is the hand at the start of the active window. 1 is the existing punch-sphere center. */
	FVector GetGlovePoint(EFBAttackId Attack, float Alpha) const;

	/**
	 * Sweeps the glove sphere from From to To against query volumes on Target.
	 * The movement capsule is not a hit. A high guard does not block a body shot.
	 * bOutBlocked is true when a guard volume was in the way and bGuardCounts is true.
	 */
	bool SweepGlove(AActor* Target, const FVector& From, const FVector& To, float Radius, bool bBodyShot, bool bGuardCounts, bool& bOutBlocked) const;

	/** Server only. Clears damage, the canvas, and the gas tank without changing the capsule. */
	void AuthorityPrepareForRematch();

	/**
	 * Why: slip, roll, and lean are bone poses. They must not move the capsule or the punch spheres.
	 * What: true when that pose started before the glove window and has not already ended.
	 * A pose that finished early, or one that started after the window opened, is not a defense.
	 */
	bool IsHeadEvasionOnTime(float ActiveWindowStart) const;

	/** Server clock. Locks and defense windows use this so every machine agrees. */
	float GetSyncedTimeSeconds() const;

	void BeginHookFacingLock(float Seconds);
	void ExtendHookFacingLock(float Seconds);
	void GrantCreatedAngle(float Seconds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Stance, Category = "FinalBell|Boxer")
	EFBStance Stance = EFBStance::Orthodox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CornerSkin, Category = "FinalBell|Boxer")
	EFBCornerSkin CornerSkin = EFBCornerSkin::BlueCorner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float MoveSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float FaceTurnSpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer")
	float PunchLockedTurnSpeed = 1.5f;

	/**
	 * Largest angle off the opponent the stick can hold. They cannot turn their back.
	 * The capsule keeps this angle so the punch sphere stays with the shoulders.
	 * The pose then lags the hips by part of the same angle. That gap is the waist twist.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "0.0", ClampMax = "70.0"))
	float MaxFacingOffsetDegrees = 50.f;

	/** Walk speed before stun and body damage shorten the step. The capsule size stays 34 x 90. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "100.0", ClampMax = "600.0"))
	float BaseWalkSpeed = 320.f;

	/** Chin pool cap. Further head shots still refresh stun, and they do not end the bout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "1.0"))
	float MaxHeadDamage = 100.f;

	/** Rib pool cap. Further body shots still slow recovery, and they do not end the bout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "1.0"))
	float MaxBodyDamage = 100.f;

	/** Balance cap. Stun fades. It is the wobble, not the lasting injury. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "1.0"))
	float MaxStun = 100.f;

	/** Stun added per point of head damage. The chin rocks harder than the ribs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0"))
	float HeadStunMultiplier = 1.35f;

	/** Stun added per point of body damage. The wind comes from the recovery scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0"))
	float BodyStunMultiplier = 0.65f;

	/** How fast stun falls while the server is running. Clients display the replicated value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0"))
	float StunRecoverPerSecond = 18.f;

	/** Burst-energy refill when body damage is full. 1 would mean body shots do not take the wind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.2", ClampMax = "1.0"))
	float BodyRecoveryAtFull = 0.45f;

	/** Share of outgoing damage removed at full body damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BodyOutputPenalty = 0.25f;

	/** Share of outgoing damage removed at full stun. This fades as stun fades. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StunOutputPenalty = 0.15f;

	/** Lowest outgoing damage scale. A hurt fighter can still punch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.2", ClampMax = "1.0"))
	float MinOutputScale = 0.55f;

	/** Walk-speed share removed at full stun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StunMovePenalty = 0.28f;

	/** Walk-speed share removed at full body damage. A tired body should look slow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BodyMovePenalty = 0.55f;

	/** Lowest walk-speed scale. The capsule radius does not change with it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Damage", meta = (ClampMin = "0.25", ClampMax = "1.0"))
	float MinMoveScale = 0.34f;

	/** Heavyweight scale on the 180cm mannequin. 1.10 is about 6'6", in the Wilder range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "1.0", ClampMax = "1.6"))
	float BodyScale = 1.10f;

	/** Extra damage while the angle window from slipping a hook is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FinalBell|Boxer", meta = (ClampMin = "1.0", ClampMax = "2.5"))
	float AnglePowerMultiplier = 1.5f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FinalBell|Combat")
	TObjectPtr<UFBCombatComponent> CombatComponent;

	/** Query only. Not the movement capsule, and not scaled by bruises or swelling. */
	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Combat")
	TObjectPtr<USphereComponent> HeadVolume;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Combat")
	TObjectPtr<USphereComponent> TorsoVolume;

	UPROPERTY(VisibleAnywhere, Category = "FinalBell|Combat")
	TObjectPtr<USphereComponent> GuardVolume;

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

	/** Cosmetic only. These sit on the face and the ribs. They have no collision. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> CheekBruiseMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SwollenEyeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BloodyNoseMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BrowCutMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> RibBruiseMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HairMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TattooMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BeardMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ScarMesh;

	FFBCombatDerived CombatDerived;
	bool bHasCompetitiveBuild = false;
	float BlockRegenDelay = 0.f;
	float HeadShock = 0.f;
	float BodyShock = 0.f;
	float PendingDazeStun = 0.f;
	FLinearColor BaseSkin = FLinearColor(0.86f, 0.62f, 0.46f);

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TObjectPtr<USkeletalMesh> MannyMesh;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TObjectPtr<USkeletalMesh> QuinnMesh;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Skin")
	TSubclassOf<UAnimInstance> UnarmedAnimClass;

	/** Head plus body. Updated only on the server, then copied to clients. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float AccumulatedDamage = 0.f;

	/** Lasting chin damage. Server-owned. Cosmetic swelling is a later, separate pass. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float HeadDamage = 0.f;

	/** Lasting rib damage. This is what slows the gas tank. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BodyDamage = 0.f;

	/** Acute wobble. The server fades it. Clients show the latest replicated value. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float Stun = 0.f;

	/** On the canvas. Replicated so every machine lies the same fighter down. */
	UPROPERTY(ReplicatedUsing = OnRep_KnockedDown, BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bKnockedDown = false;

	UPROPERTY(ReplicatedUsing = OnRep_FighterRecord, BlueprintReadOnly, Category = "FinalBell|Fighter")
	FFBFighterRecord FighterRecord;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BlockHealth = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BlockHealthMax = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float CutSeverity = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float BruiseSeverity = 0.f;

	/** Tied up. Movement stops. Punches and the turn meter still work. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bInClinch = false;

	/** How close this fighter is to spinning the other one. 100 completes a turn. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float ClinchTurnMeter = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	int32 ClinchTurnsLanded = 0;

	/** Looser steps. The auto-face eases off so the stick can drift wider. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	bool bLooseMovement = false;

	/** World time when the taunt pose ends. Both screens read the same clock. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FinalBell|Combat")
	float TauntEndTime = -1.f;

	UPROPERTY(Replicated)
	TArray<int32> AppliedAttackInstanceIds;

	int32 LocalIntentSequence = 0;
	bool bUsingMannequin = false;
	/** Reference pose locals for the mesh currently on PoseMesh. */
	TArray<FTransform> SkinnedRefLocals;
	bool bBodyModifierHeld = false;
	bool bPowerModifierHeld = false;
	bool bBodyModifierConsumed = false;
	bool bPowerModifierConsumed = false;
	float BodyModPressTime = -1.f;
	float PowerModPressTime = -1.f;
	bool bBlockHeld = false;
	bool bWeaveHeld = false;
	bool bBlockGuardSent = false;
	EFBGuardState SentBlockGuard = EFBGuardState::Open;
	bool bSquareHeld = false;
	bool bTriangleHeld = false;
	bool bCircleHeld = false;
	bool bXHeld = false;
	bool bLeadUppercutKey = false;
	bool bRearUppercutKey = false;
	bool bPrecisionLatched = false;
	bool bLeftThumbDown = false;
	bool bRightThumbDown = false;
	float LeftThumbTime = -1.f;
	float RightThumbTime = -1.f;
	float PushReadyTime = 0.f;
	int32 ClinchStarts = 0;
	int32 FoulStrikes = 0;
	float PunchStickCooldown = 0.f;

	/**
	 * Smoothed stick angle, in degrees. Replicated so every machine can lag the
	 * hips by the same amount. The owning client writes the smooth value each
	 * tick after replication, so the local fighter does not stutter.
	 */
	UPROPERTY(Replicated)
	float FacingYawOffset = 0.f;

	/**
	 * One-shot slip pose. Pitch is the fold (positive is under, negative is
	 * a lean back). Roll is the head moving to the fighter's left when positive.
	 * The end time is the synced clock. AuthoritySlip still moves the capsule.
	 */
	UPROPERTY(Replicated)
	float DefenseSlipPitch = 0.f;

	UPROPERTY(Replicated)
	float DefenseSlipRoll = 0.f;

	UPROPERTY(Replicated)
	float DefenseSlipEndTime = -1.f;

	/** Server clock when the slip, roll, or lean pose began. Not a damage value. */
	float DefenseSlipStartTime = -1.f;

	float FacingYawOffsetTarget = 0.f;

	/** Latest left stick or WASD. Used by the get-up check. */
	FVector2D CachedLeftStick = FVector2D::ZeroVector;
	/** Latest right stick or arrow keys. */
	FVector2D CachedRightStick = FVector2D::ZeroVector;
	bool bShowingDownedMesh = false;

	/** Local flinch. Each machine starts it from the impact event, then eases it out. */
	float HurtYaw = 0.f;
	float HurtLean = 0.f;
	float HurtHeadPitch = 0.f;
	float LastSentFacingOffset = 0.f;
	bool bPunchStickConsumed = false;
	float PunchPoseAlpha = 0.f;
	EFBAttackId DisplayAttack = EFBAttackId::None;
	bool bDisplayBody = false;
	float PoseClock = 0.f;
	float SmoothedMoveForward = 0.f;
	float SmoothedMoveRight = 0.f;
	float SmoothedHeadYaw = 0.f;
	/** Left stick after the ease. Release keeps fading this so the stop is not a cut. */
	FVector2D SmoothedMoveStick = FVector2D::ZeroVector;
	bool bMoveHeld = false;
	/** Degrees the capsule yawed ahead of the hips. Fed into BuildWaistTwist. */
	float MovementYawLag = 0.f;
	float LastMovementYaw = 0.f;
	bool bHasMovementYaw = false;
	/** Eased defense bend. Presentation only. The capsule does not follow it. */
	float ShownDefensePitch = 0.f;
	float ShownDefenseRoll = 0.f;

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

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LooseAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> StanceAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> TauntAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> HeadbuttAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> LeftThumbAction;

	UPROPERTY(EditDefaultsOnly, Category = "FinalBell|Input")
	TObjectPtr<UInputAction> RightThumbAction;

	UFUNCTION()
	void OnRep_CornerSkin();

	UFUNCTION()
	void OnRep_KnockedDown();

	UFUNCTION()
	void OnRep_FighterRecord();

	void ApplyPresentation();
	void TickBlock(float DeltaSeconds);

	void ApplyDownedMesh();

	void Move(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnPunchStick(const FInputActionValue& Value);
	void OnPunchStickReleased();
	void OnTurnStick(const FInputActionValue& Value);
	void OnTurnStickReleased();
	void OnJab();
	void OnJabReleased();
	void OnCross();
	void OnCrossReleased();
	void OnLeadHook();
	void OnLeadHookReleased();
	void OnRearHook();
	void OnRearHookReleased();
	void OnLeadUppercut();
	void OnLeadUppercutReleased();
	void OnRearUppercut();
	void OnRearUppercutReleased();
	void OnBlockStarted();
	void OnBlockCompleted();
	void OnWeaveStarted();
	void OnWeaveCompleted();
	void OnBodyModStarted();
	void OnBodyModCompleted();
	void OnPowerModStarted();
	void OnPowerModCompleted();
	void OnLoose();
	void OnStance();
	void OnTaunt();
	void OnHeadbutt();
	void OnLeftThumbPressed();
	void OnLeftThumbReleased();
	void OnRightThumbPressed();
	void OnRightThumbReleased();

	void SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard);
	void ReportIntentResult(EFBIntentResult Result, EFBAttackId Attack) const;
	void UpdateQueryVolumes();
	void DrawContactDebug(const FVector& From, const FVector& To, float Radius) const;
	void SubmitRingAction(EFBRingAction Action);
	void TryPrecisionPunch(bool bFromButton);
	void TryStickPunch();
	EFBAttackId MatchPrecisionAttack() const;
	EFBAttackId AttackFromStick() const;
	EFBAttackId MirrorForStance(EFBAttackId Attack) const;
	EFBGuardState BlockGuardFromStick() const;
	void RefreshBlockHeight();
	void ReleaseShoulderModifier(bool bRollLeft, float PressStartedAt, bool bConsumed);
	void BeginShoulderRoll(float StickX);
	bool ConeMatches(EFBAttackId Attack) const;
	bool StickInCone(float CenterDegrees, float HalfDegrees) const;
	void EnsureDamageMarkers();
	void ApplyVisibleDamage();

public:
	void AuthorityToggleStance();
	void AuthorityToggleLoose();
	void AuthorityStartTaunt();
	void AuthoritySlip(FVector2D Axis);
	void AuthorityTakeAKnee();
	/** Picks slip left, slip right, roll under, or lean back from the stick. Does not move the capsule. */
	void BeginDefenseSlip(FVector2D Axis);
	int32 AuthorityNoteFoul();

protected:

	UFUNCTION(Server, Unreliable)
	void ServerSubmitClinchSteer(float Axis);

	UFUNCTION()
	void OnRep_Stance();
	AFBBoxerCharacter* FindOpponent() const;
	AActor* FindCombatTarget() const;
	AFBBroadcastCamera* FindOrbitCamera() const;
	void EnsurePersonalCamera();
	void FaceOpponent(float DeltaSeconds);
	void UpdateFacingOffset(float DeltaSeconds);
	/** Eases a stop after the stick is released. Held input is applied from Move. */
	void FadeReleasedMove(float DeltaSeconds);
	void PushSmoothedMove();
	/** Capsule yaw this frame becomes hip lag. The chest share cancels it, so the shoulders stay on the capsule. */
	void UpdateMovementYawLag(float DeltaSeconds);
	/** Eases the slip, roll, and lean. Weave reads the stick or the velocity. */
	void UpdateDefensePose(float DeltaSeconds);
	void ConfigureBodyCollision();
	void EnsureRuntimeInput();

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingOffset(float OffsetDegrees);

	UFUNCTION()
	void OnRep_AngleBonus();
	void ApplyMappingContext();
	void ClearMappingContext();
	bool ControllerWantsFightMapping() const;
	TWeakObjectPtr<ULocalPlayer> FightMappingPlayer;
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
	/** Writes the guard, the step, and the active punch onto the mannequin skeleton. */
	void ApplySkinnedPose(float DeltaSeconds);

	/**
	 * Why: the mannequin and the cylinder fallback must coil by the same amount,
	 * or a failed mesh would punch with a stiff torso.
	 * What: lead-side degrees for the punch on screen, already scaled by how
	 * far the arm has extended and mirrored for southpaw.
	 */
	float ComputeDisplayedPunchYaw() const;

	/**
	 * Why: stun and a hurt body shorten the step. The capsule stays the same size,
	 * so the punch spheres do not grow or shrink with the injury.
	 * What: 1 when fresh, down to MinMoveScale when stun and body damage are full.
	 */
public:
	float GetMoveSpeedScale() const;

protected:

	/** Why: only the server may fade stun, or a client would drain it twice. What: subtracts StunRecoverPerSecond. */
	void DecayStun(float DeltaSeconds);

	/** Why: the flinch is presentation. What: eases yaw, fold, and head pitch back to zero on this machine. */
	void DecayHurtReaction(float DeltaSeconds);

	/** Why: walk speed has to follow the replicated pools on every machine. What: writes MaxWalkSpeed from BaseWalkSpeed and GetMoveSpeedScale. */
	void ApplyConditionToMovement();

	void AttachGlovesToHands();
	FName LeadHandSocket() const;
	FName RearHandSocket() const;
};
