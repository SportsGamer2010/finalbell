#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBBoxerAIController.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBSparringGameMode.h"
#include "Combat/FBSparringPlayerController.h"
#include "Camera/FBBroadcastCamera.h"
#include "Ring/FBRingBounds.h"
#include "Input/FBRuntimeInputLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "TwoBoneIK.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Training/FBHeavyBag.h"
#include "CollisionShape.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarFBDebugContact(
	TEXT("fb.DebugContact"),
	0,
	TEXT("Draw the glove sweep and the head, torso, and guard volumes. This is not the bout HUD."));
#endif

namespace
{
	const FVector MeshForward(0.f, 1.f, 0.f);
	const FVector MeshRight(-1.f, 0.f, 0.f);
	const FVector MeshUp(0.f, 0.f, 1.f);

	// Full trunk command. The chest reaches this angle. The head keeps about 0.82 of it.
	constexpr float SideRollDegrees = 48.f;
	constexpr float LeanBackDegrees = -58.f;
	constexpr float RollUnderDegrees = 74.f;
	constexpr float RollUnderSideDegrees = 42.f;
	constexpr float ModifierTapSeconds = 0.25f;

	/**
	 * Degrees of yaw for the four bones that make a waist.
	 * PelvisYaw turns the hips. WaistYaw is spine_01, the lumbar spine.
	 * MidYaw is spine_02. ChestYaw is spine_03, the upper back.
	 * They are component-space adds, so they stack from the hips upward.
	 */
	struct FFBWaistTwist
	{
		float PelvisYaw = 0.f;
		float WaistYaw = 0.f;
		float MidYaw = 0.f;
		float ChestYaw = 0.f;
	};

	/**
	 * Builds the waist coil for this frame.
	 * Why: a punch starts in the hips and finishes in the shoulders. The old
	 * pose put the whole turn on spine_03 and rotated the pelvis the other way,
	 * so the lumbar spine stayed straight. The stick turns the capsule so the
	 * punch sphere stays with the shoulders. The hips lag that turn, and the
	 * spine puts the same amount back by the chest. Feet stay on their IK
	 * targets, and the capsule does not move.
	 * What: PunchYawDegrees is the punch coil plus the step. FacingOffsetDegrees
	 * is the stick. HurtYawDegrees is the flinch. Positive is the lead side
	 * after southpaw mirroring.
	 * Hip share of a punch is 0.22. The waist takes 0.40, the mid back 0.24,
	 * and the chest 0.14. Those four add up to the full punch yaw.
	 * The stick lags the hips by 0.75 of the offset. The three spine bones
	 * add 0.40, 0.22, and 0.13, which is the same 0.75, so the shoulders
	 * still point along the punch.
	 */
	FFBWaistTwist BuildWaistTwist(float PunchYawDegrees, float FacingOffsetDegrees, float HurtYawDegrees)
	{
		const float HipLag = FacingOffsetDegrees * 0.75f;

		FFBWaistTwist Twist;
		Twist.PelvisYaw = PunchYawDegrees * 0.22f - HipLag + HurtYawDegrees * 0.15f;
		Twist.WaistYaw = PunchYawDegrees * 0.40f + FacingOffsetDegrees * 0.40f + HurtYawDegrees * 0.55f;
		Twist.MidYaw = PunchYawDegrees * 0.24f + FacingOffsetDegrees * 0.22f + HurtYawDegrees * 0.20f;
		Twist.ChestYaw = PunchYawDegrees * 0.14f + FacingOffsetDegrees * 0.13f + HurtYawDegrees * 0.10f;
		return Twist;
	}

	/**
	 * Degrees of bend for a slip, a roll under, or a lean back.
	 * Pitch folds the chest toward the opponent when positive, and away when
	 * negative. Roll moves the head to the fighter's left when positive.
	 * The neck values undo most of that bend so the face stays readable.
	 */
	struct FFBDefenseBend
	{
		float PelvisPitch = 0.f;
		float WaistPitch = 0.f;
		float MidPitch = 0.f;
		float ChestPitch = 0.f;
		float PelvisRoll = 0.f;
		float WaistRoll = 0.f;
		float MidRoll = 0.f;
		float ChestRoll = 0.f;
		float NeckPitch = 0.f;
		float NeckRoll = 0.f;
		float HeadPitch = 0.f;
		float HeadRoll = 0.f;
	};

	/**
	 * Why: a slip or a roll has to travel through the whole trunk. A tilt that
	 * lives only on the head reads as a nod. The same four shares as a punch
	 * yaw keep the hips in the bend and the shoulders ahead of them.
	 * What: PitchDegrees and RollDegrees are the full bend. Pelvis 0.22, waist
	 * 0.40, mid back 0.24, chest 0.14. The neck gives back 0.40, so the head
	 * leaves the punch line with the shoulders instead of snapping back upright.
	 * The head keeps 0.22 of the roll and 0.20 of the pitch.
	 */
	FFBDefenseBend BuildDefenseBend(float PitchDegrees, float RollDegrees)
	{
		FFBDefenseBend Bend;
		Bend.PelvisPitch = PitchDegrees * 0.22f;
		Bend.WaistPitch = PitchDegrees * 0.40f;
		Bend.MidPitch = PitchDegrees * 0.24f;
		Bend.ChestPitch = PitchDegrees * 0.14f;
		Bend.PelvisRoll = RollDegrees * 0.22f;
		Bend.WaistRoll = RollDegrees * 0.40f;
		Bend.MidRoll = RollDegrees * 0.24f;
		Bend.ChestRoll = RollDegrees * 0.14f;
		Bend.NeckPitch = -PitchDegrees * 0.40f;
		Bend.NeckRoll = -RollDegrees * 0.40f;
		Bend.HeadPitch = PitchDegrees * 0.20f;
		Bend.HeadRoll = RollDegrees * 0.22f;
		return Bend;
	}

	void RebuildComponentPose(const FReferenceSkeleton& Skel, const TArray<FTransform>& Local, TArray<FTransform>& ComponentSpace)
	{
		const int32 Num = Local.Num();
		ComponentSpace.SetNum(Num);
		if (Num == 0)
		{
			return;
		}

		ComponentSpace[0] = Local[0];
		for (int32 Index = 1; Index < Num; ++Index)
		{
			const int32 Parent = Skel.GetParentIndex(Index);
			ComponentSpace[Index] = Local[Index] * ComponentSpace[Parent];
		}
	}

	void CommitComponentRotation(const FReferenceSkeleton& Skel, int32 Bone, const FQuat& Delta, TArray<FTransform>& Local, TArray<FTransform>& ComponentSpace)
	{
		if (!ComponentSpace.IsValidIndex(Bone))
		{
			return;
		}

		ComponentSpace[Bone].SetRotation((Delta * ComponentSpace[Bone].GetRotation()).GetNormalized());
		const int32 Parent = Skel.GetParentIndex(Bone);
		Local[Bone] = (Parent >= 0) ? ComponentSpace[Bone].GetRelativeTransform(ComponentSpace[Parent]) : ComponentSpace[Bone];
		RebuildComponentPose(Skel, Local, ComponentSpace);
	}

	void AimBoneAt(const FReferenceSkeleton& Skel, int32 Bone, int32 Child, const FVector& Target, TArray<FTransform>& Local, TArray<FTransform>& ComponentSpace)
	{
		if (!ComponentSpace.IsValidIndex(Bone) || !ComponentSpace.IsValidIndex(Child))
		{
			return;
		}

		const FVector Origin = ComponentSpace[Bone].GetLocation();
		const FVector OldDir = (ComponentSpace[Child].GetLocation() - Origin).GetSafeNormal();
		const FVector NewDir = (Target - Origin).GetSafeNormal();
		if (OldDir.IsNearlyZero() || NewDir.IsNearlyZero())
		{
			return;
		}

		CommitComponentRotation(Skel, Bone, FQuat::FindBetweenNormals(OldDir, NewDir), Local, ComponentSpace);
	}

	void SolveLimb(const FReferenceSkeleton& Skel, int32 Upper, int32 Lower, int32 End, const FVector& Effector, const FVector& Pole, TArray<FTransform>& Local, TArray<FTransform>& ComponentSpace)
	{
		if (!ComponentSpace.IsValidIndex(Upper) || !ComponentSpace.IsValidIndex(Lower) || !ComponentSpace.IsValidIndex(End))
		{
			return;
		}

		FTransform UpperCS = ComponentSpace[Upper];
		FTransform LowerCS = ComponentSpace[Lower];
		FTransform EndCS = ComponentSpace[End];
		const FVector OldForearm = (EndCS.GetLocation() - LowerCS.GetLocation()).GetSafeNormal();

		AnimationCore::SolveTwoBoneIK(UpperCS, LowerCS, EndCS, Pole, Effector, false, 1.0, 1.0);

		const FVector NewForearm = (EndCS.GetLocation() - LowerCS.GetLocation()).GetSafeNormal();
		if (!OldForearm.IsNearlyZero() && !NewForearm.IsNearlyZero())
		{
			EndCS.SetRotation((FQuat::FindBetweenNormals(OldForearm, NewForearm) * EndCS.GetRotation()).GetNormalized());
		}

		const int32 Parent = Skel.GetParentIndex(Upper);
		Local[Upper] = (Parent >= 0) ? UpperCS.GetRelativeTransform(ComponentSpace[Parent]) : UpperCS;
		Local[Lower] = LowerCS.GetRelativeTransform(UpperCS);
		Local[End] = EndCS.GetRelativeTransform(LowerCS);
		RebuildComponentPose(Skel, Local, ComponentSpace);
	}
}

AFBBoxerCharacter::AFBBoxerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = AFBBoxerAIController::StaticClass();

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	bUseControllerRotationYaw = false;
	ConfigureBodyCollision();

	CombatComponent = CreateDefaultSubobject<UFBCombatComponent>(TEXT("CombatComponent"));

	auto ArmVolume = [](USphereComponent* Sphere, float Radius, const FVector& Rel, const FName& Tag, bool bEnabled)
	{
		if (!Sphere)
		{
			return;
		}
		Sphere->SetSphereRadius(Radius);
		Sphere->SetRelativeLocation(Rel);
		Sphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		Sphere->SetCollisionObjectType(ECC_WorldDynamic);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Sphere->SetGenerateOverlapEvents(false);
		Sphere->SetHiddenInGame(true);
		Sphere->SetVisibility(false);
		Sphere->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Sphere->SetCanEverAffectNavigation(false);
		Sphere->ComponentTags.Add(Tag);
	};
	HeadVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FBHeadVolume"));
	HeadVolume->SetupAttachment(GetCapsuleComponent());
	ArmVolume(HeadVolume, 18.f, FVector(0.f, 0.f, 58.f), TEXT("FB_Head"), true);
	TorsoVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FBTorsoVolume"));
	TorsoVolume->SetupAttachment(GetCapsuleComponent());
	ArmVolume(TorsoVolume, 26.f, FVector(0.f, 0.f, 2.f), TEXT("FB_Torso"), true);
	GuardVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FBGuardVolume"));
	GuardVolume->SetupAttachment(GetCapsuleComponent());
	ArmVolume(GuardVolume, 14.f, FVector(24.f, 0.f, 56.f), TEXT("FB_Guard"), false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(
		TEXT("/Game/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnFinder(
		TEXT("/Game/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedABP(
		TEXT("/Game/Mannequins/Anims/Unarmed/ABP_Unarmed"));

	if (MannyFinder.Succeeded())
	{
		MannyMesh = MannyFinder.Object;
	}
	if (QuinnFinder.Succeeded())
	{
		QuinnMesh = QuinnFinder.Object;
	}
	if (UnarmedABP.Succeeded())
	{
		UnarmedAnimClass = UnarmedABP.Class;
	}

	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		// The posed mannequin is PoseMesh. This default mesh stays empty so it cannot
		// stand in the ring as a second body.
		Skel->SetSkeletalMesh(nullptr);
		Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Skel->SetVisibility(false);
		Skel->SetHiddenInGame(true);
		Skel->SetCastShadow(false);
	}

	PoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseMesh"));
	PoseMesh->SetupAttachment(GetCapsuleComponent());
	PoseMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	PoseMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	PoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseMesh->SetCastShadow(true);
	PoseMesh->SetVisibility(false);
	PoseMesh->SetHiddenInGame(true);
	PoseMesh->bReceivesDecals = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	auto AttachMesh = [&](const TCHAR* Name, UStaticMesh* MeshAsset, const FVector& Rel, const FVector& Scale) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(GetCapsuleComponent());
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetRelativeLocation(Rel);
		Comp->SetRelativeScale3D(Scale);
		if (MeshAsset)
		{
			Comp->SetStaticMesh(MeshAsset);
		}
		return Comp;
	};

	UStaticMesh* Cyl = CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr;
	UStaticMesh* Sph = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;
	UStaticMesh* Cub = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;

	BodyMesh = AttachMesh(TEXT("BodyMesh"), Cyl, FVector(0.f, 0.f, -20.f), FVector(0.5f, 0.45f, 0.85f));
	HeadMesh = AttachMesh(TEXT("HeadMesh"), Sph, FVector(0.f, 0.f, 55.f), FVector(0.42f, 0.42f, 0.42f));
	TrunksMesh = AttachMesh(TEXT("TrunksMesh"), Cub, FVector(0.f, 0.f, -55.f), FVector(0.55f, 0.4f, 0.22f));
	LeadGloveMesh = AttachMesh(TEXT("LeadGloveMesh"), Sph, FVector(32.f, -16.f, 40.f), FVector(0.30f));
	RearGloveMesh = AttachMesh(TEXT("RearGloveMesh"), Sph, FVector(20.f, 10.f, 38.f), FVector(0.30f));
	LeadUpperArmMesh = AttachMesh(TEXT("LeadUpperArm"), Cyl, FVector(8.f, -20.f, 22.f), FVector(0.14f, 0.14f, 0.26f));
	LeadForearmMesh = AttachMesh(TEXT("LeadForearm"), Cyl, FVector(20.f, -18.f, 28.f), FVector(0.11f, 0.11f, 0.24f));
	RearUpperArmMesh = AttachMesh(TEXT("RearUpperArm"), Cyl, FVector(4.f, 20.f, 20.f), FVector(0.14f, 0.14f, 0.26f));
	RearForearmMesh = AttachMesh(TEXT("RearForearm"), Cyl, FVector(12.f, 16.f, 26.f), FVector(0.11f, 0.11f, 0.24f));
	LeadThighMesh = AttachMesh(TEXT("LeadThigh"), Cyl, FVector(10.f, -10.f, -40.f), FVector(0.18f, 0.18f, 0.36f));
	LeadShinMesh = AttachMesh(TEXT("LeadShin"), Cyl, FVector(16.f, -12.f, -70.f), FVector(0.13f, 0.13f, 0.32f));
	RearThighMesh = AttachMesh(TEXT("RearThigh"), Cyl, FVector(-2.f, 10.f, -40.f), FVector(0.18f, 0.18f, 0.36f));
	RearShinMesh = AttachMesh(TEXT("RearShin"), Cyl, FVector(-8.f, 11.f, -70.f), FVector(0.13f, 0.13f, 0.32f));
	LeadShoeMesh = AttachMesh(TEXT("LeadShoe"), Cub, FVector(22.f, -14.f, -86.f), FVector(0.24f, 0.12f, 0.08f));
	RearShoeMesh = AttachMesh(TEXT("RearShoe"), Cub, FVector(-8.f, 14.f, -86.f), FVector(0.24f, 0.12f, 0.08f));
}

void AFBBoxerCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetupMannequinVisuals();
	ApplyBasicSkin(CornerSkin);
	EnsureDamageMarkers();
	RefreshCombatPresentation();
}

void AFBBoxerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFBBoxerCharacter, Stance);
	DOREPLIFETIME(AFBBoxerCharacter, CornerSkin);
	DOREPLIFETIME(AFBBoxerCharacter, AccumulatedDamage);
	DOREPLIFETIME(AFBBoxerCharacter, HeadDamage);
	DOREPLIFETIME(AFBBoxerCharacter, BodyDamage);
	DOREPLIFETIME(AFBBoxerCharacter, Stun);
	DOREPLIFETIME(AFBBoxerCharacter, bKnockedDown);
	DOREPLIFETIME(AFBBoxerCharacter, FighterRecord);
	DOREPLIFETIME(AFBBoxerCharacter, BlockHealth);
	DOREPLIFETIME(AFBBoxerCharacter, BlockHealthMax);
	DOREPLIFETIME(AFBBoxerCharacter, CutSeverity);
	DOREPLIFETIME(AFBBoxerCharacter, BruiseSeverity);
	DOREPLIFETIME(AFBBoxerCharacter, bInClinch);
	DOREPLIFETIME(AFBBoxerCharacter, ClinchTurnMeter);
	DOREPLIFETIME(AFBBoxerCharacter, ClinchTurnsLanded);
	DOREPLIFETIME(AFBBoxerCharacter, bLooseMovement);
	DOREPLIFETIME(AFBBoxerCharacter, TauntEndTime);
	DOREPLIFETIME(AFBBoxerCharacter, AppliedAttackInstanceIds);
	DOREPLIFETIME(AFBBoxerCharacter, FacingYawOffset);
	DOREPLIFETIME(AFBBoxerCharacter, DefenseSlipPitch);
	DOREPLIFETIME(AFBBoxerCharacter, DefenseSlipRoll);
	DOREPLIFETIME(AFBBoxerCharacter, DefenseSlipEndTime);
	DOREPLIFETIME(AFBBoxerCharacter, FacingLockYaw);
	DOREPLIFETIME(AFBBoxerCharacter, FacingLockServerEndTime);
	DOREPLIFETIME(AFBBoxerCharacter, AngleBonusServerEndTime);
}

void AFBBoxerCharacter::OnRep_CornerSkin()
{
	SetupMannequinVisuals();
	ApplyBasicSkin(CornerSkin);
}

void AFBBoxerCharacter::SetKnockedDown(bool bDown)
{
	if (!HasAuthority())
	{
		return;
	}
	bKnockedDown = bDown;
	OnRep_KnockedDown();
}

void AFBBoxerCharacter::OnRep_KnockedDown()
{
	ApplyDownedMesh();
}

void AFBBoxerCharacter::ApplyDownedMesh()
{
	if (!PoseMesh)
	{
		return;
	}
	if (bKnockedDown)
	{
		PoseMesh->SetRelativeLocation(FVector(55.f, 0.f, -78.f));
		PoseMesh->SetRelativeRotation(FRotator(-78.f, -90.f, 0.f));
		bShowingDownedMesh = true;
		return;
	}
	if (bShowingDownedMesh)
	{
		PoseMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		PoseMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		bShowingDownedMesh = false;
	}
}

void AFBBoxerCharacter::ConfigureBodyCollision()
{
	SetActorScale3D(FVector(BodyScale));

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->InitCapsuleSize(34.f, 90.f);
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionObjectType(ECC_Pawn);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bEnablePhysicsInteraction = true;
		Move->PushForceFactor = 1.2f;
		Move->InitialPushForceFactor = 3.f;
		Move->RepulsionForce = 1.4f;
		Move->bPushForceScaledToMass = false;
		Move->Mass = 88.f;
		Move->MaxStepHeight = 20.f;
		Move->bCanWalkOffLedges = false;
		// Ease into a step and out of it. The capsule still owns the move.
		// Root motion is not used, so these rates are the whole acceleration.
		Move->BrakingDecelerationWalking = 800.f;
		Move->BrakingFrictionFactor = 0.8f;
		Move->GroundFriction = 5.5f;
		Move->MaxAcceleration = 1100.f;
	}
}

float AFBBoxerCharacter::GetStandingHalfHeight() const
{
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		return Capsule->GetScaledCapsuleHalfHeight();
	}
	return 112.f;
}

float AFBBoxerCharacter::GetSyncedTimeSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}
	return 0.f;
}

bool AFBBoxerCharacter::IsHookFacingLocked() const
{
	return GetSyncedTimeSeconds() < FacingLockServerEndTime;
}

FVector AFBBoxerCharacter::GetHookLockDirection() const
{
	return FRotator(0.f, FacingLockYaw, 0.f).Vector();
}

float AFBBoxerCharacter::GetAnglePowerMultiplier() const
{
	if (GetSyncedTimeSeconds() < AngleBonusServerEndTime)
	{
		return AnglePowerMultiplier;
	}
	return 1.f;
}

void AFBBoxerCharacter::BeginHookFacingLock(float Seconds)
{
	if (!IsHookFacingLocked())
	{
		FacingLockYaw = GetActorRotation().Yaw;
	}
	FacingLockServerEndTime = FMath::Max(FacingLockServerEndTime, GetSyncedTimeSeconds() + FMath::Max(0.f, Seconds));
}

void AFBBoxerCharacter::ExtendHookFacingLock(float Seconds)
{
	FacingLockServerEndTime = FMath::Max(FacingLockServerEndTime, GetSyncedTimeSeconds() + FMath::Max(0.f, Seconds));
}

void AFBBoxerCharacter::GrantCreatedAngle(float Seconds)
{
	if (!HasAuthority())
	{
		return;
	}
	AngleBonusServerEndTime = FMath::Max(AngleBonusServerEndTime, GetSyncedTimeSeconds() + FMath::Max(0.f, Seconds));
	UE_LOG(LogTemp, Log, TEXT("FinalBell angle created on %s for %.2fs (x%.2f power)"),
		*GetName(), Seconds, AnglePowerMultiplier);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.4f, FColor::Yellow, TEXT("Angle created — counter has bonus power"));
	}
}

void AFBBoxerCharacter::OnRep_AngleBonus()
{
	if (!IsLocallyControlled() || GetSyncedTimeSeconds() >= AngleBonusServerEndTime)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.4f, FColor::Yellow, TEXT("Angle — bonus punch power"));
	}
}

FVector AFBBoxerCharacter::GetPunchVolumeCenter(EFBAttackId Attack) const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	FVector Center = GetActorLocation() + FVector(0.f, 0.f, 54.f);
	if (!Forward.IsNearlyZero())
	{
		Center += Forward * 68.f;
	}
	const bool bBody = CombatComponent && CombatComponent->IsBodyShot();
	switch (Attack)
	{
	case EFBAttackId::LeadHook:
		// Fist finishes in front, across the centerline, not outside the lead shoulder.
		Center += Forward * 8.f;
		Center += Right * 24.f;
		break;
	case EFBAttackId::RearHook:
		Center += Forward * 12.f;
		Center += Right * -24.f;
		break;
	case EFBAttackId::LeadUppercut:
	case EFBAttackId::RearUppercut:
		Center.Z += bBody ? 0.f : 24.f;
		break;
	case EFBAttackId::Cross:
		Center += Forward * 12.f;
		break;
	default:
		break;
	}
	if (bBody)
	{
		// Head line sits about 54cm above the capsule center. The body line is the ribs.
		Center.Z -= 52.f;
	}
	return Center;
}

float AFBBoxerCharacter::GetPunchVolumeRadius(EFBAttackId Attack) const
{
	switch (Attack)
	{
	case EFBAttackId::LeadHook:
		return 48.f;
	case EFBAttackId::RearHook:
		return 50.f;
	default:
		return 40.f;
	}
}

bool AFBBoxerCharacter::DoesPunchConnect(EFBAttackId Attack, AActor* Target) const
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return false;
	}
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		return false;
	}

	const FVector Center = GetPunchVolumeCenter(Attack);
	const float Radius = GetPunchVolumeRadius(Attack);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FBPunchVolume), false, this);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Hits;
	if (!World->OverlapMultiByChannel(Hits, Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params))
	{
		return false;
	}
	for (const FOverlapResult& Hit : Hits)
	{
		if (Hit.GetActor() == Target)
		{
			return true;
		}
	}
	return false;
}

FVector AFBBoxerCharacter::GetGlovePoint(EFBAttackId Attack, float Alpha) const
{
	const FVector Extended = GetPunchVolumeCenter(Attack);
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const bool bBody = CombatComponent && CombatComponent->IsBodyShot();
	FVector Retracted = GetActorLocation() + FVector(0.f, 0.f, bBody ? 8.f : 50.f);
	if (!Forward.IsNearlyZero())
	{
		Retracted += Forward * 28.f;
	}
	return FMath::Lerp(Retracted, Extended, FMath::Clamp(Alpha, 0.f, 1.f));
}

bool AFBBoxerCharacter::SweepGlove(AActor* Target, const FVector& From, const FVector& To, float Radius, bool bBodyShot, bool bGuardCounts, bool& bOutBlocked) const
{
	bOutBlocked = false;
	UWorld* World = GetWorld();
	const AFBBoxerCharacter* Defender = Cast<AFBBoxerCharacter>(Target);
	if (!World || !Defender)
	{
		return false;
	}

	DrawContactDebug(From, To, Radius);

	FVector End = To;
	if (FVector::DistSquared(From, To) < 1.f)
	{
		End = To + FVector(0.f, 0.f, 1.f);
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FBGloveSweep), false, this);
	Params.AddIgnoredActor(this);
	TArray<FHitResult> Hits;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	World->SweepMultiByChannel(Hits, From, End, FQuat::Identity, ECC_Visibility, Sphere, Params);

	const UFBCombatComponent* DefenderCombat = Defender->GetCombatComponent();
	const EFBGuardState Guard = DefenderCombat ? DefenderCombat->GetGuardState() : EFBGuardState::Open;
	Hits.Sort([](const FHitResult& A, const FHitResult& B)
	{
		return A.Time < B.Time;
	});

	bool bClean = false;
	for (const FHitResult& Hit : Hits)
	{
		if (Hit.GetActor() != Defender)
		{
			continue;
		}
		const UPrimitiveComponent* Comp = Hit.GetComponent();
		if (!Comp || Comp == Defender->GetCapsuleComponent())
		{
			continue;
		}
		const bool bHead = Comp == Defender->HeadVolume;
		const bool bTorso = Comp == Defender->TorsoVolume;
		const bool bGuard = Comp == Defender->GuardVolume;
		if (bGuard)
		{
			if (!bGuardCounts || Guard == EFBGuardState::Open || Guard == EFBGuardState::Weave)
			{
				continue;
			}
			if (bBodyShot && Guard == EFBGuardState::High)
			{
				continue;
			}
			if (!bBodyShot && Guard == EFBGuardState::Low)
			{
				continue;
			}
			bOutBlocked = true;
			return true;
		}
		if (bHead && !bBodyShot)
		{
			bClean = true;
			break;
		}
		if (bTorso && bBodyShot)
		{
			bClean = true;
			break;
		}
	}
	return bClean;
}

void AFBBoxerCharacter::DrawContactDebug(const FVector& From, const FVector& To, float Radius) const
{
#if UE_BUILD_SHIPPING
	(void)From;
	(void)To;
	(void)Radius;
#else
	if (CVarFBDebugContact.GetValueOnGameThread() <= 0 || !GetWorld())
	{
		return;
	}
	DrawDebugSphere(GetWorld(), From, Radius, 12, FColor::Silver, false, 0.f, 0, 0.6f);
	DrawDebugSphere(GetWorld(), To, Radius, 12, FColor::White, false, 0.f, 0, 0.8f);
	DrawDebugLine(GetWorld(), From, To, FColor::White, false, 0.f, 0, 0.5f);
	auto Mark = [this](const USphereComponent* Volume, const FColor& Color)
	{
		if (!Volume || Volume->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			return;
		}
		DrawDebugSphere(GetWorld(), Volume->GetComponentLocation(), Volume->GetScaledSphereRadius(), 10, Color, false, 0.f, 0, 0.5f);
	};
	Mark(HeadVolume, FColor::Cyan);
	Mark(TorsoVolume, FColor::Green);
	Mark(GuardVolume, FColor::Yellow);
#endif
}

void AFBBoxerCharacter::UpdateQueryVolumes()
{
	if (!GuardVolume)
	{
		return;
	}
	const EFBGuardState Guard = CombatComponent ? CombatComponent->GetGuardState() : EFBGuardState::Open;
	if (Guard == EFBGuardState::High)
	{
		GuardVolume->SetRelativeLocation(FVector(26.f, 0.f, 58.f));
		GuardVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else if (Guard == EFBGuardState::Low)
	{
		GuardVolume->SetRelativeLocation(FVector(26.f, 0.f, -6.f));
		GuardVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		GuardVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AFBBoxerCharacter::ServerSetFacingOffset_Implementation(float OffsetDegrees)
{
	const float FaceLimit = bLooseMovement ? 72.f : MaxFacingOffsetDegrees;
	FacingYawOffset = FMath::Clamp(OffsetDegrees, -FaceLimit, FaceLimit);
}

FName AFBBoxerCharacter::LeadHandSocket() const
{
	return (Stance == EFBStance::Southpaw) ? FName(TEXT("hand_r")) : FName(TEXT("hand_l"));
}

FName AFBBoxerCharacter::RearHandSocket() const
{
	return (Stance == EFBStance::Southpaw) ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"));
}

void AFBBoxerCharacter::AttachGlovesToCapsule()
{
	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, false);
	if (LeadGloveMesh)
	{
		LeadGloveMesh->AttachToComponent(GetCapsuleComponent(), Rules);
	}
	if (RearGloveMesh)
	{
		RearGloveMesh->AttachToComponent(GetCapsuleComponent(), Rules);
	}
}

void AFBBoxerCharacter::AttachGlovesToHands()
{
	if (!PoseMesh || !bUsingMannequin)
	{
		return;
	}

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	auto Seat = [&](UStaticMeshComponent* Glove, FName Socket)
	{
		if (!Glove)
		{
			return;
		}
		Glove->AttachToComponent(PoseMesh, Rules, Socket);
		Glove->SetRelativeLocation(FVector(10.f, 0.f, 0.f));
		Glove->SetRelativeRotation(FRotator::ZeroRotator);
		Glove->SetRelativeScale3D(FVector(0.20f));
		Glove->SetVisibility(true);
		Glove->SetHiddenInGame(false);
	};

	Seat(LeadGloveMesh, LeadHandSocket());
	Seat(RearGloveMesh, RearHandSocket());
}

void AFBBoxerCharacter::HideShapeBody(bool bHideShapes)
{
	auto Show = [bHideShapes](UStaticMeshComponent* MeshComp)
	{
		if (MeshComp)
		{
			MeshComp->SetVisibility(!bHideShapes);
			MeshComp->SetHiddenInGame(bHideShapes);
		}
	};
	Show(BodyMesh);
	Show(HeadMesh);
	Show(TrunksMesh);
	Show(LeadUpperArmMesh);
	Show(LeadForearmMesh);
	Show(RearUpperArmMesh);
	Show(RearForearmMesh);
	Show(LeadThighMesh);
	Show(LeadShinMesh);
	Show(RearThighMesh);
	Show(RearShinMesh);
	Show(LeadShoeMesh);
	Show(RearShoeMesh);

	if (LeadGloveMesh)
	{
		LeadGloveMesh->SetVisibility(true);
		LeadGloveMesh->SetHiddenInGame(false);
	}
	if (RearGloveMesh)
	{
		RearGloveMesh->SetVisibility(true);
		RearGloveMesh->SetHiddenInGame(false);
	}
}

bool AFBBoxerCharacter::TryActivateSkinnedMesh()
{
	if (!PoseMesh)
	{
		return false;
	}

	USkeletalMesh* Asset = (CornerSkin == EFBCornerSkin::RedCorner) ? QuinnMesh.Get() : MannyMesh.Get();
	if (!Asset)
	{
		Asset = MannyMesh ? MannyMesh.Get() : QuinnMesh.Get();
	}
	if (!Asset)
	{
		return false;
	}

	// The copied mannequin still names its skeleton at /Game/Characters/Mannequins.
	// The file on disk is /Game/Mannequins/Meshes/SK_Mannequin. Posing without that
	// skeleton asserts inside the bone container.
	if (!Asset->GetSkeleton())
	{
		USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, TEXT("/Game/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
		if (Skeleton)
		{
			Asset->SetSkeleton(Skeleton);
		}
	}
	if (!Asset->GetSkeleton())
	{
		return false;
	}

	if (PoseMesh->GetSkinnedAsset() != Asset)
	{
		PoseMesh->SetSkinnedAssetAndUpdate(Asset, true);
		SkinnedRefLocals = Asset->GetRefSkeleton().GetRefBonePose();
	}

	if (SkinnedRefLocals.Num() == 0 || PoseMesh->GetBoneIndex(FName(TEXT("upperarm_l"))) == INDEX_NONE
		|| PoseMesh->GetBoneIndex(FName(TEXT("hand_r"))) == INDEX_NONE
		|| PoseMesh->GetBoneIndex(FName(TEXT("foot_l"))) == INDEX_NONE)
	{
		SkinnedRefLocals.Reset();
		return false;
	}

	if (PoseMesh->BoneSpaceTransforms.Num() != SkinnedRefLocals.Num())
	{
		PoseMesh->AllocateTransformData();
	}

	PoseMesh->SetVisibility(true);
	PoseMesh->SetHiddenInGame(false);
	PoseMesh->SetCastShadow(true);
	return PoseMesh->BoneSpaceTransforms.Num() == SkinnedRefLocals.Num();
}

void AFBBoxerCharacter::SetupMannequinVisuals()
{
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetSkeletalMesh(nullptr);
		Skel->SetVisibility(false);
		Skel->SetHiddenInGame(true);
		Skel->SetCastShadow(false);
		Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TryActivateSkinnedMesh())
	{
		bUsingMannequin = true;
		HideShapeBody(true);
		AttachGlovesToHands();
		return;
	}

	bUsingMannequin = false;
	SkinnedRefLocals.Reset();
	if (PoseMesh)
	{
		PoseMesh->SetVisibility(false);
		PoseMesh->SetHiddenInGame(true);
	}
	HideShapeBody(false);
	AttachGlovesToCapsule();
}

void AFBBoxerCharacter::TintMesh(UStaticMeshComponent* MeshComp, const FLinearColor& Color)
{
	if (!MeshComp)
	{
		return;
	}
	UMaterialInterface* Base = MeshComp->GetMaterial(0);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Base))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}

void AFBBoxerCharacter::TintSkeletalMesh(const FLinearColor& Tint)
{
	USkeletalMeshComponent* Skel = GetMesh();
	if (!Skel)
	{
		return;
	}

	const int32 NumMats = Skel->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		UMaterialInterface* Base = Skel->GetMaterial(i);
		if (!Base)
		{
			continue;
		}
		if (UMaterialInstanceDynamic* MID = Skel->CreateAndSetMaterialInstanceDynamicFromMaterial(i, Base))
		{
			MID->SetVectorParameterValue(TEXT("Tint"), Tint);
			MID->SetVectorParameterValue(TEXT("Paint Color"), Tint);
		}
	}
}

void AFBBoxerCharacter::ApplyBasicSkin(EFBCornerSkin Corner)
{
	CornerSkin = Corner;
	const bool bRed = Corner == EFBCornerSkin::RedCorner;
	const FLinearColor Skin(0.86f, 0.62f, 0.46f);
	const FLinearColor Trunks = bRed ? FLinearColor(0.82f, 0.08f, 0.08f) : FLinearColor(0.08f, 0.28f, 0.82f);
	const FLinearColor Gloves = bRed ? FLinearColor(0.9f, 0.12f, 0.08f) : FLinearColor(0.1f, 0.35f, 0.95f);
	const FLinearColor Shoes(0.08f, 0.08f, 0.09f);

	SetupMannequinVisuals();

	TintMesh(BodyMesh, Skin);
	TintMesh(HeadMesh, Skin);
	TintMesh(LeadUpperArmMesh, Skin);
	TintMesh(LeadForearmMesh, Skin);
	TintMesh(RearUpperArmMesh, Skin);
	TintMesh(RearForearmMesh, Skin);
	TintMesh(LeadThighMesh, Skin);
	TintMesh(LeadShinMesh, Skin);
	TintMesh(RearThighMesh, Skin);
	TintMesh(RearShinMesh, Skin);
	TintMesh(TrunksMesh, Trunks);
	TintMesh(LeadGloveMesh, Gloves);
	TintMesh(RearGloveMesh, Gloves);
	TintMesh(LeadShoeMesh, Shoes);
	TintMesh(RearShoeMesh, Shoes);
}

void AFBBoxerCharacter::ApplyPlaceholderAppearance(bool bIsOpponent)
{
	ApplyBasicSkin(bIsOpponent ? EFBCornerSkin::RedCorner : EFBCornerSkin::BlueCorner);
}

void AFBBoxerCharacter::RefreshCombatPresentation()
{
	UpdateQueryVolumes();
}

void AFBBoxerCharacter::UpdateDisplayedPunch(float DeltaSeconds)
{
	const EFBAttackPhase Phase = CombatComponent ? CombatComponent->GetAttackPhase() : EFBAttackPhase::Idle;
	const EFBAttackId LiveAttack = CombatComponent ? CombatComponent->GetActiveAttack() : EFBAttackId::None;
	if (LiveAttack != EFBAttackId::None)
	{
		DisplayAttack = LiveAttack;
		bDisplayBody = CombatComponent->IsBodyShot();
	}

	float TargetAlpha = 0.f;
	if (Phase == EFBAttackPhase::Startup)
	{
		TargetAlpha = 0.55f;
	}
	else if (Phase == EFBAttackPhase::Active)
	{
		TargetAlpha = 1.f;
	}
	PunchPoseAlpha = FMath::FInterpTo(PunchPoseAlpha, TargetAlpha, DeltaSeconds, TargetAlpha > PunchPoseAlpha ? 18.f : 8.f);
	if (LiveAttack == EFBAttackId::None && PunchPoseAlpha < 0.04f)
	{
		DisplayAttack = EFBAttackId::None;
		bDisplayBody = false;
	}
}

void AFBBoxerCharacter::UpdateMovementYawLag(float DeltaSeconds)
{
	const float Yaw = GetActorRotation().Yaw;
	if (!bHasMovementYaw)
	{
		LastMovementYaw = Yaw;
		bHasMovementYaw = true;
	}

	const float YawDelta = FMath::FindDeltaAngleDegrees(LastMovementYaw, Yaw);
	LastMovementYaw = Yaw;
	MovementYawLag = FMath::FInterpTo(MovementYawLag, 0.f, DeltaSeconds, 4.5f);
	MovementYawLag = FMath::Clamp(MovementYawLag + YawDelta, -28.f, 28.f);
}

void AFBBoxerCharacter::UpdateDefensePose(float DeltaSeconds)
{
	float TargetPitch = 0.f;
	float TargetRoll = 0.f;
	const EFBGuardState Guard = CombatComponent ? CombatComponent->GetGuardState() : EFBGuardState::Open;
	const bool bSlip = GetSyncedTimeSeconds() < DefenseSlipEndTime;
	if (bSlip)
	{
		TargetPitch = DefenseSlipPitch;
		TargetRoll = DefenseSlipRoll;
	}
	else if (Guard == EFBGuardState::Weave)
	{
		// The owner still has the stick. Everyone else reads the step that already replicated.
		const float Away = IsLocallyControlled() ? CachedLeftStick.Y : SmoothedMoveForward;
		const float Side = IsLocallyControlled() ? CachedLeftStick.X : SmoothedMoveRight;
		const float LeadX = (Stance == EFBStance::Southpaw) ? 1.f : -1.f;
		if (Away < -0.35f)
		{
			TargetPitch = LeanBackDegrees;
			TargetRoll = -Side * 14.f;
		}
		else
		{
			TargetPitch = RollUnderDegrees;
			const float Drop = FMath::Abs(Side) > 0.35f ? Side : LeadX;
			TargetRoll = -Drop * RollUnderSideDegrees;
		}
	}

	const float Ease = (bSlip || Guard == EFBGuardState::Weave) ? 16.f : 7.f;
	ShownDefensePitch = FMath::FInterpTo(ShownDefensePitch, TargetPitch, DeltaSeconds, Ease);
	ShownDefenseRoll = FMath::FInterpTo(ShownDefenseRoll, TargetRoll, DeltaSeconds, Ease);
}

void AFBBoxerCharacter::UpdateBoxerPose(float DeltaSeconds)
{
	UpdateDisplayedPunch(DeltaSeconds);
	UpdateMovementYawLag(DeltaSeconds);
	UpdateDefensePose(DeltaSeconds);
	PoseClock += DeltaSeconds;
	const FVector LocalVelocity = GetActorRotation().UnrotateVector(GetVelocity());
	SmoothedMoveForward = FMath::FInterpTo(SmoothedMoveForward, FMath::Clamp(LocalVelocity.X / 320.f, -1.f, 1.f), DeltaSeconds, 3.5f);
	SmoothedMoveRight = FMath::FInterpTo(SmoothedMoveRight, FMath::Clamp(LocalVelocity.Y / 320.f, -1.f, 1.f), DeltaSeconds, 3.5f);
	if (bUsingMannequin)
	{
		ApplySkinnedPose(DeltaSeconds);
	}
	else
	{
		ApplyShapePose();
	}
}

void AFBBoxerCharacter::ApplyShapePose()
{
	auto Side = [this](FVector Point)
	{
		if (Stance == EFBStance::Southpaw)
		{
			Point.Y = -Point.Y;
		}
		return Point;
	};
	auto PlaceBone = [](UStaticMeshComponent* BoneMesh, const FVector& A, const FVector& B, float Radius)
	{
		if (!BoneMesh)
		{
			return;
		}
		const FVector Delta = B - A;
		const float Len = FMath::Max(Delta.Size(), 1.f);
		BoneMesh->SetRelativeLocation((A + B) * 0.5f);
		BoneMesh->SetRelativeRotation(UKismetMathLibrary::MakeRotFromZ(Delta));
		BoneMesh->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Len / 100.f));
	};
	auto PlaceBall = [](UStaticMeshComponent* BallMesh, const FVector& At, float Scale)
	{
		if (!BallMesh)
		{
			return;
		}
		BallMesh->SetRelativeLocation(At);
		BallMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BallMesh->SetRelativeScale3D(FVector(Scale));
	};

	const EFBGuardState Guard = CombatComponent ? CombatComponent->GetGuardState() : EFBGuardState::Open;

	const FVector LeadShoulder = Side(FVector(4.f, -18.f, 30.f));
	const FVector RearShoulder = Side(FVector(0.f, 18.f, 30.f));
	FVector LeadElbow = Side(FVector(12.f, -24.f, 16.f));
	FVector RearElbow = Side(FVector(4.f, 22.f, 14.f));
	FVector LeadHand = Side(FVector(34.f, -14.f, 42.f));
	FVector RearHand = Side(FVector(22.f, 12.f, 40.f));
	FVector Head = Side(FVector(4.f, 0.f, 52.f));

	if (Guard == EFBGuardState::High)
	{
		LeadHand.Z += 8.f;
		RearHand.Z += 8.f;
		LeadHand.X -= 6.f;
		RearHand.X -= 4.f;
	}
	else if (Guard == EFBGuardState::Low)
	{
		LeadHand.Z -= 28.f;
		RearHand.Z -= 28.f;
		LeadHand.X += 8.f;
		RearHand.X += 6.f;
	}
	Head.Z -= ShownDefensePitch * 0.55f;
	// Positive roll is the fighter's left in both stances. Left is -Y on the capsule.
	Head.Y -= ShownDefenseRoll * 0.62f;
	if (ShownDefensePitch > 8.f)
	{
		const float Drop = FMath::Min(ShownDefensePitch, 74.f) * 0.35f;
		LeadHand.Z -= Drop;
		RearHand.Z -= Drop;
	}
	else if (ShownDefensePitch < -8.f)
	{
		const float Lift = FMath::Min(-ShownDefensePitch, 58.f) * 0.18f;
		LeadHand.Z += Lift;
		RearHand.Z += Lift;
	}
	LeadHand.Y -= ShownDefenseRoll * 0.2f;
	RearHand.Y -= ShownDefenseRoll * 0.2f;

	auto PunchArm = [&](bool bLead, const FVector& ElbowOut, const FVector& HandOut)
	{
		if (bLead)
		{
			LeadElbow = FMath::Lerp(LeadElbow, Side(ElbowOut), PunchPoseAlpha);
			LeadHand = FMath::Lerp(LeadHand, Side(HandOut), PunchPoseAlpha);
		}
		else
		{
			RearElbow = FMath::Lerp(RearElbow, Side(ElbowOut), PunchPoseAlpha);
			RearHand = FMath::Lerp(RearHand, Side(HandOut), PunchPoseAlpha);
		}
	};

	switch (DisplayAttack)
	{
	case EFBAttackId::Jab:
		PunchArm(true, bDisplayBody ? FVector(36.f, -10.f, -8.f) : FVector(40.f, -12.f, 32.f), bDisplayBody ? FVector(70.f, -4.f, -6.f) : FVector(82.f, -6.f, 44.f));
		break;
	case EFBAttackId::Cross:
		PunchArm(false, bDisplayBody ? FVector(32.f, 18.f, -10.f) : FVector(36.f, 22.f, 28.f), bDisplayBody ? FVector(78.f, 14.f, -4.f) : FVector(96.f, 20.f, 46.f));
		break;
	case EFBAttackId::LeadHook:
		PunchArm(true, bDisplayBody ? FVector(16.f, -22.f, -12.f) : FVector(20.f, -16.f, 28.f), bDisplayBody ? FVector(30.f, 26.f, -8.f) : FVector(36.f, 32.f, 42.f));
		break;
	case EFBAttackId::RearHook:
		PunchArm(false, bDisplayBody ? FVector(22.f, 22.f, -12.f) : FVector(28.f, 26.f, 30.f), bDisplayBody ? FVector(40.f, -28.f, -6.f) : FVector(52.f, -36.f, 46.f));
		break;
	case EFBAttackId::LeadUppercut:
		PunchArm(true, bDisplayBody ? FVector(16.f, -16.f, -20.f) : FVector(18.f, -18.f, 2.f), bDisplayBody ? FVector(34.f, -2.f, 8.f) : FVector(40.f, -4.f, 68.f));
		break;
	case EFBAttackId::RearUppercut:
		PunchArm(false, bDisplayBody ? FVector(20.f, 20.f, -22.f) : FVector(24.f, 24.f, 0.f), bDisplayBody ? FVector(42.f, 10.f, 10.f) : FVector(58.f, 16.f, 78.f));
		break;
	default:
		break;
	}

	const float GuardLife = 1.f - PunchPoseAlpha;
	const float Breath = FMath::Sin(PoseClock * 1.7f);
	const float Sway = FMath::Sin(PoseClock * 1.15f);
	const float LeadSign = (Stance == EFBStance::Southpaw) ? 1.f : -1.f;
	LeadHand.Z += (Breath * 3.5f + SmoothedMoveForward * -2.f) * GuardLife;
	RearHand.Z += Breath * 2.2f * GuardLife;
	LeadHand.Y += LeadSign * (Sway * 4.f + SmoothedMoveRight * 3.f) * GuardLife;
	RearHand.Y -= LeadSign * Sway * 2.f * GuardLife;
	Head.Z += Breath * 1.4f * GuardLife;
	Head.Y += LeadSign * Sway * 1.6f * GuardLife;
	if (GetWorld() && GetWorld()->GetTimeSeconds() < TauntEndTime)
	{
		LeadHand.Z += 26.f;
		RearHand.Z += 26.f;
		LeadHand.Y += LeadSign * 18.f;
		RearHand.Y -= LeadSign * 18.f;
	}

	const float ShapeLead = (Stance == EFBStance::Southpaw) ? -1.f : 1.f;
	const float ShapeStep = SmoothedMoveForward * 6.f * GuardLife * ShapeLead;
	const FFBWaistTwist ShapeTwist = BuildWaistTwist(ComputeDisplayedPunchYaw() + ShapeStep, FacingYawOffset + MovementYawLag, HurtYaw);
	if (BodyMesh)
	{
		const float BodyDip = bDisplayBody ? 16.f * PunchPoseAlpha : 0.f;
		const float Chest = Breath * 3.5f * GuardLife;
		const float DefenseWeight = FMath::Clamp(FMath::Max(FMath::Abs(ShownDefensePitch) / 74.f, FMath::Abs(ShownDefenseRoll) / 48.f), 0.f, 1.f);
		const float Roll = (Sway * 7.f - SmoothedMoveRight * 8.f) * GuardLife * (1.f - 0.8f * DefenseWeight);
		const float ChestYaw = ShapeTwist.WaistYaw + ShapeTwist.MidYaw + ShapeTwist.ChestYaw;
		const float DefenseDrop = FMath::Max(0.f, ShownDefensePitch) * 0.12f;
		BodyMesh->SetRelativeLocation(FVector(2.f + BodyDip * 0.4f, Roll * 0.15f, 8.f - DefenseDrop - BodyDip * 0.15f + Chest * 0.08f));
		BodyMesh->SetRelativeRotation(FRotator(BodyDip + Chest + HurtLean + ShownDefensePitch * 0.78f, ChestYaw, Roll + ShownDefenseRoll * 0.78f));
		BodyMesh->SetRelativeScale3D(FVector(0.26f, 0.38f, 0.48f));
	}
	if (TrunksMesh)
	{
		TrunksMesh->SetRelativeLocation(FVector(0.f, 0.f, -28.f));
		TrunksMesh->SetRelativeRotation(FRotator(ShownDefensePitch * 0.22f, ShapeTwist.PelvisYaw, ShownDefenseRoll * 0.22f));
		TrunksMesh->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.18f));
	}
	const float HeadScale = 0.30f;
	PlaceBall(HeadMesh, Head, HeadScale);
	PlaceBall(LeadGloveMesh, LeadHand, 0.30f);
	PlaceBall(RearGloveMesh, RearHand, 0.30f);

	PlaceBone(LeadUpperArmMesh, LeadShoulder, LeadElbow, 7.f);
	PlaceBone(LeadForearmMesh, LeadElbow, LeadHand, 5.5f);
	PlaceBone(RearUpperArmMesh, RearShoulder, RearElbow, 7.f);
	PlaceBone(RearForearmMesh, RearElbow, RearHand, 5.5f);

	const FVector LeadHip = Side(FVector(8.f, -9.f, -22.f));
	const FVector RearHip = Side(FVector(-4.f, 9.f, -24.f));
	const FVector LeadKnee = Side(FVector(18.f, -12.f, -54.f));
	const FVector RearKnee = Side(FVector(-6.f, 10.f, -54.f));
	const FVector LeadAnkle = Side(FVector(20.f, -14.f, -84.f));
	const FVector RearAnkle = Side(FVector(-8.f, 12.f, -84.f));
	PlaceBone(LeadThighMesh, LeadHip, LeadKnee, 9.f);
	PlaceBone(LeadShinMesh, LeadKnee, LeadAnkle, 6.5f);
	PlaceBone(RearThighMesh, RearHip, RearKnee, 9.f);
	PlaceBone(RearShinMesh, RearKnee, RearAnkle, 6.5f);

	if (LeadShoeMesh)
	{
		LeadShoeMesh->SetRelativeLocation(LeadAnkle + Side(FVector(8.f, 0.f, -3.f)));
		LeadShoeMesh->SetRelativeScale3D(FVector(0.26f, 0.12f, 0.08f));
	}
	if (RearShoeMesh)
	{
		RearShoeMesh->SetRelativeLocation(RearAnkle + Side(FVector(8.f, 0.f, -3.f)));
		RearShoeMesh->SetRelativeScale3D(FVector(0.26f, 0.12f, 0.08f));
	}
}

void AFBBoxerCharacter::ApplySkinnedPose(float DeltaSeconds)
{
	if (!PoseMesh || SkinnedRefLocals.Num() == 0 || PoseMesh->GetSkinnedAsset() == nullptr)
	{
		return;
	}

	const FReferenceSkeleton& Skel = PoseMesh->GetSkinnedAsset()->GetRefSkeleton();
	if (PoseMesh->BoneSpaceTransforms.Num() != SkinnedRefLocals.Num())
	{
		PoseMesh->AllocateTransformData();
	}
	if (PoseMesh->BoneSpaceTransforms.Num() != SkinnedRefLocals.Num())
	{
		return;
	}

	auto Side = [this](FVector Point)
	{
		if (Stance == EFBStance::Southpaw)
		{
			Point.Y = -Point.Y;
		}
		return Point;
	};
	auto Bone = [this](const TCHAR* Name)
	{
		return PoseMesh->GetBoneIndex(FName(Name));
	};
	auto ToMesh = [this](const FVector& CapsulePoint)
	{
		return PoseMesh->GetRelativeTransform().InverseTransformPosition(CapsulePoint);
	};
	auto ToCapsule = [this](const FVector& MeshPoint)
	{
		return PoseMesh->GetRelativeTransform().TransformPosition(MeshPoint);
	};

	const bool bSouthpaw = Stance == EFBStance::Southpaw;
	const float LeadYaw = bSouthpaw ? -1.f : 1.f;
	const EFBGuardState Guard = CombatComponent ? CombatComponent->GetGuardState() : EFBGuardState::Open;
	const bool bPower = CombatComponent && CombatComponent->IsPowerShot() && DisplayAttack != EFBAttackId::None;

	TArray<FTransform> Local = SkinnedRefLocals;
	TArray<FTransform> ComponentSpace;
	RebuildComponentPose(Skel, Local, ComponentSpace);

	const int32 Pelvis = Bone(TEXT("pelvis"));
	const int32 SpineA = Bone(TEXT("spine_01"));
	const int32 SpineB = Bone(TEXT("spine_03"));
	const int32 Neck = Bone(TEXT("neck_01"));
	const int32 Head = Bone(TEXT("head"));

	float Lean = 8.f;
	float Chin = 16.f;
	if (ShownDefensePitch > 12.f)
	{
		Chin += 22.f;
	}
	else if (ShownDefensePitch < -12.f)
	{
		Chin -= 14.f;
	}
	switch (DisplayAttack)
	{
	case EFBAttackId::Jab: Lean += 4.f; break;
	case EFBAttackId::Cross: Lean += 8.f; break;
	case EFBAttackId::LeadUppercut: Lean += 10.f; break;
	case EFBAttackId::RearUppercut: Lean += 12.f; break;
	default: break;
	}
	if (bDisplayBody)
	{
		Lean += 34.f * PunchPoseAlpha;
		Chin -= 14.f * PunchPoseAlpha;
	}

	const float GuardLife = FMath::Lerp(1.f, 0.22f, PunchPoseAlpha) * (Guard == EFBGuardState::Weave ? 0.35f : 1.f);
	const float Breath = FMath::Sin(PoseClock * 1.7f) * 3.6f * GuardLife;
	const float DefenseWeight = FMath::Clamp(FMath::Max(FMath::Abs(ShownDefensePitch) / 74.f, FMath::Abs(ShownDefenseRoll) / 48.f), 0.f, 1.f);
	const float ShoulderRoll = (FMath::Sin(PoseClock * 1.15f) * 7.f - SmoothedMoveRight * 9.f) * GuardLife * LeadYaw * (1.f - 0.8f * DefenseWeight);
	const float StepYaw = (SmoothedMoveForward * 7.f + FMath::Sin(PoseClock * 0.85f) * 3.2f) * GuardLife * LeadYaw;
	const float TorsoLean = Lean + Breath + HurtLean;
	const FFBWaistTwist Twist = BuildWaistTwist(ComputeDisplayedPunchYaw() + StepYaw, FacingYawOffset + MovementYawLag, HurtYaw);
	const FFBDefenseBend Bend = BuildDefenseBend(ShownDefensePitch, ShownDefenseRoll);

	const int32 SpineM = Bone(TEXT("spine_02"));
	const bool bHasMidSpine = SpineM != INDEX_NONE;
	float WaistYaw = Twist.WaistYaw;
	float MidYaw = Twist.MidYaw;
	float DefWaistPitch = Bend.WaistPitch;
	float DefMidPitch = Bend.MidPitch;
	float DefWaistRoll = Bend.WaistRoll;
	float DefMidRoll = Bend.MidRoll;
	if (!bHasMidSpine)
	{
		// This mesh has no mid spine, so the waist bone keeps that share.
		WaistYaw += MidYaw;
		MidYaw = 0.f;
		DefWaistPitch += DefMidPitch;
		DefMidPitch = 0.f;
		DefWaistRoll += DefMidRoll;
		DefMidRoll = 0.f;
	}

	float HeadLook = -ShoulderRoll * 0.45f;
	if (const AActor* LookTarget = FindCombatTarget())
	{
		const FVector ToTarget = GetActorTransform().InverseTransformVectorNoScale(LookTarget->GetActorLocation() - GetActorLocation());
		const float DesiredLook = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, FMath::Max(ToTarget.X, 1.f))), -14.f, 14.f);
		SmoothedHeadYaw = FMath::FInterpTo(SmoothedHeadYaw, DesiredLook, DeltaSeconds, 4.f);
		HeadLook += SmoothedHeadYaw * GuardLife;
	}

	CommitComponentRotation(Skel, Pelvis, FQuat(MeshRight, FMath::DegreesToRadians(Bend.PelvisPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(14.f * LeadYaw + Twist.PelvisYaw)) * FQuat(MeshForward, FMath::DegreesToRadians(Bend.PelvisRoll)), Local, ComponentSpace);

	const float LowShare = bHasMidSpine ? 0.28f : 0.45f;
	const float MidShare = bHasMidSpine ? 0.32f : 0.f;
	const float HighShare = bHasMidSpine ? 0.40f : 0.55f;
	CommitComponentRotation(Skel, SpineA, FQuat(MeshRight, FMath::DegreesToRadians(TorsoLean * LowShare + HurtLean * 0.35f + DefWaistPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(WaistYaw)) * FQuat(MeshForward, FMath::DegreesToRadians(ShoulderRoll * 0.25f + DefWaistRoll)), Local, ComponentSpace);
	if (bHasMidSpine)
	{
		CommitComponentRotation(Skel, SpineM, FQuat(MeshRight, FMath::DegreesToRadians(TorsoLean * MidShare + DefMidPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(MidYaw)) * FQuat(MeshForward, FMath::DegreesToRadians(ShoulderRoll * 0.35f + DefMidRoll)), Local, ComponentSpace);
	}
	CommitComponentRotation(Skel, SpineB, FQuat(MeshRight, FMath::DegreesToRadians(TorsoLean * HighShare + Bend.ChestPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(Twist.ChestYaw)) * FQuat(MeshForward, FMath::DegreesToRadians(ShoulderRoll * 0.4f + Bend.ChestRoll)), Local, ComponentSpace);
	const float SpineYaw = WaistYaw + MidYaw + Twist.ChestYaw;
	CommitComponentRotation(Skel, Neck, FQuat(MeshRight, FMath::DegreesToRadians(Chin * 0.35f + Bend.NeckPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(-SpineYaw * 0.55f)) * FQuat(MeshForward, FMath::DegreesToRadians(-ShoulderRoll * 0.2f + Bend.NeckRoll)), Local, ComponentSpace);
	CommitComponentRotation(Skel, Head, FQuat(MeshRight, FMath::DegreesToRadians(Chin * 0.65f + Breath * 0.35f + HurtHeadPitch + Bend.HeadPitch)) * FQuat(MeshUp, FMath::DegreesToRadians(HeadLook)) * FQuat(MeshForward, FMath::DegreesToRadians(Bend.HeadRoll)), Local, ComponentSpace);

	const int32 LeftThigh = Bone(TEXT("thigh_l"));
	const int32 LeftCalf = Bone(TEXT("calf_l"));
	const int32 LeftFoot = Bone(TEXT("foot_l"));
	const int32 RightThigh = Bone(TEXT("thigh_r"));
	const int32 RightCalf = Bone(TEXT("calf_r"));
	const int32 RightFoot = Bone(TEXT("foot_r"));
	const int32 LeadThigh = bSouthpaw ? RightThigh : LeftThigh;
	const int32 LeadCalf = bSouthpaw ? RightCalf : LeftCalf;
	const int32 LeadFootBone = bSouthpaw ? RightFoot : LeftFoot;
	const int32 RearThigh = bSouthpaw ? LeftThigh : RightThigh;
	const int32 RearCalf = bSouthpaw ? LeftCalf : RightCalf;
	const int32 RearFootBone = bSouthpaw ? LeftFoot : RightFoot;

	SolveLimb(Skel, LeadThigh, LeadCalf, LeadFootBone, ToMesh(Side(FVector(22.f, -16.f, -82.f))), ToMesh(Side(FVector(52.f, -16.f, -30.f))), Local, ComponentSpace);
	SolveLimb(Skel, RearThigh, RearCalf, RearFootBone, ToMesh(Side(FVector(-18.f, 18.f, -82.f))), ToMesh(Side(FVector(8.f, 18.f, -30.f))), Local, ComponentSpace);

	const int32 LeftClav = Bone(TEXT("clavicle_l"));
	const int32 RightClav = Bone(TEXT("clavicle_r"));
	const int32 LeftUpper = Bone(TEXT("upperarm_l"));
	const int32 LeftLower = Bone(TEXT("lowerarm_l"));
	const int32 LeftHand = Bone(TEXT("hand_l"));
	const int32 RightUpper = Bone(TEXT("upperarm_r"));
	const int32 RightLower = Bone(TEXT("lowerarm_r"));
	const int32 RightHand = Bone(TEXT("hand_r"));
	const int32 LeadUpper = bSouthpaw ? RightUpper : LeftUpper;
	const int32 LeadLower = bSouthpaw ? RightLower : LeftLower;
	const int32 LeadHandBone = bSouthpaw ? RightHand : LeftHand;
	const int32 RearUpper = bSouthpaw ? LeftUpper : RightUpper;
	const int32 RearLower = bSouthpaw ? LeftLower : RightLower;
	const int32 RearHandBone = bSouthpaw ? LeftHand : RightHand;
	const int32 LeadClav = bSouthpaw ? RightClav : LeftClav;
	const int32 RearClav = bSouthpaw ? LeftClav : RightClav;

	auto NudgeClavicle = [&](int32 Clavicle, int32 Upper, float Forward, float Drop)
	{
		if ((FMath::Abs(Forward) < 0.15f && Drop < 0.5f) || !ComponentSpace.IsValidIndex(Clavicle) || !ComponentSpace.IsValidIndex(Upper))
		{
			return;
		}
		const FVector Target = ComponentSpace[Upper].GetLocation() + MeshForward * Forward - MeshUp * Drop;
		AimBoneAt(Skel, Clavicle, Upper, Target, Local, ComponentSpace);
	};
	const bool bLeadPunch = DisplayAttack == EFBAttackId::Jab || DisplayAttack == EFBAttackId::LeadHook || DisplayAttack == EFBAttackId::LeadUppercut;
	const bool bRearPunch = DisplayAttack == EFBAttackId::Cross || DisplayAttack == EFBAttackId::RearHook || DisplayAttack == EFBAttackId::RearUppercut;
	const float Duck = FMath::Max(0.f, ShownDefensePitch) * 0.10f;
	const float LeftDrop = FMath::Max(0.f, ShownDefenseRoll) * 0.35f + Duck;
	const float RightDrop = FMath::Max(0.f, -ShownDefenseRoll) * 0.35f + Duck;
	auto DropFor = [&](int32 Clavicle)
	{
		return Clavicle == LeftClav ? LeftDrop : RightDrop;
	};
	NudgeClavicle(LeadClav, LeadUpper, (bLeadPunch ? 6.f * PunchPoseAlpha : 0.f) + FMath::Max(0.f, FMath::Sin(PoseClock * 1.15f)) * 3.5f * (1.f - PunchPoseAlpha), DropFor(LeadClav));
	NudgeClavicle(RearClav, RearUpper, (bRearPunch ? 6.f * PunchPoseAlpha : 0.f) + FMath::Max(0.f, FMath::Sin(PoseClock * 1.15f + 2.4f)) * 3.5f * (1.f - PunchPoseAlpha), DropFor(RearClav));

	auto ShoulderCapsule = [&](int32 UpperBone)
	{
		return ComponentSpace.IsValidIndex(UpperBone) ? ToCapsule(ComponentSpace[UpperBone].GetLocation()) : FVector(0.f, 0.f, 40.f);
	};

	const FVector LeadShoulder = ShoulderCapsule(LeadUpper);
	const FVector RearShoulder = ShoulderCapsule(RearUpper);

	FVector LeadHand = LeadShoulder + Side(FVector(22.f, 6.f, -4.f));
	FVector RearHand = RearShoulder + Side(FVector(14.f, -4.f, -6.f));
	FVector LeadPole = LeadShoulder + Side(FVector(2.f, -26.f, -24.f));
	FVector RearPole = RearShoulder + Side(FVector(2.f, 26.f, -24.f));

	if (Guard == EFBGuardState::High)
	{
		LeadHand.Z += 8.f;
		RearHand.Z += 8.f;
		LeadHand.X -= 4.f;
		RearHand.X -= 3.f;
		LeadPole.Z += 6.f;
		RearPole.Z += 6.f;
	}
	else if (Guard == EFBGuardState::Low)
	{
		LeadHand.Z -= 28.f;
		RearHand.Z -= 28.f;
		LeadHand.X += 6.f;
		RearHand.X += 4.f;
		LeadPole.Z -= 16.f;
		RearPole.Z -= 16.f;
	}
	if (ShownDefensePitch > 8.f)
	{
		const float Drop = FMath::Min(ShownDefensePitch, 74.f) * 0.42f;
		LeadHand.Z -= Drop;
		RearHand.Z -= Drop;
	}
	else if (ShownDefensePitch < -8.f)
	{
		const float Lift = FMath::Min(-ShownDefensePitch, 58.f) * 0.22f;
		LeadHand.Z += Lift;
		RearHand.Z += Lift;
	}
	LeadHand.Y -= ShownDefenseRoll * 0.22f;
	RearHand.Y -= ShownDefenseRoll * 0.22f;

	const float GuardBob = 1.f - PunchPoseAlpha;
	const float BreathBob = FMath::Sin(PoseClock * 1.7f) * 3.2f * GuardBob;
	const float HandSway = FMath::Sin(PoseClock * 1.15f) * 3.6f * GuardBob;
	const float LeadSign = bSouthpaw ? 1.f : -1.f;
	LeadHand.Z += BreathBob + SmoothedMoveForward * -2.5f * GuardBob;
	RearHand.Z += BreathBob * 0.65f;
	LeadHand.Y += LeadSign * (HandSway + SmoothedMoveRight * 2.5f);
	RearHand.Y -= LeadSign * HandSway * 0.45f;
	LeadHand.X += FMath::Sin(PoseClock * 1.15f + 1.1f) * 2.4f * GuardBob;
	RearHand.X += FMath::Sin(PoseClock * 0.85f) * 1.6f * GuardBob;

	auto BlendArm = [&](bool bLead, const FVector& HandOffset, const FVector& PoleOffset)
	{
		FVector Hand = HandOffset;
		if (bPower)
		{
			Hand.X += 8.f;
		}
		if (bLead)
		{
			LeadHand = FMath::Lerp(LeadHand, LeadShoulder + Side(Hand), PunchPoseAlpha);
			LeadPole = FMath::Lerp(LeadPole, LeadShoulder + Side(PoleOffset), PunchPoseAlpha);
		}
		else
		{
			RearHand = FMath::Lerp(RearHand, RearShoulder + Side(Hand), PunchPoseAlpha);
			RearPole = FMath::Lerp(RearPole, RearShoulder + Side(PoleOffset), PunchPoseAlpha);
		}
	};

	switch (DisplayAttack)
	{
	case EFBAttackId::Jab:
		BlendArm(true, bDisplayBody ? FVector(52.f, 6.f, -36.f) : FVector(64.f, 4.f, 4.f), bDisplayBody ? FVector(14.f, -22.f, -40.f) : FVector(18.f, -30.f, -8.f));
		break;
	case EFBAttackId::Cross:
		BlendArm(false, bDisplayBody ? FVector(58.f, -4.f, -34.f) : FVector(72.f, -6.f, 6.f), bDisplayBody ? FVector(16.f, 24.f, -38.f) : FVector(20.f, 28.f, -6.f));
		break;
	case EFBAttackId::LeadHook:
		// Elbow wings out to the lead side so the bend shows from ringside. Hand sweeps across.
		BlendArm(true, bDisplayBody ? FVector(32.f, 30.f, -34.f) : FVector(46.f, 34.f, 2.f), bDisplayBody ? FVector(4.f, -48.f, -30.f) : FVector(8.f, -58.f, 0.f));
		break;
	case EFBAttackId::RearHook:
		BlendArm(false, bDisplayBody ? FVector(36.f, -32.f, -34.f) : FVector(52.f, -36.f, 2.f), bDisplayBody ? FVector(6.f, 48.f, -30.f) : FVector(10.f, 58.f, 0.f));
		break;
	case EFBAttackId::LeadUppercut:
		BlendArm(true, bDisplayBody ? FVector(26.f, 4.f, -22.f) : FVector(22.f, 4.f, 18.f), bDisplayBody ? FVector(8.f, -12.f, -42.f) : FVector(10.f, -12.f, -36.f));
		break;
	case EFBAttackId::RearUppercut:
		BlendArm(false, bDisplayBody ? FVector(28.f, -4.f, -20.f) : FVector(24.f, -4.f, 20.f), bDisplayBody ? FVector(10.f, 12.f, -44.f) : FVector(12.f, 12.f, -38.f));
		break;
	default:
		break;
	}

	float CheekZ = 48.f;
	if (ComponentSpace.IsValidIndex(Head))
	{
		CheekZ = ToCapsule(ComponentSpace[Head].GetLocation()).Z - 12.f;
	}
	LeadHand.Z = FMath::Min(LeadHand.Z, CheekZ);
	RearHand.Z = FMath::Min(RearHand.Z, CheekZ);

	SolveLimb(Skel, LeadUpper, LeadLower, LeadHandBone, ToMesh(LeadHand), ToMesh(LeadPole), Local, ComponentSpace);
	SolveLimb(Skel, RearUpper, RearLower, RearHandBone, ToMesh(RearHand), ToMesh(RearPole), Local, ComponentSpace);

	PoseMesh->BoneSpaceTransforms = Local;
	PoseMesh->MarkRefreshTransformDirty();
	PoseMesh->RefreshBoneTransforms();
}

void AFBBoxerCharacter::HandleImpactPresentation(const FFBImpactEvent& Impact)
{
	if (Impact.Result != EFBContactResult::CleanHit && Impact.Result != EFBContactResult::Blocked)
	{
		return;
	}

	// This runs on the puncher. The defender is the one who flinches.
	if (AFBBoxerCharacter* Defender = FindOpponent())
	{
		Defender->PlayHurtReaction(Impact);
	}

	if (GEngine)
	{
		const TCHAR* Place = Impact.Region == EFBHitRegion::Body ? TEXT("Body") : TEXT("Head");
		const TCHAR* Kind = Impact.Result == EFBContactResult::Blocked ? TEXT("blocked") : TEXT("clean");
		GEngine->AddOnScreenDebugMessage(
			2,
			1.6f,
			FColor::Orange,
			FString::Printf(TEXT("%s  %s  %.0f"), Place, Kind, Impact.DamageApplied));
	}
}

void AFBBoxerCharacter::EnsureRuntimeInput()
{
	if (DefaultMappingContext && MoveAction && LookAction && TurnAction && JabAction && CrossAction && LeadHookAction && RearHookAction && LeadUppercutAction && RearUppercutAction && BlockAction && WeaveAction && BodyModAction && PowerModAction && LooseAction && StanceAction && TauntAction && HeadbuttAction && LeftThumbAction && RightThumbAction)
	{
		return;
	}

	const FFBRuntimeInputBundle Bundle = UFBRuntimeInputLibrary::EnsureBoxerInput(this);
	DefaultMappingContext = Bundle.MappingContext;
	MoveAction = Bundle.Move;
	LookAction = Bundle.Look;
	PunchStickAction = Bundle.PunchStick;
	TurnAction = Bundle.Turn;
	JabAction = Bundle.Jab;
	CrossAction = Bundle.Cross;
	LeadHookAction = Bundle.LeadHook;
	RearHookAction = Bundle.RearHook;
	LeadUppercutAction = Bundle.LeadUppercut;
	RearUppercutAction = Bundle.RearUppercut;
	BlockAction = Bundle.Block;
	WeaveAction = Bundle.Weave;
	BodyModAction = Bundle.BodyMod;
	PowerModAction = Bundle.PowerMod;
	LooseAction = Bundle.Loose;
	StanceAction = Bundle.Stance;
	TauntAction = Bundle.Taunt;
	HeadbuttAction = Bundle.Headbutt;
	LeftThumbAction = Bundle.LeftThumb;
	RightThumbAction = Bundle.RightThumb;
}

void AFBBoxerCharacter::SetFightMappingActive(bool bActive)
{
	if (bActive)
	{
		ApplyMappingContext();
	}
	else
	{
		ClearMappingContext();
	}
}

bool AFBBoxerCharacter::ControllerWantsFightMapping() const
{
	if (const AFBSparringPlayerController* Sparring = Cast<AFBSparringPlayerController>(GetController()))
	{
		return Sparring->WantsFightMapping();
	}
	return true;
}

void AFBBoxerCharacter::ClearMappingContext()
{
	ULocalPlayer* LocalPlayer = FightMappingPlayer.Get();
	if (!LocalPlayer)
	{
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			LocalPlayer = PC->GetLocalPlayer();
		}
	}
	if (!LocalPlayer || !DefaultMappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		Subsystem->RemoveMappingContext(DefaultMappingContext);
	}
}

void AFBBoxerCharacter::ApplyMappingContext()
{
	EnsureRuntimeInput();
	if (!ControllerWantsFightMapping())
	{
		ClearMappingContext();
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	FightMappingPlayer = LocalPlayer;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AFBBoxerCharacter::UnPossessed()
{
	ClearMappingContext();
	Super::UnPossessed();
}

void AFBBoxerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMappingContext();
	Super::EndPlay(EndPlayReason);
}

void AFBBoxerCharacter::RefreshFightCamera()
{
	if (!IsValid(PersonalOrbitCamera))
	{
		PersonalOrbitCamera = nullptr;
	}
	EnsurePersonalCamera();
}

void AFBBoxerCharacter::EnsurePersonalCamera()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!PersonalOrbitCamera)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		PersonalOrbitCamera = GetWorld()->SpawnActor<AFBBroadcastCamera>(
			GetActorLocation() + FVector(0.f, -260.f, 160.f),
			FRotator(-12.f, 90.f, 0.f),
			Params);
	}

	if (PersonalOrbitCamera)
	{
		PersonalOrbitCamera->SetFighters(this, FindOpponent());
		PersonalOrbitCamera->ForcePossessedView();
	}
}

void AFBBoxerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyMappingContext();
	EnsurePersonalCamera();
}

void AFBBoxerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	ApplyMappingContext();
	EnsurePersonalCamera();
}

void AFBBoxerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

void AFBBoxerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PunchStickCooldown > 0.f)
	{
		PunchStickCooldown -= DeltaSeconds;
	}

	if (HasAuthority() || IsLocallyControlled())
	{
		bool bBagTraining = false;
		for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
		{
			if (It->IsActiveBag())
			{
				bBagTraining = true;
				break;
			}
		}
		if (!bBagTraining)
		{
			for (TActorIterator<AFBRingBounds> It(GetWorld()); It; ++It)
			{
				const FVector Loc = GetActorLocation();
				// The bag room is north of the gym. A fighter already standing there is not in this ring.
				if (FVector::Dist2D(Loc, It->GetActorLocation()) > 1500.f)
				{
					break;
				}
				const FVector Clamped = It->ClampToRing(Loc);
				if (!Clamped.Equals(Loc, 0.5f))
				{
					SetActorLocation(Clamped, false, nullptr, ETeleportType::TeleportPhysics);
				}
				break;
			}
		}
	}

	const bool bDriveFacing = IsLocallyControlled() || HasAuthority();
	if (bDriveFacing)
	{
		UpdateFacingOffset(DeltaSeconds);
		FaceOpponent(DeltaSeconds);
	}
	if (IsLocallyControlled())
	{
		FadeReleasedMove(DeltaSeconds);
	}

	if (IsLocallyControlled() && PersonalOrbitCamera)
	{
		PersonalOrbitCamera->SetFighters(this, FindOpponent());
	}

	DecayHurtReaction(DeltaSeconds);
	if (HasAuthority())
	{
		DecayStun(DeltaSeconds);
	}
	ApplyConditionToMovement();
	ApplyDownedMesh();
	ApplyVisibleDamage();
	if (HasAuthority())
	{
		TickBlock(DeltaSeconds);
	}
	if (IsLocallyControlled() && bInClinch)
	{
		ServerSubmitClinchSteer(CachedLeftStick.X);
	}
	if (bLeftThumbDown && !bRightThumbDown && GetWorld() && GetWorld()->GetTimeSeconds() - LeftThumbTime > 0.14f && LeftThumbTime > 0.f)
	{
		LeftThumbTime = -1.f;
		if (!bInClinch && CachedLeftStick.Size() > 0.35f)
		{
			BeginDefenseSlip(CachedLeftStick);
			SubmitRingAction(EFBRingAction::Slip);
		}
	}

	UpdateQueryVolumes();
	RefreshCombatPresentation();
	UpdateBoxerPose(DeltaSeconds);
}

void AFBBoxerCharacter::UpdateFacingOffset(float DeltaSeconds)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	FacingYawOffset = FMath::FInterpTo(FacingYawOffset, FacingYawOffsetTarget, DeltaSeconds, 6.f);
	const float FaceLimit = bLooseMovement ? 72.f : MaxFacingOffsetDegrees;
	FacingYawOffset = FMath::Clamp(FacingYawOffset, -FaceLimit, FaceLimit);

	if (!HasAuthority() && FMath::Abs(FacingYawOffset - LastSentFacingOffset) > 1.5f)
	{
		ServerSetFacingOffset(FacingYawOffset);
		LastSentFacingOffset = FacingYawOffset;
	}
}

void AFBBoxerCharacter::FaceOpponent(float DeltaSeconds)
{
	if (IsHookFacingLocked())
	{
		const FRotator LockedRot(0.f, FacingLockYaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), LockedRot, DeltaSeconds, 14.f));
		return;
	}

	AActor* Target = FindCombatTarget();
	if (!Target)
	{
		return;
	}

	const FVector ToOpp = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToOpp.IsNearlyZero())
	{
		return;
	}

	const float FaceLimit = bLooseMovement ? 72.f : MaxFacingOffsetDegrees;
	const float OpponentYaw = ToOpp.Rotation().Yaw;
	const float DeltaYaw = FMath::Clamp(
		FMath::FindDeltaAngleDegrees(OpponentYaw, OpponentYaw + FacingYawOffset),
		-FaceLimit,
		FaceLimit);
	const FRotator TargetRot(0.f, OpponentYaw + DeltaYaw, 0.f);

	const bool bPunching = CombatComponent && CombatComponent->IsAttackCommitted();
	const float Speed = bPunching ? PunchLockedTurnSpeed : (bLooseMovement ? FaceTurnSpeed * 0.45f : FaceTurnSpeed);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, Speed));
}

AFBBroadcastCamera* AFBBoxerCharacter::FindOrbitCamera() const
{
	if (PersonalOrbitCamera)
	{
		return PersonalOrbitCamera;
	}
	for (TActorIterator<AFBBroadcastCamera> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AFBBoxerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureRuntimeInput();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFBBoxerCharacter::Move);
			EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnMoveReleased);
			EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnMoveReleased);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFBBoxerCharacter::Look);
		}
		if (PunchStickAction)
		{
			EIC->BindAction(PunchStickAction, ETriggerEvent::Triggered, this, &AFBBoxerCharacter::OnPunchStick);
			EIC->BindAction(PunchStickAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnPunchStickReleased);
			EIC->BindAction(PunchStickAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnPunchStickReleased);
		}
		if (TurnAction)
		{
			EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AFBBoxerCharacter::OnTurnStick);
			EIC->BindAction(TurnAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnTurnStickReleased);
			EIC->BindAction(TurnAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnTurnStickReleased);
		}
		if (JabAction)
		{
			EIC->BindAction(JabAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnJab);
			EIC->BindAction(JabAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnJabReleased);
			EIC->BindAction(JabAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnJabReleased);
		}
		if (CrossAction)
		{
			EIC->BindAction(CrossAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnCross);
			EIC->BindAction(CrossAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnCrossReleased);
			EIC->BindAction(CrossAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnCrossReleased);
		}
		if (LeadHookAction)
		{
			EIC->BindAction(LeadHookAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLeadHook);
			EIC->BindAction(LeadHookAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnLeadHookReleased);
			EIC->BindAction(LeadHookAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnLeadHookReleased);
		}
		if (RearHookAction)
		{
			EIC->BindAction(RearHookAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnRearHook);
			EIC->BindAction(RearHookAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnRearHookReleased);
			EIC->BindAction(RearHookAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnRearHookReleased);
		}
		if (LeadUppercutAction)
		{
			EIC->BindAction(LeadUppercutAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLeadUppercut);
			EIC->BindAction(LeadUppercutAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnLeadUppercutReleased);
			EIC->BindAction(LeadUppercutAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnLeadUppercutReleased);
		}
		if (RearUppercutAction)
		{
			EIC->BindAction(RearUppercutAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnRearUppercut);
			EIC->BindAction(RearUppercutAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnRearUppercutReleased);
			EIC->BindAction(RearUppercutAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnRearUppercutReleased);
		}
		if (LooseAction)
		{
			EIC->BindAction(LooseAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLoose);
		}
		if (StanceAction)
		{
			EIC->BindAction(StanceAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnStance);
		}
		if (TauntAction)
		{
			EIC->BindAction(TauntAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnTaunt);
		}
		if (HeadbuttAction)
		{
			EIC->BindAction(HeadbuttAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnHeadbutt);
		}
		if (LeftThumbAction)
		{
			EIC->BindAction(LeftThumbAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLeftThumbPressed);
			EIC->BindAction(LeftThumbAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnLeftThumbReleased);
			EIC->BindAction(LeftThumbAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnLeftThumbReleased);
		}
		if (RightThumbAction)
		{
			EIC->BindAction(RightThumbAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnRightThumbPressed);
			EIC->BindAction(RightThumbAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnRightThumbReleased);
			EIC->BindAction(RightThumbAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnRightThumbReleased);
		}
		if (BlockAction)
		{
			EIC->BindAction(BlockAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnBlockStarted);
			EIC->BindAction(BlockAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnBlockCompleted);
			EIC->BindAction(BlockAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnBlockCompleted);
		}
		if (WeaveAction)
		{
			EIC->BindAction(WeaveAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnWeaveStarted);
			EIC->BindAction(WeaveAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnWeaveCompleted);
			EIC->BindAction(WeaveAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnWeaveCompleted);
		}
		if (BodyModAction)
		{
			EIC->BindAction(BodyModAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnBodyModStarted);
			EIC->BindAction(BodyModAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnBodyModCompleted);
			EIC->BindAction(BodyModAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnBodyModCompleted);
		}
		if (PowerModAction)
		{
			EIC->BindAction(PowerModAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnPowerModStarted);
			EIC->BindAction(PowerModAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnPowerModCompleted);
			EIC->BindAction(PowerModAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnPowerModCompleted);
		}
	}
}

void AFBBoxerCharacter::PushSmoothedMove()
{
	if (SmoothedMoveStick.SizeSquared() < FMath::Square(0.05f))
	{
		return;
	}
	const bool bPunchButtons = bSquareHeld || bTriangleHeld || bXHeld || bCircleHeld;
	const float Dodge = (bPowerModifierHeld && !bPunchButtons) ? 1.85f : (SmoothedMoveStick.Size() > 0.85f ? 1.28f : 0.84f);
	const FRotator Facing(0.f, GetActorRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), SmoothedMoveStick.Y * MoveSpeedScale * Dodge);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), SmoothedMoveStick.X * MoveSpeedScale * Dodge);
}

void AFBBoxerCharacter::FadeReleasedMove(float DeltaSeconds)
{
	if (bMoveHeld)
	{
		return;
	}
	if (bKnockedDown || bInClinch)
	{
		SmoothedMoveStick = FVector2D::ZeroVector;
		return;
	}
	SmoothedMoveStick = FMath::Vector2DInterpTo(SmoothedMoveStick, FVector2D::ZeroVector, DeltaSeconds, 8.f);
	PushSmoothedMove();
}

void AFBBoxerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();
	CachedLeftStick = Axis;
	bMoveHeld = true;
	if (bKnockedDown || bInClinch)
	{
		SmoothedMoveStick = FVector2D::ZeroVector;
		RefreshBlockHeight();
		return;
	}
	const float Dt = GetWorld() ? FMath::Max(GetWorld()->GetDeltaSeconds(), 0.001f) : 0.016f;
	SmoothedMoveStick = FMath::Vector2DInterpTo(SmoothedMoveStick, Axis, Dt, 8.f);
	PushSmoothedMove();
	RefreshBlockHeight();
}

void AFBBoxerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.SizeSquared() < FMath::Square(0.40f))
	{
		return;
	}
	if (AFBBroadcastCamera* Cam = FindOrbitCamera())
	{
		Cam->AddOrbitInput(Axis);
	}
}

void AFBBoxerCharacter::OnMoveReleased(const FInputActionValue& Value)
{
	(void)Value;
	CachedLeftStick = FVector2D::ZeroVector;
	bMoveHeld = false;
	RefreshBlockHeight();
}

void AFBBoxerCharacter::OnTurnStick(const FInputActionValue& Value)
{
	// Explicit rule: one right-stick sample either yaws the shoulders or throws a punch.
	// It does not do both. The mapping dead zone stays at 0.12. This 0.55 flick is the only punch gate.
	constexpr float PunchFlickThreshold = 0.55f;
	const FVector2D Axis = Value.Get<FVector2D>();
	CachedRightStick = Axis;
	if (bKnockedDown)
	{
		return;
	}
	if (Axis.Size() >= PunchFlickThreshold)
	{
		TryStickPunch();
		return;
	}
	bPunchStickConsumed = false;
	if (!bInClinch)
	{
		const float FaceLimit = bLooseMovement ? 72.f : MaxFacingOffsetDegrees;
		FacingYawOffsetTarget = FMath::Clamp(Axis.X, -1.f, 1.f) * FaceLimit;
	}
}

void AFBBoxerCharacter::OnTurnStickReleased()
{
	CachedRightStick = FVector2D::ZeroVector;
	FacingYawOffsetTarget = 0.f;
	bPunchStickConsumed = false;
}

void AFBBoxerCharacter::OnPunchStickReleased()
{
	bPunchStickConsumed = false;
}

void AFBBoxerCharacter::OnPunchStick(const FInputActionValue& Value)
{
	// The right stick is mapped only to Turn. This action has no key, so it cannot
	// throw a second punch from the same deflection.
	(void)Value;
}

void AFBBoxerCharacter::OnJab()
{
	bSquareHeld = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnJabReleased()
{
	bSquareHeld = false;
	if (!bSquareHeld && !bTriangleHeld && !bXHeld && !bCircleHeld && !bLeadUppercutKey && !bRearUppercutKey)
	{
		bPrecisionLatched = false;
	}
}

void AFBBoxerCharacter::OnCross()
{
	bTriangleHeld = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnCrossReleased()
{
	bTriangleHeld = false;
	bPrecisionLatched = false;
}

void AFBBoxerCharacter::OnLeadHook()
{
	bXHeld = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnLeadHookReleased()
{
	bXHeld = false;
	bPrecisionLatched = false;
}

void AFBBoxerCharacter::OnRearHook()
{
	bCircleHeld = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnRearHookReleased()
{
	bCircleHeld = false;
	bPrecisionLatched = false;
}

void AFBBoxerCharacter::OnLeadUppercut()
{
	bLeadUppercutKey = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnLeadUppercutReleased()
{
	bLeadUppercutKey = false;
	bPrecisionLatched = false;
}

void AFBBoxerCharacter::OnRearUppercut()
{
	bRearUppercutKey = true;
	TryPrecisionPunch(true);
}

void AFBBoxerCharacter::OnRearUppercutReleased()
{
	bRearUppercutKey = false;
	bPrecisionLatched = false;
}

void AFBBoxerCharacter::OnBlockStarted()
{
	bBlockHeld = true;
	bBlockGuardSent = false;
	RefreshBlockHeight();
}

void AFBBoxerCharacter::OnBlockCompleted()
{
	bBlockHeld = false;
	bBlockGuardSent = false;
	if (bWeaveHeld)
	{
		SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::Weave);
	}
	else
	{
		SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
	}
}

void AFBBoxerCharacter::OnWeaveStarted()
{
	bWeaveHeld = true;
	SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::Weave);
}

void AFBBoxerCharacter::OnWeaveCompleted()
{
	bWeaveHeld = false;
	if (bBlockHeld)
	{
		bBlockGuardSent = false;
		RefreshBlockHeight();
	}
	else
	{
		SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
	}
}

void AFBBoxerCharacter::OnBodyModStarted()
{
	bBodyModifierHeld = true;
	bBodyModifierConsumed = false;
	BodyModPressTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void AFBBoxerCharacter::OnBodyModCompleted()
{
	bBodyModifierHeld = false;
	ReleaseShoulderModifier(true, BodyModPressTime, bBodyModifierConsumed);
	BodyModPressTime = -1.f;
}

void AFBBoxerCharacter::OnPowerModStarted()
{
	bPowerModifierHeld = true;
	bPowerModifierConsumed = false;
	PowerModPressTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void AFBBoxerCharacter::OnPowerModCompleted()
{
	bPowerModifierHeld = false;
	ReleaseShoulderModifier(false, PowerModPressTime, bPowerModifierConsumed);
	PowerModPressTime = -1.f;
}

void AFBBoxerCharacter::OnLoose()
{
	SubmitRingAction(EFBRingAction::LooseToggle);
}

void AFBBoxerCharacter::OnStance()
{
	SubmitRingAction(EFBRingAction::StanceSwitch);
}

void AFBBoxerCharacter::OnTaunt()
{
	SubmitRingAction(EFBRingAction::Taunt);
}

void AFBBoxerCharacter::OnHeadbutt()
{
	if (bBodyModifierHeld)
	{
		bBodyModifierConsumed = true;
		SubmitRingAction(EFBRingAction::LowBlow);
		return;
	}
	if (bPowerModifierHeld)
	{
		bPowerModifierConsumed = true;
		SubmitRingAction(EFBRingAction::TakeAKnee);
		return;
	}
	SubmitRingAction(EFBRingAction::Headbutt);
}

void AFBBoxerCharacter::OnLeftThumbPressed()
{
	bLeftThumbDown = true;
	LeftThumbTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (bRightThumbDown)
	{
		LeftThumbTime = -1.f;
		SubmitRingAction(EFBRingAction::StartClinch);
		return;
	}
	if (!bInClinch && CachedLeftStick.Size() > 0.35f)
	{
		LeftThumbTime = -1.f;
		BeginDefenseSlip(CachedLeftStick);
		SubmitRingAction(EFBRingAction::Slip);
	}
}

void AFBBoxerCharacter::OnLeftThumbReleased()
{
	bLeftThumbDown = false;
}

void AFBBoxerCharacter::OnRightThumbPressed()
{
	bRightThumbDown = true;
	RightThumbTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (bLeftThumbDown)
	{
		LeftThumbTime = -1.f;
		SubmitRingAction(EFBRingAction::StartClinch);
		return;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now < PushReadyTime || bKnockedDown)
	{
		return;
	}
	PushReadyTime = Now + 0.7f;
	SubmitRingAction(EFBRingAction::Push);
}

void AFBBoxerCharacter::OnRightThumbReleased()
{
	bRightThumbDown = false;
}

bool AFBBoxerCharacter::StickInCone(float CenterDegrees, float HalfDegrees) const
{
	if (CachedRightStick.Size() < 0.55f)
	{
		return false;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(CachedRightStick.Y, CachedRightStick.X));
	return FMath::Abs(FMath::FindDeltaAngleDegrees(Angle, CenterDegrees)) <= HalfDegrees;
}

EFBAttackId AFBBoxerCharacter::MatchPrecisionAttack() const
{
	const bool bLeadUpper = bLeadUppercutKey || (bXHeld && bSquareHeld);
	const bool bRearUpper = bRearUppercutKey || (bCircleHeld && bTriangleHeld);
	if (bLeadUpper)
	{
		return EFBAttackId::LeadUppercut;
	}
	if (bRearUpper)
	{
		return EFBAttackId::RearUppercut;
	}
	if (bSquareHeld && !bXHeld)
	{
		return EFBAttackId::Jab;
	}
	if (bTriangleHeld && !bCircleHeld)
	{
		return EFBAttackId::Cross;
	}
	if (bXHeld && !bSquareHeld)
	{
		return EFBAttackId::LeadHook;
	}
	if (bCircleHeld && !bTriangleHeld)
	{
		return EFBAttackId::RearHook;
	}
	return EFBAttackId::None;
}

bool AFBBoxerCharacter::ConeMatches(EFBAttackId Attack) const
{
	const float Cone = GetAccuracyCone();
	switch (Attack)
	{
	case EFBAttackId::Jab: return StickInCone(135.f, Cone);
	case EFBAttackId::Cross: return StickInCone(45.f, Cone);
	case EFBAttackId::LeadHook: return StickInCone(180.f, Cone);
	case EFBAttackId::RearHook: return StickInCone(0.f, Cone);
	case EFBAttackId::LeadUppercut: return StickInCone(-135.f, Cone);
	case EFBAttackId::RearUppercut: return StickInCone(-45.f, Cone);
	default: return false;
	}
}

EFBAttackId AFBBoxerCharacter::MirrorForStance(EFBAttackId Attack) const
{
	if (Stance != EFBStance::Southpaw)
	{
		return Attack;
	}
	switch (Attack)
	{
	case EFBAttackId::Jab: return EFBAttackId::Cross;
	case EFBAttackId::Cross: return EFBAttackId::Jab;
	case EFBAttackId::LeadHook: return EFBAttackId::RearHook;
	case EFBAttackId::RearHook: return EFBAttackId::LeadHook;
	case EFBAttackId::LeadUppercut: return EFBAttackId::RearUppercut;
	case EFBAttackId::RearUppercut: return EFBAttackId::LeadUppercut;
	default: return Attack;
	}
}

EFBAttackId AFBBoxerCharacter::AttackFromStick() const
{
	if (CachedRightStick.Size() < 0.55f)
	{
		return EFBAttackId::None;
	}
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(CachedRightStick.Y, CachedRightStick.X));
	struct FStickPunch
	{
		EFBAttackId Attack;
		float Center;
	};
	const FStickPunch Punches[] = {
		{ EFBAttackId::Jab, 135.f },
		{ EFBAttackId::Cross, 45.f },
		{ EFBAttackId::LeadHook, 180.f },
		{ EFBAttackId::RearHook, 0.f },
		{ EFBAttackId::LeadUppercut, -135.f },
		{ EFBAttackId::RearUppercut, -45.f },
	};
	EFBAttackId Best = EFBAttackId::None;
	float BestDelta = 32.f;
	for (const FStickPunch& Punch : Punches)
	{
		const float Delta = FMath::Abs(FMath::FindDeltaAngleDegrees(Angle, Punch.Center));
		if (Delta < BestDelta)
		{
			BestDelta = Delta;
			Best = Punch.Attack;
		}
	}
	return Best;
}

EFBGuardState AFBBoxerCharacter::BlockGuardFromStick() const
{
	return CachedLeftStick.Y < -0.45f ? EFBGuardState::Low : EFBGuardState::High;
}

void AFBBoxerCharacter::RefreshBlockHeight()
{
	if (!bBlockHeld || bWeaveHeld || !IsLocallyControlled() || bKnockedDown)
	{
		return;
	}
	const EFBGuardState Want = BlockGuardFromStick();
	if (bBlockGuardSent && SentBlockGuard == Want)
	{
		return;
	}
	bBlockGuardSent = true;
	SentBlockGuard = Want;
	SubmitLocalIntent(EFBAttackId::None, true, Want);
}

void AFBBoxerCharacter::ReleaseShoulderModifier(bool bRollLeft, float PressStartedAt, bool bConsumed)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Held = PressStartedAt >= 0.f ? Now - PressStartedAt : 1.f;
	const bool bDodge = !bRollLeft && CachedLeftStick.Size() > 0.45f;
	if (Held > ModifierTapSeconds || bConsumed || bDodge || bKnockedDown || bInClinch)
	{
		return;
	}
	BeginShoulderRoll(bRollLeft ? -1.f : 1.f);
}

void AFBBoxerCharacter::BeginShoulderRoll(float StickX)
{
	BeginDefenseSlip(FVector2D(StickX, 0.f));
	if (!HasAuthority())
	{
		ServerApplyShoulderRoll(StickX);
	}
	if (IsLocallyControlled() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 0.7f, FColor::Cyan, StickX < 0.f ? TEXT("Roll left") : TEXT("Roll right"));
	}
}

void AFBBoxerCharacter::ServerApplyShoulderRoll_Implementation(float StickX)
{
	if (!HasAuthority() || bKnockedDown || bInClinch || FMath::Abs(StickX) < 0.5f)
	{
		return;
	}
	BeginDefenseSlip(FVector2D(FMath::Clamp(StickX, -1.f, 1.f), 0.f));
}

void AFBBoxerCharacter::AuthorityTakeAKnee()
{
	if (!HasAuthority() || bKnockedDown || bInClinch)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AFBHeavyBag> It(World); It; ++It)
	{
		return;
	}
	AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>();
	if (!Mode || Mode->GetPhase() != EFBBoutPhase::RoundActive)
	{
		return;
	}
	Mode->BeginKnockdown(this);
}

void AFBBoxerCharacter::TryStickPunch()
{
	if (bKnockedDown || bPunchStickConsumed || PunchStickCooldown > 0.f || bInClinch)
	{
		return;
	}
	if (bSquareHeld || bTriangleHeld || bXHeld || bCircleHeld || bLeadUppercutKey || bRearUppercutKey)
	{
		return;
	}
	const EFBAttackId Attack = MirrorForStance(AttackFromStick());
	if (Attack == EFBAttackId::None)
	{
		return;
	}
	bPunchStickConsumed = true;
	PunchStickCooldown = 0.28f;
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(Attack, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::TryPrecisionPunch(bool bFromButton)
{
	(void)bFromButton;
	if (bKnockedDown || bPrecisionLatched || PunchStickCooldown > 0.f)
	{
		return;
	}

	const EFBAttackId Attack = MirrorForStance(MatchPrecisionAttack());
	if (Attack == EFBAttackId::None)
	{
		return;
	}

	bPrecisionLatched = true;
	PunchStickCooldown = 0.28f;
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(Attack, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::SubmitRingAction(EFBRingAction Action)
{
	if (bKnockedDown || Action == EFBRingAction::None)
	{
		return;
	}
	FFBCombatIntent Intent;
	Intent.Sequence = ++LocalIntentSequence;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = EFBAttackId::None;
	Intent.SchemaVersion = TEXT("1.0.0");
	Intent.RingAction = Action;
	Intent.MoveAxis = CachedLeftStick;
	Intent.bBodyTarget = bBodyModifierHeld;
	Intent.bPowerPunch = bPowerModifierHeld;

	AActor* Target = FindCombatTarget();
	if (HasAuthority())
	{
		if (CombatComponent)
		{
			CombatComponent->ServerTryProcessIntent(Intent, Target);
		}
	}
	else
	{
		ServerSubmitCombatIntent(Intent);
	}
}

void AFBBoxerCharacter::SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard)
{
	if (bKnockedDown)
	{
		return;
	}
	FFBCombatIntent Intent;
	Intent.Sequence = ++LocalIntentSequence;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = Attack;
	Intent.bGuardHeld = bGuardHeld;
	Intent.Guard = Guard;
	Intent.SchemaVersion = TEXT("1.0.0");
	Intent.bBodyTarget = bBodyModifierHeld;
	Intent.bPowerPunch = bPowerModifierHeld;
	Intent.bPrecisionCone = Attack != EFBAttackId::None && ConeMatches(MirrorForStance(Attack));
	if (Attack != EFBAttackId::None)
	{
		if (bBodyModifierHeld)
		{
			bBodyModifierConsumed = true;
		}
		if (bPowerModifierHeld)
		{
			bPowerModifierConsumed = true;
		}
	}

	AActor* Target = FindCombatTarget();
	if (HasAuthority())
	{
		const EFBIntentResult Result = CombatComponent
			? CombatComponent->ServerTryProcessIntent(Intent, Target)
			: EFBIntentResult::RejectedNoAuthority;
		ReportIntentResult(Result, Attack);
	}
	else
	{
		ServerSubmitCombatIntent(Intent);
	}
}

void AFBBoxerCharacter::ReportIntentResult(EFBIntentResult Result, EFBAttackId Attack) const
{
#if UE_BUILD_SHIPPING
	(void)Result;
	(void)Attack;
#else
	if (!IsLocallyControlled() || !GEngine || Attack == EFBAttackId::None)
	{
		return;
	}
	const TCHAR* Name = TEXT("Punch");
	switch (Attack)
	{
	case EFBAttackId::Jab: Name = TEXT("Jab"); break;
	case EFBAttackId::Cross: Name = TEXT("Straight"); break;
	case EFBAttackId::LeadHook: Name = TEXT("Lead hook"); break;
	case EFBAttackId::RearHook: Name = TEXT("Rear hook"); break;
	case EFBAttackId::LeadUppercut: Name = TEXT("Lead uppercut"); break;
	case EFBAttackId::RearUppercut: Name = TEXT("Rear uppercut"); break;
	default: break;
	}
	const TCHAR* Why = TEXT("rejected");
	FColor Color = FColor::Red;
	switch (Result)
	{
	case EFBIntentResult::Accepted: Why = TEXT("accepted"); Color = FColor::White; break;
	case EFBIntentResult::Buffered: Why = TEXT("buffered"); Color = FColor::Yellow; break;
	case EFBIntentResult::RejectedDown: Why = TEXT("rejected — down"); break;
	case EFBIntentResult::RejectedSequence: Why = TEXT("rejected — stale input"); break;
	case EFBIntentResult::RejectedSchema: Why = TEXT("rejected — schema"); break;
	case EFBIntentResult::RejectedPhase: Why = TEXT("rejected — punch still out"); break;
	case EFBIntentResult::RejectedEnergy: Why = TEXT("rejected — no energy"); break;
	case EFBIntentResult::RejectedBreak: Why = TEXT("rejected — bell or break"); break;
	case EFBIntentResult::RejectedNoAuthority: Why = TEXT("rejected — no authority"); break;
	default: break;
	}
	const FString Line = (Result == EFBIntentResult::Accepted || Result == EFBIntentResult::Buffered)
		? FString::Printf(TEXT("%s — %s"), Name, Why)
		: FString::Printf(TEXT("%s — %s"), Why, Name);
	GEngine->AddOnScreenDebugMessage(7, 1.1f, Color, Line);
#endif
}

void AFBBoxerCharacter::ClientReportIntent_Implementation(EFBIntentResult Result, EFBAttackId Attack)
{
	ReportIntentResult(Result, Attack);
}

void AFBBoxerCharacter::AuthorityPrepareForRematch()
{
	if (!HasAuthority())
	{
		return;
	}
	HeadDamage = 0.f;
	BodyDamage = 0.f;
	Stun = 0.f;
	AccumulatedDamage = 0.f;
	HeadShock = 0.f;
	BodyShock = 0.f;
	PendingDazeStun = 0.f;
	CutSeverity = 0.f;
	BruiseSeverity = 0.f;
	BlockHealth = BlockHealthMax;
	AppliedAttackInstanceIds.Reset();
	ClinchTurnMeter = 0.f;
	SetKnockedDown(false);
	AuthoritySetClinch(false);
	if (CombatComponent)
	{
		CombatComponent->ResetForRematch();
	}
}

bool AFBBoxerCharacter::ServerSubmitCombatIntent_Validate(const FFBCombatIntent& Intent)
{
	return Intent.Sequence > 0 && Intent.SchemaVersion.Len() > 0;
}

void AFBBoxerCharacter::ServerSubmitCombatIntent_Implementation(const FFBCombatIntent& Intent)
{
	EFBIntentResult Result = EFBIntentResult::RejectedNoAuthority;
	if (Intent.RingAction == EFBRingAction::TakeAKnee)
	{
		AuthorityTakeAKnee();
		Result = IsKnockedDown() ? EFBIntentResult::Accepted : EFBIntentResult::RejectedBreak;
	}
	else if (CombatComponent)
	{
		Result = CombatComponent->ServerTryProcessIntent(Intent, FindCombatTarget());
	}
#if !UE_BUILD_SHIPPING
	ClientReportIntent(Result, Intent.Attack);
#else
	(void)Result;
#endif
}

AFBBoxerCharacter* AFBBoxerCharacter::FindOpponent() const
{
	AFBBoxerCharacter* Fallback = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Other = *It;
		if (!Other || Other == this)
		{
			continue;
		}
		// The sparring partner is the tagged opponent. Any other boxer is the
		// spare that used to be left in the ring.
		if (Other->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			return Other;
		}
		if (!Fallback)
		{
			Fallback = Other;
		}
	}
	return Fallback;
}

AActor* AFBBoxerCharacter::FindCombatTarget() const
{
	if (AFBBoxerCharacter* Opp = FindOpponent())
	{
		return Opp;
	}
	AFBHeavyBag* Fallback = nullptr;
	for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		if (It->IsActiveBag())
		{
			return *It;
		}
		if (!Fallback)
		{
			Fallback = *It;
		}
	}
	return Fallback;
}

float AFBBoxerCharacter::ComputeDisplayedPunchYaw() const
{
	float PunchYaw = 0.f;
	switch (DisplayAttack)
	{
	case EFBAttackId::Jab: PunchYaw = 6.f; break;
	case EFBAttackId::Cross: PunchYaw = -18.f; break;
	case EFBAttackId::LeadHook: PunchYaw = 24.f; break;
	case EFBAttackId::RearHook: PunchYaw = -26.f; break;
	case EFBAttackId::LeadUppercut: PunchYaw = 8.f; break;
	case EFBAttackId::RearUppercut: PunchYaw = -10.f; break;
	default: break;
	}
	const float LeadYaw = (Stance == EFBStance::Southpaw) ? -1.f : 1.f;
	return PunchYaw * PunchPoseAlpha * LeadYaw;
}

float AFBBoxerCharacter::GetMoveSpeedScale() const
{
	const float StunAlpha = (MaxStun > 1.f) ? FMath::Clamp(Stun / MaxStun, 0.f, 1.f) : 0.f;
	const float BodyAlpha = (MaxBodyDamage > 1.f) ? FMath::Clamp(BodyDamage / MaxBodyDamage, 0.f, 1.f) : 0.f;
	const float Scale = (1.f - StunAlpha * StunMovePenalty - BodyAlpha * BodyMovePenalty) * CombatDerived.Footwork;
	return FMath::Clamp(Scale, MinMoveScale, 1.15f);
}

float AFBBoxerCharacter::GetOutgoingDamageScale() const
{
	const float BodyAlpha = (MaxBodyDamage > 1.f) ? FMath::Clamp(BodyDamage / MaxBodyDamage, 0.f, 1.f) : 0.f;
	const float StunAlpha = (MaxStun > 1.f) ? FMath::Clamp(Stun / MaxStun, 0.f, 1.f) : 0.f;
	const float Scale = 1.f - BodyAlpha * BodyOutputPenalty - StunAlpha * StunOutputPenalty;
	return FMath::Clamp(Scale, MinOutputScale, 1.f);
}

float AFBBoxerCharacter::GetBurstRecoveryScale() const
{
	const float BodyAlpha = (MaxBodyDamage > 1.f) ? FMath::Clamp(BodyDamage / MaxBodyDamage, 0.f, 1.f) : 0.f;
	return FMath::Lerp(1.f, BodyRecoveryAtFull, BodyAlpha);
}

float AFBBoxerCharacter::GetActionTimeScale() const
{
	const float BodyAlpha = (MaxBodyDamage > 1.f) ? FMath::Clamp(BodyDamage / MaxBodyDamage, 0.f, 1.f) : 0.f;
	return FMath::Lerp(1.f, 1.85f, BodyAlpha);
}

void AFBBoxerCharacter::DecayStun(float DeltaSeconds)
{
	if (!HasAuthority() || Stun <= 0.f)
	{
		return;
	}
	Stun = FMath::Max(0.f, Stun - StunRecoverPerSecond * DeltaSeconds);
}

void AFBBoxerCharacter::DecayHurtReaction(float DeltaSeconds)
{
	const float Speed = 6.f;
	HurtYaw = FMath::FInterpTo(HurtYaw, 0.f, DeltaSeconds, Speed);
	HurtLean = FMath::FInterpTo(HurtLean, 0.f, DeltaSeconds, Speed);
	HurtHeadPitch = FMath::FInterpTo(HurtHeadPitch, 0.f, DeltaSeconds, Speed);
}

void AFBBoxerCharacter::ApplyConditionToMovement()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = bKnockedDown ? 0.f : BaseWalkSpeed * GetMoveSpeedScale();
	}
}

void AFBBoxerCharacter::PlayHurtReaction(const FFBImpactEvent& Impact)
{
	if (Impact.Result != EFBContactResult::CleanHit && Impact.Result != EFBContactResult::Blocked)
	{
		return;
	}

	// A blocked shot still moves the body, just less. The sign follows the
	// punch so a rear hand and a lead hand do not flinch the same way.
	const float Gain = (Impact.Result == EFBContactResult::Blocked) ? 0.35f : 1.f;
	float Side = 1.f;
	switch (Impact.Attack)
	{
	case EFBAttackId::Cross:
	case EFBAttackId::RearHook:
	case EFBAttackId::RearUppercut:
		Side = -1.f;
		break;
	default:
		break;
	}
	if (Stance == EFBStance::Southpaw)
	{
		Side = -Side;
	}

	if (Impact.Region == EFBHitRegion::Body)
	{
		HurtYaw = 10.f * Side * Gain;
		HurtLean = 18.f * Gain;
		HurtHeadPitch = 4.f * Gain;
	}
	else
	{
		HurtYaw = 16.f * Side * Gain;
		HurtLean = 6.f * Gain;
		HurtHeadPitch = 14.f * Gain;
	}
}

float AFBBoxerCharacter::ApplyAuthoritativeDamage(float Amount, int32 AttackInstanceId, EFBHitRegion Region, bool bCountAsClean)
{
	if (!HasAuthority())
	{
		return 0.f;
	}
	if (AppliedAttackInstanceIds.Contains(AttackInstanceId))
	{
		return 0.f;
	}
	AppliedAttackInstanceIds.Add(AttackInstanceId);

	const float Added = TakeRegionalDamage(FMath::Max(0.f, Amount), Region);
	if (Region == EFBHitRegion::Body)
	{
		BodyDamage = FMath::Min(MaxBodyDamage, BodyDamage + Added);
	}
	else
	{
		HeadDamage = FMath::Min(MaxHeadDamage, HeadDamage + Added);
	}
	AccumulatedDamage = HeadDamage + BodyDamage;

	const float StunGain = Added * ((Region == EFBHitRegion::Body) ? BodyStunMultiplier : HeadStunMultiplier);
	Stun = FMath::Min(MaxStun, Stun + StunGain);

	if (bCountAsClean && Added > 0.f)
	{
		NoteInjuryChance(Region, true);
		if (Region == EFBHitRegion::Body)
		{
			NoteBodyShock(Added);
		}
		else
		{
			NoteHeadShock(Added);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("FinalBell damage region=%s amount=%.1f head=%.1f body=%.1f stun=%.1f"),
		Region == EFBHitRegion::Body ? TEXT("body") : TEXT("head"),
		Added,
		HeadDamage,
		BodyDamage,
		Stun);
	return Added;
}

bool AFBBoxerCharacter::ApplyFighterRecord(const FFBFighterRecord& Record, FString& OutWhy)
{
	if (!HasAuthority())
	{
		OutWhy = TEXT("Only the server applies a build.");
		return false;
	}
	if (Record.RulesetId != TEXT("CompetitiveFair") || Record.RulesetVersion != FFBFighterBalance::RulesetVersion)
	{
		OutWhy = TEXT("That build is not on CompetitiveFair.");
		return false;
	}
	if (!FFBFighterBalance::IsReady(Record.Ratings))
	{
		OutWhy = TEXT("A competitive bout needs a build that costs exactly 600.");
		return false;
	}
	if (Record.WeightClass != FFBFighterBalance::ClassForDivision(Record.Division))
	{
		OutWhy = TEXT("Division and weight class do not match.");
		return false;
	}
	FighterRecord = Record;
	bHasCompetitiveBuild = true;
	OnRep_FighterRecord();
	return true;
}

void AFBBoxerCharacter::OnRep_FighterRecord()
{
	CombatDerived = FFBFighterBalance::Derive(FighterRecord.Ratings);
	bHasCompetitiveBuild = FFBFighterBalance::IsReady(FighterRecord.Ratings);
	if (!bHasCompetitiveBuild)
	{
		CombatDerived = FFBCombatDerived();
	}
	Stance = FighterRecord.Stance;
	BaseSkin = FFBFighterBalance::ComplexionColor(FighterRecord.Look.Complexion);
	BlockHealthMax = 100.f * CombatDerived.GuardCapacity;
	BlockHealth = BlockHealthMax;
	if (UFBCombatComponent* Combat = GetCombatComponent())
	{
		Combat->MaxBurstEnergy = 100.f * CombatDerived.StaminaPool;
		if (HasAuthority())
		{
			Combat->RestoreBurstEnergy(Combat->MaxBurstEnergy);
		}
	}
	ApplyPresentation();
}

float AFBBoxerCharacter::GetHandPower(EFBAttackId Attack) const
{
	const bool bLeadAnim = Attack == EFBAttackId::Jab || Attack == EFBAttackId::LeadHook || Attack == EFBAttackId::LeadUppercut;
	const bool bPhysicalLeft = (Stance == EFBStance::Orthodox) ? bLeadAnim : !bLeadAnim;
	if (Attack == EFBAttackId::None)
	{
		return 1.f;
	}
	return bPhysicalLeft ? CombatDerived.LeftHand : CombatDerived.RightHand;
}

void AFBBoxerCharacter::NoteBlockedImpact(float CleanDamage)
{
	if (!HasAuthority())
	{
		return;
	}
	const float Tax = FMath::Max(1.f, CleanDamage) * CombatDerived.GuardTaken;
	BlockHealth = FMath::Max(0.f, BlockHealth - Tax);
	BlockRegenDelay = FFBFighterBalance::BlockDelaySeconds;
}

void AFBBoxerCharacter::NoteInjuryChance(EFBHitRegion Region, bool bClean)
{
	if (!HasAuthority() || !bClean)
	{
		return;
	}
	if (Region == EFBHitRegion::Head && FMath::FRand() < 0.35f * CombatDerived.CutMul)
	{
		CutSeverity = FMath::Min(1.f, CutSeverity + 0.12f * CombatDerived.CutMul);
	}
	if (FMath::FRand() < 0.4f * CombatDerived.BruiseMul)
	{
		BruiseSeverity = FMath::Min(1.f, BruiseSeverity + 0.1f * CombatDerived.BruiseMul);
	}
}

float AFBBoxerCharacter::TakeRegionalDamage(float Amount, EFBHitRegion Region)
{
	float Scaled = Amount * (Region == EFBHitRegion::Body ? CombatDerived.BodyTaken : CombatDerived.ChinTaken);
	if (Region == EFBHitRegion::Head && CutSeverity > 0.f)
	{
		Scaled *= 1.f + FMath::Min(0.04f, CutSeverity * 0.04f);
	}
	if (BruiseSeverity > 0.f)
	{
		const float ExtraStun = Amount * 0.15f * FMath::Min(0.05f, BruiseSeverity * 0.05f);
		Stun = FMath::Min(MaxStun, Stun + ExtraStun);
	}
	return Scaled;
}

void AFBBoxerCharacter::NoteHeadShock(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f || IsKnockedDown())
	{
		return;
	}
	HeadShock += Amount;
	const float Line = 70.f * CombatDerived.ChinDaze;
	if (HeadShock < Line)
	{
		return;
	}
	HeadShock = 0.f;
	PendingDazeStun = 25.f * (2.f - CombatDerived.ChinDaze);
	if (UWorld* World = GetWorld())
	{
		if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
		{
			Mode->NotifyEnergyDepleted(this);
		}
	}
}

void AFBBoxerCharacter::NoteBodyShock(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f || IsKnockedDown())
	{
		return;
	}
	BodyShock += Amount;
	const float Line = 70.f * CombatDerived.BodyDaze;
	if (BodyShock < Line)
	{
		return;
	}
	BodyShock = 0.f;
	PendingDazeStun = 25.f * (2.f - CombatDerived.BodyDaze);
	if (UWorld* World = GetWorld())
	{
		if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
		{
			Mode->NotifyEnergyDepleted(this);
		}
	}
}

float AFBBoxerCharacter::ConsumeDazeStun()
{
	const float StunBonus = PendingDazeStun;
	PendingDazeStun = 0.f;
	if (StunBonus > 0.f)
	{
		Stun = FMath::Min(MaxStun, Stun + StunBonus);
	}
	return StunBonus;
}

void AFBBoxerCharacter::TickBlock(float DeltaSeconds)
{
	if (BlockRegenDelay > 0.f)
	{
		BlockRegenDelay = FMath::Max(0.f, BlockRegenDelay - DeltaSeconds);
		return;
	}
	if (BlockHealth < BlockHealthMax)
	{
		BlockHealth = FMath::Min(BlockHealthMax, BlockHealth + 8.f * CombatDerived.BlockRegen * DeltaSeconds);
	}
}

namespace
{
	FLinearColor GearPalette(int32 Index)
	{
		switch (Index % 6)
		{
		case 0: return FLinearColor(0.55f, 0.08f, 0.08f);
		case 1: return FLinearColor(0.08f, 0.18f, 0.55f);
		case 2: return FLinearColor(0.08f, 0.08f, 0.08f);
		case 3: return FLinearColor(0.72f, 0.55f, 0.16f);
		case 4: return FLinearColor(0.85f, 0.85f, 0.82f);
		default: return FLinearColor(0.1f, 0.38f, 0.16f);
		}
	}

	FLinearColor SkinFromMap(float U, float V)
	{
		const float Light = FMath::Lerp(0.16f, 0.88f, FMath::Clamp(U, 0.f, 1.f));
		const FLinearColor Cool(Light * 0.82f, Light * 0.64f, Light * 0.52f);
		const FLinearColor Warm(Light, Light * 0.52f, Light * 0.34f);
		return FMath::Lerp(Cool, Warm, FMath::Clamp(V, 0.f, 1.f));
	}
}

void AFBBoxerCharacter::PreviewCreatorLook(const FFBPresentation& Look)
{
	FighterRecord.Look = Look;
	BaseSkin = SkinFromMap(Look.SkinU, Look.SkinV);
	ApplyPresentation();
}

void AFBBoxerCharacter::ApplyPresentation()
{
	EnsureDamageMarkers();
	const FFBPresentation& Look = FighterRecord.Look;
	BaseSkin = SkinFromMap(Look.SkinU, Look.SkinV);
	TintMesh(HeadMesh, BaseSkin);
	TintMesh(BodyMesh, BaseSkin);
	TintSkeletalMesh(BaseSkin);
	if (HairMesh)
	{
		const bool bShow = Look.Hair != EFBHairStyle::Bald;
		HairMesh->SetVisibility(bShow);
		HairMesh->SetHiddenInGame(!bShow);
		const float Bulk = Look.Hair == EFBHairStyle::Braids ? 0.12f : 0.08f;
		HairMesh->SetRelativeScale3D(FVector(Bulk));
		const FLinearColor Dark(0.08f, 0.07f, 0.06f);
		TintMesh(HairMesh, FMath::Lerp(Dark, FLinearColor(0.62f, 0.62f, 0.6f), Look.HairGray));
	}
	if (BeardMesh)
	{
		const bool bShow = Look.Beard > 0 || Look.Mustache > 0;
		BeardMesh->SetVisibility(bShow);
		BeardMesh->SetHiddenInGame(!bShow);
		const float Bulk = 0.03f + 0.03f * static_cast<float>(FMath::Clamp(Look.Beard, 0, 3));
		BeardMesh->SetRelativeScale3D(FVector(Bulk, Bulk * 0.7f, Look.Mustache > 0 ? Bulk * 0.4f : Bulk * 0.2f));
		TintMesh(BeardMesh, FMath::Lerp(FLinearColor(0.08f, 0.07f, 0.06f), FLinearColor(0.7f, 0.7f, 0.68f), Look.HairGray));
	}
	if (TattooMesh)
	{
		const bool bShow = Look.Tattoos > 0.15f && Look.TattooPlace > 0;
		TattooMesh->SetVisibility(bShow);
		TattooMesh->SetHiddenInGame(!bShow);
		const float Place = static_cast<float>(FMath::Clamp(Look.TattooPlace, 1, 3));
		TattooMesh->SetRelativeLocation(FVector(8.f, -6.f + Place * 2.f, 4.f + Place));
		TattooMesh->SetRelativeScale3D(FVector(0.03f, 0.06f * Look.Tattoos, 0.02f));
	}
	if (ScarMesh)
	{
		const bool bShow = Look.Scar > 0;
		ScarMesh->SetVisibility(bShow);
		ScarMesh->SetHiddenInGame(!bShow);
		const float Shift = (Look.Asymmetry - 0.5f) * 4.f;
		ScarMesh->SetRelativeLocation(FVector(10.f, Shift, 6.f - static_cast<float>(Look.Scar)));
	}
	const float Muscle = FMath::Clamp(FMath::Max(Look.Muscle, Look.Muscularity), 0.f, 1.f);
	const float Glove = 0.18f + 0.05f * Muscle;
	const FLinearColor Gloves = GearPalette(Look.GloveColor);
	if (LeadGloveMesh)
	{
		LeadGloveMesh->SetRelativeScale3D(FVector(bUsingMannequin ? Glove * 0.7f : Glove));
		TintMesh(LeadGloveMesh, Gloves);
	}
	if (RearGloveMesh)
	{
		RearGloveMesh->SetRelativeScale3D(FVector(bUsingMannequin ? Glove * 0.7f : Glove));
		TintMesh(RearGloveMesh, Gloves);
	}
}

void AFBBoxerCharacter::OnRep_Stance()
{
	if (bUsingMannequin)
	{
		AttachGlovesToHands();
	}
}

void AFBBoxerCharacter::AuthoritySetClinch(bool bDown)
{
	if (!HasAuthority())
	{
		return;
	}
	bInClinch = bDown;
	if (!bDown)
	{
		ClinchTurnMeter = 0.f;
	}
}

void AFBBoxerCharacter::AuthorityAddClinchMeter(float Amount)
{
	if (!HasAuthority() || !bInClinch)
	{
		return;
	}
	ClinchTurnMeter = FMath::Clamp(ClinchTurnMeter + Amount, 0.f, 100.f);
}

void AFBBoxerCharacter::AuthorityResetClinchMeter()
{
	if (!HasAuthority())
	{
		return;
	}
	ClinchTurnMeter = 0.f;
	++ClinchTurnsLanded;
}

void AFBBoxerCharacter::AuthorityToggleStance()
{
	if (!HasAuthority() || bKnockedDown || bInClinch)
	{
		return;
	}
	Stance = (Stance == EFBStance::Southpaw) ? EFBStance::Orthodox : EFBStance::Southpaw;
	OnRep_Stance();
}

void AFBBoxerCharacter::AuthorityToggleLoose()
{
	if (!HasAuthority() || bKnockedDown)
	{
		return;
	}
	bLooseMovement = !bLooseMovement;
}

void AFBBoxerCharacter::AuthorityStartTaunt()
{
	if (!HasAuthority() || bKnockedDown || bInClinch)
	{
		return;
	}
	TauntEndTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + 1.15f;
}

int32 AFBBoxerCharacter::AuthorityNoteFoul()
{
	if (!HasAuthority())
	{
		return FoulStrikes;
	}
	return ++FoulStrikes;
}

void AFBBoxerCharacter::BeginDefenseSlip(FVector2D Axis)
{
	const FVector2D N = Axis.GetSafeNormal();
	if (N.IsNearlyZero())
	{
		return;
	}

	float Pitch = 18.f;
	float Roll = 0.f;
	if (FMath::Abs(N.X) >= FMath::Abs(N.Y))
	{
		// Stick right is +X. Positive roll moves the head to the fighter's left.
		Roll = -N.X * SideRollDegrees;
		Pitch = 18.f;
	}
	else if (N.Y < 0.f)
	{
		Pitch = LeanBackDegrees;
		Roll = -N.X * 14.f;
	}
	else
	{
		Pitch = RollUnderDegrees;
		const float LeadX = (Stance == EFBStance::Southpaw) ? 1.f : -1.f;
		const float Side = FMath::Abs(N.X) > 0.2f ? N.X : LeadX;
		Roll = -Side * RollUnderSideDegrees;
	}

	DefenseSlipPitch = Pitch;
	DefenseSlipRoll = Roll;
	DefenseSlipStartTime = GetSyncedTimeSeconds();
	DefenseSlipEndTime = DefenseSlipStartTime + 0.70f;
}

bool AFBBoxerCharacter::IsHeadEvasionOnTime(float ActiveWindowStart) const
{
	const float Now = GetSyncedTimeSeconds();
	if (DefenseSlipStartTime < 0.f || Now >= DefenseSlipEndTime)
	{
		return false;
	}
	return DefenseSlipStartTime <= ActiveWindowStart;
}

void AFBBoxerCharacter::AuthoritySlip(FVector2D Axis)
{
	if (!HasAuthority() || bKnockedDown || bInClinch || Axis.Size() < 0.2f)
	{
		return;
	}
	const FVector Local(Axis.Y, Axis.X, 0.f);
	const FVector World = GetActorRotation().RotateVector(Local.GetSafeNormal());
	const float Slow = GetMoveSpeedScale();
	LaunchCharacter(World * 640.f * Slow * CombatDerived.ReflexWeave, true, false);
	GrantCreatedAngle(0.35f);
	BeginDefenseSlip(Axis);
}

void AFBBoxerCharacter::ServerSubmitClinchSteer_Implementation(float Axis)
{
	if (!HasAuthority() || !bInClinch)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (AFBSparringGameMode* Mode = World->GetAuthGameMode<AFBSparringGameMode>())
		{
			Mode->AddClinchSteer(this, Axis);
		}
	}
}

void AFBBoxerCharacter::EnsureDamageMarkers()
{
	if (CheekBruiseMesh)
	{
		return;
	}
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Sphere || !Cube)
	{
		return;
	}

	auto MakeMarker = [&](const TCHAR* Name, UStaticMesh* MarkerMesh) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, Name);
		Comp->SetStaticMesh(MarkerMesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetVisibility(false);
		Comp->SetHiddenInGame(true);
		Comp->RegisterComponent();
		return Comp;
	};

	CheekBruiseMesh = MakeMarker(TEXT("CheekBruise"), Sphere);
	SwollenEyeMesh = MakeMarker(TEXT("SwollenEye"), Sphere);
	BloodyNoseMesh = MakeMarker(TEXT("BloodyNose"), Sphere);
	BrowCutMesh = MakeMarker(TEXT("BrowCut"), Cube);
	RibBruiseMesh = MakeMarker(TEXT("RibBruise"), Sphere);
	HairMesh = MakeMarker(TEXT("Hair"), Sphere);
	TattooMesh = MakeMarker(TEXT("Tattoo"), Cube);
	BeardMesh = MakeMarker(TEXT("Beard"), Sphere);
	ScarMesh = MakeMarker(TEXT("Scar"), Cube);

	TintMesh(CheekBruiseMesh, FLinearColor(0.35f, 0.12f, 0.22f));
	TintMesh(SwollenEyeMesh, FLinearColor(0.62f, 0.28f, 0.34f));
	TintMesh(BloodyNoseMesh, FLinearColor(0.55f, 0.05f, 0.06f));
	TintMesh(BrowCutMesh, FLinearColor(0.62f, 0.04f, 0.05f));
	TintMesh(RibBruiseMesh, FLinearColor(0.42f, 0.16f, 0.18f));

	USceneComponent* HeadParent = (bUsingMannequin && PoseMesh) ? static_cast<USceneComponent*>(PoseMesh) : static_cast<USceneComponent*>(HeadMesh);
	USceneComponent* RibParent = (bUsingMannequin && PoseMesh) ? static_cast<USceneComponent*>(PoseMesh) : static_cast<USceneComponent*>(BodyMesh);
	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, false);
	const FName HeadBone = bUsingMannequin ? FName(TEXT("head")) : NAME_None;
	const FName RibBone = bUsingMannequin ? FName(TEXT("spine_02")) : NAME_None;

	auto Seat = [&](UStaticMeshComponent* Comp, USceneComponent* Parent, FName Bone, const FVector& Loc, const FVector& Scale)
	{
		if (!Comp || !Parent)
		{
			return;
		}
		Comp->AttachToComponent(Parent, Rules, Bone);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeScale3D(Scale);
	};

	Seat(CheekBruiseMesh, HeadParent, HeadBone, FVector(10.f, -5.f, 2.f), FVector(0.045f));
	Seat(SwollenEyeMesh, HeadParent, HeadBone, FVector(9.f, 4.5f, 5.f), FVector(0.04f));
	Seat(BloodyNoseMesh, HeadParent, HeadBone, FVector(12.f, 0.f, -1.f), FVector(0.03f, 0.03f, 0.05f));
	Seat(BrowCutMesh, HeadParent, HeadBone, FVector(10.f, 3.f, 8.f), FVector(0.01f, 0.04f, 0.008f));
	Seat(RibBruiseMesh, RibParent, RibBone, FVector(16.f, 6.f, 4.f), FVector(0.08f, 0.10f, 0.05f));
	Seat(HairMesh, HeadParent, HeadBone, FVector(0.f, 0.f, 14.f), FVector(0.08f));
	Seat(BeardMesh, HeadParent, HeadBone, FVector(8.f, 0.f, -6.f), FVector(0.04f));
	Seat(TattooMesh, RibParent, RibBone, FVector(8.f, -8.f, 8.f), FVector(0.03f, 0.06f, 0.02f));
	Seat(ScarMesh, HeadParent, HeadBone, FVector(10.f, 0.f, 6.f), FVector(0.012f, 0.03f, 0.006f));
	if (HairMesh)
	{
		HairMesh->SetVisibility(false);
		HairMesh->SetHiddenInGame(true);
	}
	if (TattooMesh)
	{
		TattooMesh->SetVisibility(false);
		TattooMesh->SetHiddenInGame(true);
	}
	if (BeardMesh)
	{
		BeardMesh->SetVisibility(false);
		BeardMesh->SetHiddenInGame(true);
	}
	if (ScarMesh)
	{
		ScarMesh->SetVisibility(false);
		ScarMesh->SetHiddenInGame(true);
		TintMesh(ScarMesh, FLinearColor(0.55f, 0.42f, 0.4f));
	}
}

void AFBBoxerCharacter::ApplyVisibleDamage()
{
	if (!CheekBruiseMesh)
	{
		EnsureDamageMarkers();
	}
	const float HeadAlpha = (MaxHeadDamage > 1.f) ? FMath::Clamp(HeadDamage / MaxHeadDamage, 0.f, 1.f) : 0.f;
	const float BodyAlpha = (MaxBodyDamage > 1.f) ? FMath::Clamp(BodyDamage / MaxBodyDamage, 0.f, 1.f) : 0.f;

	auto Show = [](UStaticMeshComponent* Comp, bool bShow, const FVector& Scale)
	{
		if (!Comp)
		{
			return;
		}
		Comp->SetVisibility(bShow);
		Comp->SetHiddenInGame(!bShow);
		if (bShow)
		{
			Comp->SetRelativeScale3D(Scale);
		}
	};

	Show(CheekBruiseMesh, HeadAlpha > 0.12f, FVector(0.035f + HeadAlpha * 0.05f, 0.045f + HeadAlpha * 0.04f, 0.02f + HeadAlpha * 0.02f));
	Show(SwollenEyeMesh, HeadAlpha > 0.22f, FVector(0.045f + HeadAlpha * 0.07f, 0.035f + HeadAlpha * 0.03f, 0.03f + HeadAlpha * 0.08f));
	Show(BloodyNoseMesh, HeadAlpha > 0.34f, FVector(0.028f, 0.028f, 0.04f + HeadAlpha * 0.08f));
	Show(BrowCutMesh, HeadAlpha > 0.55f, FVector(0.012f, 0.035f + HeadAlpha * 0.02f, 0.008f));
	Show(RibBruiseMesh, BodyAlpha > 0.2f, FVector(0.07f + BodyAlpha * 0.08f, 0.09f + BodyAlpha * 0.06f, 0.04f + BodyAlpha * 0.04f));

	const FLinearColor Bruised(0.48f, 0.24f, 0.28f);
	TintMesh(HeadMesh, FMath::Lerp(BaseSkin, Bruised, HeadAlpha));
	TintMesh(BodyMesh, FMath::Lerp(BaseSkin, FLinearColor(0.52f, 0.28f, 0.26f), BodyAlpha * 0.85f));
	if (BrowCutMesh)
	{
		const float BrowShift = (FighterRecord.Look.Brow - 0.5f) * 4.f;
		const float JawShift = (FighterRecord.Look.Jaw - 0.5f) * 3.f;
		BrowCutMesh->SetRelativeLocation(FVector(10.f, 3.f + JawShift, 8.f + BrowShift));
	}
	if (BloodyNoseMesh)
	{
		const float NoseShift = (FighterRecord.Look.Nose - 0.5f) * 3.f;
		BloodyNoseMesh->SetRelativeLocation(FVector(12.f + NoseShift, 0.f, -1.f));
	}
}
