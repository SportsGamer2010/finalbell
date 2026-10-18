#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Camera/FBBroadcastCamera.h"
#include "Ring/FBRingBounds.h"
#include "Input/FBRuntimeInputLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

AFBBoxerCharacter::AFBBoxerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->MaxWalkSpeed = 320.f;
	bUseControllerRotationYaw = false;
	ConfigureBodyCollision();

	CombatComponent = CreateDefaultSubobject<UFBCombatComponent>(TEXT("CombatComponent"));

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
		// The visible body is the posed limbs below. The skeletal mesh stays hidden
		// so a mannequin is not standing inside the boxer.
		Skel->SetSkeletalMesh(nullptr);
		Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Skel->SetVisibility(false);
		Skel->SetHiddenInGame(true);
		Skel->SetCastShadow(false);
	}

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
	RefreshCombatPresentation();
}

void AFBBoxerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFBBoxerCharacter, Stance);
	DOREPLIFETIME(AFBBoxerCharacter, CornerSkin);
	DOREPLIFETIME(AFBBoxerCharacter, AccumulatedDamage);
	DOREPLIFETIME(AFBBoxerCharacter, AppliedAttackInstanceIds);
	DOREPLIFETIME(AFBBoxerCharacter, FacingLockYaw);
	DOREPLIFETIME(AFBBoxerCharacter, FacingLockServerEndTime);
	DOREPLIFETIME(AFBBoxerCharacter, AngleBonusServerEndTime);
}

void AFBBoxerCharacter::OnRep_CornerSkin()
{
	SetupMannequinVisuals();
	ApplyBasicSkin(CornerSkin);
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
		Move->BrakingDecelerationWalking = 1800.f;
		Move->GroundFriction = 8.f;
		Move->MaxAcceleration = 2400.f;
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

bool AFBBoxerCharacter::DoesPunchConnect(EFBAttackId Attack, AActor* Target) const
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return false;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		return false;
	}

	FVector Center = GetActorLocation() + FVector(0.f, 0.f, 54.f) + Forward * 68.f;
	float Radius = 40.f;
	const bool bBody = CombatComponent && CombatComponent->IsBodyShot();
	switch (Attack)
	{
	case EFBAttackId::LeadHook:
		Center += Right * -32.f;
		Radius = 36.f;
		break;
	case EFBAttackId::RearHook:
		Center += Right * 32.f;
		Radius = 36.f;
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
		Center.Z -= 34.f;
	}

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

void AFBBoxerCharacter::ServerSetFacingOffset_Implementation(float OffsetDegrees)
{
	FacingYawOffset = FMath::Clamp(OffsetDegrees, -MaxFacingOffsetDegrees, MaxFacingOffsetDegrees);
}

FName AFBBoxerCharacter::LeadHandSocket() const
{
	return (Stance == EFBStance::Southpaw) ? FName(TEXT("hand_r")) : FName(TEXT("hand_l"));
}

FName AFBBoxerCharacter::RearHandSocket() const
{
	return (Stance == EFBStance::Southpaw) ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"));
}

void AFBBoxerCharacter::AttachGlovesToHands()
{
	USkeletalMeshComponent* Skel = GetMesh();
	if (!Skel || !bUsingMannequin)
	{
		return;
	}

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);

	if (LeadGloveMesh)
	{
		LeadGloveMesh->AttachToComponent(Skel, Rules, LeadHandSocket());
		LeadGloveMesh->SetRelativeLocation(FVector(8.f, 0.f, 0.f));
		LeadGloveMesh->SetRelativeScale3D(FVector(0.30f));
		LeadGloveMesh->SetVisibility(true);
		LeadGloveMesh->SetHiddenInGame(false);
	}
	if (RearGloveMesh)
	{
		RearGloveMesh->AttachToComponent(Skel, Rules, RearHandSocket());
		RearGloveMesh->SetRelativeLocation(FVector(8.f, 0.f, 0.f));
		RearGloveMesh->SetRelativeScale3D(FVector(0.30f));
		RearGloveMesh->SetVisibility(true);
		RearGloveMesh->SetHiddenInGame(false);
	}
}

void AFBBoxerCharacter::SetupMannequinVisuals()
{
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetVisibility(false);
		Skel->SetHiddenInGame(true);
		Skel->SetCastShadow(false);
		Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	bUsingMannequin = false;
	auto Show = [](UStaticMeshComponent* MeshComp)
	{
		if (MeshComp)
		{
			MeshComp->SetVisibility(true);
			MeshComp->SetHiddenInGame(false);
		}
	};
	Show(BodyMesh);
	Show(HeadMesh);
	Show(TrunksMesh);
	Show(LeadGloveMesh);
	Show(RearGloveMesh);
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
}

void AFBBoxerCharacter::UpdateBoxerPose(float DeltaSeconds)
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
	else if (Guard == EFBGuardState::Weave)
	{
		Head.Z -= 16.f;
		Head.Y += (Stance == EFBStance::Southpaw) ? -8.f : 8.f;
		LeadHand.Z -= 12.f;
		RearHand.Z -= 12.f;
	}

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

	const float BodyDrop = bDisplayBody ? 26.f : 0.f;
	switch (DisplayAttack)
	{
	case EFBAttackId::Jab:
		PunchArm(true, FVector(40.f, -12.f, 32.f - BodyDrop), FVector(82.f, -6.f, 44.f - BodyDrop));
		break;
	case EFBAttackId::Cross:
		PunchArm(false, FVector(36.f, 22.f, 28.f - BodyDrop), FVector(96.f, 20.f, 46.f - BodyDrop));
		break;
	case EFBAttackId::LeadHook:
		PunchArm(true, FVector(20.f, -16.f, 28.f - BodyDrop), FVector(36.f, 32.f, 42.f - BodyDrop));
		break;
	case EFBAttackId::RearHook:
		PunchArm(false, FVector(28.f, 26.f, 30.f - BodyDrop), FVector(52.f, -36.f, 46.f - BodyDrop));
		break;
	case EFBAttackId::LeadUppercut:
		PunchArm(true, FVector(18.f, -18.f, 2.f - BodyDrop * 0.4f), FVector(40.f, -4.f, 68.f - BodyDrop));
		break;
	case EFBAttackId::RearUppercut:
		PunchArm(false, FVector(24.f, 24.f, 0.f - BodyDrop * 0.4f), FVector(58.f, 16.f, 78.f - BodyDrop));
		break;
	default:
		break;
	}

	if (BodyMesh)
	{
		BodyMesh->SetRelativeLocation(FVector(2.f, 0.f, Guard == EFBGuardState::Weave ? -2.f : 8.f));
		BodyMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BodyMesh->SetRelativeScale3D(FVector(0.26f, 0.38f, 0.48f));
	}
	if (TrunksMesh)
	{
		TrunksMesh->SetRelativeLocation(FVector(0.f, 0.f, -28.f));
		TrunksMesh->SetRelativeRotation(FRotator::ZeroRotator);
		TrunksMesh->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.18f));
	}
	const float HeadScale = 0.30f * (1.f + HitFlashTime);
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

void AFBBoxerCharacter::HandleImpactPresentation(const FFBImpactEvent& Impact)
{
	if (Impact.Result == EFBContactResult::CleanHit)
	{
		HitFlashTime = 0.18f;
		if (AFBBoxerCharacter* Opp = FindOpponent())
		{
			Opp->HitFlashTime = 0.18f;
		}
	}
	else if (Impact.Result == EFBContactResult::Blocked)
	{
		HitFlashTime = 0.08f;
	}
}

void AFBBoxerCharacter::EnsureRuntimeInput()
{
	if (DefaultMappingContext && MoveAction && LookAction && TurnAction && JabAction && CrossAction && LeadHookAction && RearHookAction && LeadUppercutAction && RearUppercutAction && BlockAction && WeaveAction)
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
}

void AFBBoxerCharacter::ApplyMappingContext()
{
	EnsureRuntimeInput();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
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
		for (TActorIterator<AFBRingBounds> It(GetWorld()); It; ++It)
		{
			const FVector Loc = GetActorLocation();
			const FVector Clamped = It->ClampToRing(Loc);
			if (!Clamped.Equals(Loc, 0.5f))
			{
				SetActorLocation(Clamped, false, nullptr, ETeleportType::TeleportPhysics);
			}
			break;
		}
	}

	const bool bDriveFacing = IsLocallyControlled() || HasAuthority();
	if (bDriveFacing)
	{
		UpdateFacingOffset(DeltaSeconds);
		FaceOpponent(DeltaSeconds);
	}

	if (IsLocallyControlled() && PersonalOrbitCamera)
	{
		PersonalOrbitCamera->SetFighters(this, FindOpponent());
	}

	RefreshCombatPresentation();
	UpdateBoxerPose(DeltaSeconds);

	if (HitFlashTime > 0.f)
	{
		HitFlashTime -= DeltaSeconds;
	}
}

void AFBBoxerCharacter::UpdateFacingOffset(float DeltaSeconds)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	FacingYawOffset = FMath::FInterpTo(FacingYawOffset, FacingYawOffsetTarget, DeltaSeconds, 9.f);
	FacingYawOffset = FMath::Clamp(FacingYawOffset, -MaxFacingOffsetDegrees, MaxFacingOffsetDegrees);

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

	const float OpponentYaw = ToOpp.Rotation().Yaw;
	const float DeltaYaw = FMath::Clamp(
		FMath::FindDeltaAngleDegrees(OpponentYaw, OpponentYaw + FacingYawOffset),
		-MaxFacingOffsetDegrees,
		MaxFacingOffsetDegrees);
	const FRotator TargetRot(0.f, OpponentYaw + DeltaYaw, 0.f);

	const bool bPunching = CombatComponent && CombatComponent->IsAttackCommitted();
	const float Speed = bPunching ? PunchLockedTurnSpeed : FaceTurnSpeed;
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
		}
		if (CrossAction)
		{
			EIC->BindAction(CrossAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnCross);
		}
		if (LeadHookAction)
		{
			EIC->BindAction(LeadHookAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLeadHook);
		}
		if (RearHookAction)
		{
			EIC->BindAction(RearHookAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnRearHook);
		}
		if (LeadUppercutAction)
		{
			EIC->BindAction(LeadUppercutAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnLeadUppercut);
		}
		if (RearUppercutAction)
		{
			EIC->BindAction(RearUppercutAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnRearUppercut);
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

void AFBBoxerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.SizeSquared() < FMath::Square(0.35f))
	{
		return;
	}
	const FRotator Facing(0.f, GetActorRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), Axis.Y * MoveSpeedScale);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), Axis.X * MoveSpeedScale);
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

void AFBBoxerCharacter::OnTurnStick(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	FacingYawOffsetTarget = FMath::Clamp(Axis.X, -1.f, 1.f) * MaxFacingOffsetDegrees;
}

void AFBBoxerCharacter::OnTurnStickReleased()
{
	FacingYawOffsetTarget = 0.f;
}

void AFBBoxerCharacter::OnPunchStickReleased()
{
	bPunchStickConsumed = false;
}

void AFBBoxerCharacter::OnPunchStick(const FInputActionValue& Value)
{
	if (bPunchStickConsumed || PunchStickCooldown > 0.f)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.Size() < 0.35f)
	{
		return;
	}

	// Competitor RS flick: up-left jab, up-right cross, left hook, right hook, down-left/right uppercut
	EFBAttackId Attack = EFBAttackId::None;
	const float X = Axis.X;
	const float Y = Axis.Y;
	if (Y > 0.35f && FMath::Abs(X) <= 0.55f)
	{
		Attack = (X < 0.f) ? EFBAttackId::Jab : EFBAttackId::Cross;
	}
	else if (Y < -0.35f)
	{
		Attack = (X < 0.f) ? EFBAttackId::LeadUppercut : EFBAttackId::RearUppercut;
	}
	else if (X < -0.55f)
	{
		Attack = EFBAttackId::LeadHook;
	}
	else if (X > 0.55f)
	{
		Attack = EFBAttackId::RearHook;
	}

	if (Attack == EFBAttackId::None)
	{
		return;
	}

	bPunchStickConsumed = true;
	PunchStickCooldown = 0.22f;
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(Attack, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

EFBAttackId AFBBoxerCharacter::ResolveUppercutVariant(EFBAttackId BaseHook) const
{
	// Holding body mod + hook face button → uppercut (Competitor combo feel)
	if (!bBodyModifierHeld)
	{
		return BaseHook;
	}
	if (BaseHook == EFBAttackId::LeadHook)
	{
		return EFBAttackId::LeadUppercut;
	}
	if (BaseHook == EFBAttackId::RearHook)
	{
		return EFBAttackId::RearUppercut;
	}
	return BaseHook;
}

void AFBBoxerCharacter::OnJab()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Jab, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnCross()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Cross, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnLeadHook()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(ResolveUppercutVariant(EFBAttackId::LeadHook), bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnRearHook()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(ResolveUppercutVariant(EFBAttackId::RearHook), bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnLeadUppercut()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::LeadUppercut, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnRearUppercut()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::RearUppercut, bGuard, bGuard ? CombatComponent->GetGuardState() : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnBlockStarted()
{
	SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::High);
}

void AFBBoxerCharacter::OnBlockCompleted()
{
	SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
}

void AFBBoxerCharacter::OnWeaveStarted()
{
	SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::Weave);
}

void AFBBoxerCharacter::OnWeaveCompleted()
{
	SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
}

void AFBBoxerCharacter::OnBodyModStarted()
{
	bBodyModifierHeld = true;
}

void AFBBoxerCharacter::OnBodyModCompleted()
{
	bBodyModifierHeld = false;
}

void AFBBoxerCharacter::OnPowerModStarted()
{
	bPowerModifierHeld = true;
}

void AFBBoxerCharacter::OnPowerModCompleted()
{
	bPowerModifierHeld = false;
}

void AFBBoxerCharacter::SubmitLocalIntent(EFBAttackId Attack, bool bGuardHeld, EFBGuardState Guard)
{
	FFBCombatIntent Intent;
	Intent.Sequence = ++LocalIntentSequence;
	Intent.ClientTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Intent.Attack = Attack;
	Intent.bGuardHeld = bGuardHeld;
	Intent.Guard = Guard;
	Intent.SchemaVersion = TEXT("1.0.0");
	Intent.bBodyTarget = bBodyModifierHeld;
	Intent.bPowerPunch = bPowerModifierHeld;

	if (Attack != EFBAttackId::None && IsLocallyControlled() && GEngine)
	{
		const TCHAR* Label = TEXT("Punch");
		switch (Attack)
		{
		case EFBAttackId::Jab: Label = TEXT("Jab"); break;
		case EFBAttackId::Cross: Label = TEXT("Straight"); break;
		case EFBAttackId::LeadHook: Label = TEXT("Lead hook"); break;
		case EFBAttackId::RearHook: Label = TEXT("Rear hook"); break;
		case EFBAttackId::LeadUppercut: Label = TEXT("Lead uppercut"); break;
		case EFBAttackId::RearUppercut: Label = TEXT("Rear uppercut"); break;
		default: break;
		}
		const FString Line = Intent.bBodyTarget
			? FString::Printf(TEXT("%s — body%s"), Label, Intent.bPowerPunch ? TEXT(" — power") : TEXT(""))
			: FString::Printf(TEXT("%s%s"), Label, Intent.bPowerPunch ? TEXT(" — power") : TEXT(""));
		GEngine->AddOnScreenDebugMessage(1, 0.9f, FColor::White, Line);
	}

	if ((Attack == EFBAttackId::LeadHook || Attack == EFBAttackId::RearHook) && CombatComponent)
	{
		BeginHookFacingLock(CombatComponent->StartupSeconds + CombatComponent->ActiveWindowSeconds + 0.12f);
	}

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

bool AFBBoxerCharacter::ServerSubmitCombatIntent_Validate(const FFBCombatIntent& Intent)
{
	return Intent.Sequence > 0 && Intent.SchemaVersion.Len() > 0;
}

void AFBBoxerCharacter::ServerSubmitCombatIntent_Implementation(const FFBCombatIntent& Intent)
{
	if (CombatComponent)
	{
		CombatComponent->ServerTryProcessIntent(Intent, FindCombatTarget());
	}
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
	for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void AFBBoxerCharacter::ApplyAuthoritativeDamage(float Amount, int32 AttackInstanceId)
{
	if (!HasAuthority())
	{
		return;
	}
	if (AppliedAttackInstanceIds.Contains(AttackInstanceId))
	{
		return;
	}
	AppliedAttackInstanceIds.Add(AttackInstanceId);
	AccumulatedDamage += FMath::Max(0.f, Amount);
	HitFlashTime = 0.18f;
}
