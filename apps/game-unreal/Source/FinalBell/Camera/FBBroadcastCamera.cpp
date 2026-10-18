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
	Camera->SetFieldOfView(FightFOV);
	Camera->PostProcessBlendWeight = 1.f;
	Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
	Camera->PostProcessSettings.VignetteIntensity = 0.06f;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = 1.15f;
	Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Camera->PostProcessSettings.AutoExposureMinBrightness = 1.f;
	Camera->PostProcessSettings.bOverride_ColorContrast = true;
	Camera->PostProcessSettings.ColorContrast = FVector4(1.02f, 1.02f, 1.02f, 1.f);
	Camera->PostProcessSettings.bOverride_ColorGain = true;
	Camera->PostProcessSettings.ColorGain = FVector4(1.08f, 1.06f, 1.02f, 1.f);
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = 0.15f;
}

void AFBBroadcastCamera::BeginPlay()
{
	Super::BeginPlay();
	if (Camera)
	{
		Camera->SetFieldOfView(FightFOV);
	}
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
	const bool bFirstFrame = !bHasFramed && Player && Opponent;
	PlayerFighter = Player;
	OpponentFighter = Opponent;

	if (bFirstFrame)
	{
		const FVector ToOpp = (Opponent->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		if (!ToOpp.IsNearlyZero())
		{
			// Yaw 180 from the opponent puts the lens behind the controlled fighter.
			OrbitYaw = ToOpp.Rotation().Yaw + 180.f;
		}
		OrbitPitch = 5.f;
		bHasFramed = true;
		SnapToFrame();
		ForcePossessedView();
	}
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

void AFBBroadcastCamera::ComputeFrame(FVector& OutLocation, FVector& OutLookAt) const
{
	const AFBBoxerCharacter* Player = PlayerFighter.Get();
	if (!Player)
	{
		OutLocation = GetActorLocation();
		OutLookAt = OutLocation + GetActorForwardVector() * 200.f;
		return;
	}

	const float PlayerHalf = FMath::Max(Player->GetStandingHalfHeight(), 1.f);
	const FVector PlayerLoc = Player->GetActorLocation();
	const FVector PlayerFeet = PlayerLoc - FVector(0.f, 0.f, PlayerHalf);

	FVector OppLoc = PlayerLoc + Player->GetActorForwardVector().GetSafeNormal2D() * 180.f;
	float OppHalf = PlayerHalf;
	if (const AFBBoxerCharacter* Opponent = OpponentFighter.Get())
	{
		OppLoc = Opponent->GetActorLocation();
		OppHalf = FMath::Max(Opponent->GetStandingHalfHeight(), 1.f);
	}
	const FVector OppFeet = OppLoc - FVector(0.f, 0.f, OppHalf);

	// OrbitYaw auto-aligns to "behind the player", so this vector points away from the opponent.
	const FVector Back = FRotator(0.f, OrbitYaw, 0.f).Vector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, -Back).GetSafeNormal();
	const float LeadSign = (Player->Stance == EFBStance::Southpaw) ? 1.f : -1.f;

	const float Separation = FVector::Dist2D(PlayerLoc, OppLoc);
	const float CloseAlpha = 1.f - FMath::Clamp(Separation / FMath::Max(CloseSeparation, 1.f), 0.f, 1.f);
	const float Distance = FollowDistance + ClosePullback * CloseAlpha;
	const float PitchRad = FMath::DegreesToRadians(OrbitPitch);

	OutLocation = PlayerLoc + Back * Distance + Right * (LeadSign * LeadSideOffset);
	OutLocation.Z = PlayerFeet.Z + (PlayerHalf * 2.f) * ChestHeightFraction + FMath::Sin(PitchRad) * Distance * 0.45f;
	OutLocation.Z = FMath::Max(OutLocation.Z, PlayerFeet.Z + 90.f);

	OutLookAt = FVector(OppLoc.X, OppLoc.Y, OppFeet.Z + (OppHalf * 2.f) * LookAtHeightFraction);
	OutLookAt.Z -= FMath::Sin(PitchRad) * 24.f;
}

void AFBBroadcastCamera::SnapToFrame()
{
	FVector Location;
	FVector LookAt;
	ComputeFrame(Location, LookAt);
	SetActorLocation(Location);
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Location, LookAt));
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

	FVector DesiredLocation;
	FVector LookAt;
	ComputeFrame(DesiredLocation, LookAt);

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaSeconds, LocationInterpSpeed);
	SetActorLocation(NewLocation);

	const FRotator DesiredLook = UKismetMathLibrary::FindLookAtRotation(NewLocation, LookAt);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredLook, DeltaSeconds, RotationInterpSpeed));
}
