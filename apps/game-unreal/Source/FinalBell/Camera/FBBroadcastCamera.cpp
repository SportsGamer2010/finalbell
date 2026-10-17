#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

AFBBroadcastCamera::AFBBroadcastCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->bUsePawnControlRotation = false;
}

void AFBBroadcastCamera::BeginPlay()
{
	Super::BeginPlay();
	AutoFindFighters();
	ForcePossessedView();
}

void AFBBroadcastCamera::ForcePossessedView()
{
	APlayerController* PC = nullptr;
	if (AFBBoxerCharacter* OwnerBoxer = Cast<AFBBoxerCharacter>(GetOwner()))
	{
		PC = Cast<APlayerController>(OwnerBoxer->GetController());
	}
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}
	ForceViewForController(PC);
}

void AFBBroadcastCamera::ForceViewForController(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	PC->SetViewTargetWithBlend(this, 0.f);
	PC->bShowMouseCursor = false;
}

void AFBBroadcastCamera::SetFighters(AFBBoxerCharacter* Player, AFBBoxerCharacter* Opponent)
{
	PlayerFighter = Player;
	OpponentFighter = Opponent;

	if (Player && Opponent)
	{
		const FVector ToOpp = (Opponent->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		if (!ToOpp.IsNearlyZero())
		{
			// Start behind the player, looking toward the opponent.
			OrbitYaw = ToOpp.Rotation().Yaw + 180.f;
		}
	}

	ForcePossessedView();
}

void AFBBroadcastCamera::AddOrbitInput(FVector2D Axis)
{
	PendingOrbitInput += Axis;
}

void AFBBroadcastCamera::AutoFindFighters()
{
	AFBBoxerCharacter* Local = nullptr;
	AFBBoxerCharacter* Other = nullptr;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		Local = Cast<AFBBoxerCharacter>(PC->GetPawn());
	}

	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == Local)
		{
			continue;
		}
		Other = *It;
		break;
	}

	if (Local)
	{
		PlayerFighter = Local;
	}
	if (Other)
	{
		OpponentFighter = Other;
	}
}

void AFBBroadcastCamera::ResolveFocus(FVector& OutFocus, float& OutDistance) const
{
	const FVector LocPlayer = PlayerFighter.IsValid()
		? PlayerFighter->GetActorLocation()
		: GetActorLocation();
	const FVector LocOpp = OpponentFighter.IsValid()
		? OpponentFighter->GetActorLocation()
		: LocPlayer + FVector(250.f, 0.f, 0.f);

	const FVector Mid = (LocPlayer + LocOpp) * 0.5f;
	OutFocus = FMath::Lerp(Mid, LocOpp, FocusOpponentBias) + FVector(0.f, 0.f, FocusHeight);

	const float Separation = FVector::Dist(LocPlayer, LocOpp);
	OutDistance = FMath::Clamp(BaseDistance + Separation * SeparationDistanceScale, MinDistance, MaxDistance);
}

void AFBBroadcastCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PlayerFighter.IsValid())
	{
		AutoFindFighters();
	}
	if (!PlayerFighter.IsValid())
	{
		return;
	}

	const bool bHasOrbitInput = !PendingOrbitInput.IsNearlyZero(KINDA_SMALL_NUMBER);
	if (bHasOrbitInput)
	{
		OrbitYaw += PendingOrbitInput.X * OrbitYawSpeed * DeltaSeconds;
		OrbitPitch = FMath::Clamp(OrbitPitch + PendingOrbitInput.Y * OrbitPitchSpeed * DeltaSeconds, MinPitch, MaxPitch);
		OrbitInputHoldTime = AutoAlignIdleDelay;
		PendingOrbitInput = FVector2D::ZeroVector;
	}
	else
	{
		OrbitInputHoldTime = FMath::Max(0.f, OrbitInputHoldTime - DeltaSeconds);
	}

	if (bAutoAlignWhenIdle && OrbitInputHoldTime <= 0.f && OpponentFighter.IsValid())
	{
		const FVector ToOpp = (OpponentFighter->GetActorLocation() - PlayerFighter->GetActorLocation()).GetSafeNormal2D();
		if (!ToOpp.IsNearlyZero())
		{
			const float DesiredYaw = ToOpp.Rotation().Yaw + 180.f;
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(OrbitYaw, DesiredYaw);
			const float MaxStep = AutoAlignSpeed * 60.f * DeltaSeconds;
			OrbitYaw += FMath::Clamp(DeltaYaw, -MaxStep, MaxStep);
		}
	}

	FVector Focus;
	float Distance = BaseDistance;
	ResolveFocus(Focus, Distance);

	// Positive OrbitPitch = camera elevated above the ring looking down.
	// FRotator pitch is opposite (positive looks up), so negate for placement.
	const FRotator OrbitRot(-OrbitPitch, OrbitYaw, 0.f);
	const FVector DesiredLocation = Focus - OrbitRot.Vector() * Distance;
	const FVector SafeLocation(
		DesiredLocation.X,
		DesiredLocation.Y,
		FMath::Max(DesiredLocation.Z, Focus.Z + 80.f));
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), SafeLocation, DeltaSeconds, LocationInterpSpeed);
	SetActorLocation(NewLocation);

	const FRotator DesiredLook = UKismetMathLibrary::FindLookAtRotation(NewLocation, Focus);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredLook, DeltaSeconds, RotationInterpSpeed));
}
