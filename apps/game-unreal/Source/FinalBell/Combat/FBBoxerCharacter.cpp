#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Camera/FBBroadcastCamera.h"
#include "Ring/FBRingBounds.h"
#include "Input/FBRuntimeInputLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include "Kismet/GameplayStatics.h"

AFBBoxerCharacter::AFBBoxerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->MaxWalkSpeed = 320.f;
	bUseControllerRotationYaw = false;

	CombatComponent = CreateDefaultSubobject<UFBCombatComponent>(TEXT("CombatComponent"));

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
	LeadGloveMesh = AttachMesh(TEXT("LeadGloveMesh"), Sph, FVector(35.f, -18.f, 20.f), FVector(0.22f, 0.22f, 0.22f));
	RearGloveMesh = AttachMesh(TEXT("RearGloveMesh"), Sph, FVector(20.f, 22.f, 18.f), FVector(0.22f, 0.22f, 0.22f));
}

void AFBBoxerCharacter::BeginPlay()
{
	Super::BeginPlay();
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
}

void AFBBoxerCharacter::OnRep_CornerSkin()
{
	ApplyBasicSkin(CornerSkin);
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

void AFBBoxerCharacter::ApplyBasicSkin(EFBCornerSkin Corner)
{
	CornerSkin = Corner;
	const bool bRed = Corner == EFBCornerSkin::RedCorner;
	const FLinearColor Skin(0.86f, 0.68f, 0.52f);
	const FLinearColor Trunks = bRed ? FLinearColor(0.75f, 0.08f, 0.08f) : FLinearColor(0.08f, 0.25f, 0.75f);
	const FLinearColor Gloves = bRed ? FLinearColor(0.85f, 0.12f, 0.1f) : FLinearColor(0.12f, 0.35f, 0.9f);

	TintMesh(BodyMesh, Skin);
	TintMesh(HeadMesh, Skin);
	TintMesh(TrunksMesh, Trunks);
	TintMesh(LeadGloveMesh, Gloves);
	TintMesh(RearGloveMesh, Gloves);
}

void AFBBoxerCharacter::ApplyPlaceholderAppearance(bool bIsOpponent)
{
	ApplyBasicSkin(bIsOpponent ? EFBCornerSkin::RedCorner : EFBCornerSkin::BlueCorner);
}

void AFBBoxerCharacter::RefreshCombatPresentation()
{
	if (!CombatComponent)
	{
		return;
	}

	const bool bGuarding = CombatComponent->GetGuardState() != EFBGuardState::Open;
	const EFBAttackPhase Phase = CombatComponent->GetAttackPhase();
	const EFBAttackId Attack = CombatComponent->GetActiveAttack();

	FVector Lead = FVector(35.f, -18.f, 20.f);
	FVector Rear = FVector(20.f, 22.f, 18.f);

	if (bGuarding)
	{
		Lead = FVector(28.f, -12.f, 38.f);
		Rear = FVector(28.f, 12.f, 38.f);
	}

	if (Phase == EFBAttackPhase::Startup || Phase == EFBAttackPhase::Active)
	{
		const float Extend = (Phase == EFBAttackPhase::Active) ? 55.f : 25.f;
		if (Attack == EFBAttackId::Jab)
		{
			Lead.X += Extend;
		}
		else if (Attack == EFBAttackId::Cross)
		{
			Rear.X += Extend + 10.f;
		}
	}

	if (LeadGloveMesh)
	{
		LeadGloveMesh->SetRelativeLocation(Lead);
	}
	if (RearGloveMesh)
	{
		RearGloveMesh->SetRelativeLocation(Rear);
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
	if (DefaultMappingContext && MoveAction && LookAction && JabAction && CrossAction && GuardAction)
	{
		return;
	}

	const FFBRuntimeInputBundle Bundle = UFBRuntimeInputLibrary::EnsureBoxerInput(this);
	DefaultMappingContext = Bundle.MappingContext;
	MoveAction = Bundle.Move;
	LookAction = Bundle.Look;
	JabAction = Bundle.Jab;
	CrossAction = Bundle.Cross;
	GuardAction = Bundle.Guard;
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
			GetActorLocation() + FVector(0.f, -700.f, 420.f),
			FRotator(-18.f, 90.f, 0.f),
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

	// Keep fighters on the canvas (authority + local prediction).
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

	if (IsLocallyControlled())
	{
		FaceOpponent(DeltaSeconds);
		if (PersonalOrbitCamera)
		{
			PersonalOrbitCamera->SetFighters(this, FindOpponent());
		}
	}

	RefreshCombatPresentation();

	if (HitFlashTime > 0.f)
	{
		HitFlashTime -= DeltaSeconds;
		const float Pulse = 1.f + HitFlashTime * 2.f;
		if (HeadMesh)
		{
			HeadMesh->SetRelativeScale3D(FVector(0.42f) * Pulse);
		}
	}
	else if (HeadMesh)
	{
		HeadMesh->SetRelativeScale3D(FVector(0.42f));
	}
}

void AFBBoxerCharacter::FaceOpponent(float DeltaSeconds)
{
	const AFBBoxerCharacter* Opp = FindOpponent();
	if (!Opp)
	{
		return;
	}

	const bool bLocked = CombatComponent && CombatComponent->IsAttackCommitted();
	const float Speed = bLocked ? PunchLockedTurnSpeed : FaceTurnSpeed;

	const FVector ToOpp = (Opp->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToOpp.IsNearlyZero())
	{
		return;
	}

	const FRotator Target(0.f, ToOpp.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, Speed));
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
		if (JabAction)
		{
			EIC->BindAction(JabAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnJab);
		}
		if (CrossAction)
		{
			EIC->BindAction(CrossAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnCross);
		}
		if (GuardAction)
		{
			EIC->BindAction(GuardAction, ETriggerEvent::Started, this, &AFBBoxerCharacter::OnGuardStarted);
			EIC->BindAction(GuardAction, ETriggerEvent::Completed, this, &AFBBoxerCharacter::OnGuardCompleted);
			EIC->BindAction(GuardAction, ETriggerEvent::Canceled, this, &AFBBoxerCharacter::OnGuardCompleted);
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

void AFBBoxerCharacter::OnJab()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Jab, bGuard, bGuard ? EFBGuardState::High : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnCross()
{
	const bool bGuard = CombatComponent && CombatComponent->GetGuardState() != EFBGuardState::Open;
	SubmitLocalIntent(EFBAttackId::Cross, bGuard, bGuard ? EFBGuardState::High : EFBGuardState::Open);
}

void AFBBoxerCharacter::OnGuardStarted()
{
	SubmitLocalIntent(EFBAttackId::None, true, EFBGuardState::High);
}

void AFBBoxerCharacter::OnGuardCompleted()
{
	SubmitLocalIntent(EFBAttackId::None, false, EFBGuardState::Open);
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

	if (HasAuthority())
	{
		if (CombatComponent)
		{
			CombatComponent->ServerTryProcessIntent(Intent, FindOpponent());
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
		CombatComponent->ServerTryProcessIntent(Intent, FindOpponent());
	}
}

AFBBoxerCharacter* AFBBoxerCharacter::FindOpponent() const
{
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It && *It != this)
		{
			return *It;
		}
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
