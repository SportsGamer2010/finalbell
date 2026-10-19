#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Training/FBHeavyBag.h"
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
	const bool bFirstFrame = Player && Opponent && (!bHasFramed || bTrainingFramed);
	PlayerFighter = Player;
	OpponentFighter = Opponent;
	if (Opponent)
	{
		bTrainingFramed = false;
	}

	if (bFirstFrame)
	{
		const FVector Line = (Opponent->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		if (!Line.IsNearlyZero())
		{
			// Profile side of the fight line, preferring the south apron.
			FVector Side(-Line.Y, Line.X, 0.f);
			if (FVector::DotProduct(Side, FVector(0.f, -1.f, 0.f)) < 0.f)
			{
				Side *= -1.f;
			}
			OrbitYaw = Side.Rotation().Yaw;
		}
		OrbitPitch = 8.f;
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

	const AFBHeavyBag* TrainingBag = nullptr;
	if (!OpponentFighter.IsValid())
	{
		for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
		{
			if (IsValid(*It) && It->IsActiveBag())
			{
				TrainingBag = *It;
				break;
			}
		}
	}

	FVector OppLoc = PlayerLoc + Player->GetActorForwardVector().GetSafeNormal2D() * 180.f;
	float OppHalf = PlayerHalf;
	if (const AFBBoxerCharacter* Opponent = OpponentFighter.Get())
	{
		OppLoc = Opponent->GetActorLocation();
		OppHalf = FMath::Max(Opponent->GetStandingHalfHeight(), 1.f);
	}
	else if (TrainingBag)
	{
		OppLoc = TrainingBag->GetStrikeCenter();
	}
	const FVector OppFeet = OppLoc - FVector(0.f, 0.f, OppHalf);
	const float FeetZ = FMath::Min(PlayerFeet.Z, OppFeet.Z);
	const FVector Mid = (PlayerLoc + OppLoc) * 0.5f;

	FVector Line = (OppLoc - PlayerLoc).GetSafeNormal2D();
	if (Line.IsNearlyZero())
	{
		Line = Player->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Side = FRotator(0.f, OrbitYaw, 0.f).Vector().GetSafeNormal2D();

	if (TrainingBag)
	{
		// Close enough that a 46 cm bag is a bag. The ringside distance frames the whole ring.
		const float FrameDistance = 340.f;
		OutLocation = Mid + Side * FrameDistance;
		OutLocation.Z = FeetZ + 155.f + FMath::Max(0.f, OrbitPitch) * 4.f;
		OutLookAt = FVector(Mid.X, Mid.Y, FeetZ + 110.f);
		return;
	}

	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(FMath::Max(FightFOV, 45.f) * 0.5f));
	const float Radius = (RingHalfExtent / FMath::Max(HalfTan, 0.25f)) + RingHalfExtent * 0.35f + OutsideMargin;
	const float Along = FMath::Clamp(FVector::DotProduct(Mid, Line), -DollyLimit, DollyLimit);

	OutLocation = Side * Radius + Line * (Along * DollyFollow);
	OutLocation.Z = FeetZ + HeightAboveFeet + FMath::Max(0.f, OrbitPitch) * 6.f;

	const FVector RingCenter(0.f, 0.f, FeetZ);
	OutLookAt = FVector(
		FMath::Lerp(RingCenter.X, Mid.X, 0.65f),
		FMath::Lerp(RingCenter.Y, Mid.Y, 0.65f),
		FeetZ + (PlayerHalf * 2.f) * LookHeightFraction);
}

void AFBBroadcastCamera::SnapToFrame()
{
	FVector Location;
	FVector LookAt;
	ComputeFrame(Location, LookAt);
	SetActorLocation(Location);
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Location, LookAt));
}

void AFBBroadcastCamera::SetRefereeView(FVector EyeLocation, FVector LookAtLocation)
{
	bRefereeView = true;
	RefereeEye = EyeLocation;
	RefereeLook = LookAtLocation;
	if (Camera)
	{
		Camera->SetFieldOfView(42.f);
	}
}

void AFBBroadcastCamera::ClearRefereeView()
{
	bRefereeView = false;
	if (Camera)
	{
		Camera->SetFieldOfView(FightFOV);
	}
}

void AFBBroadcastCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRefereeView)
	{
		const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), RefereeEye, DeltaSeconds, 6.f);
		SetActorLocation(NewLocation);
		const FRotator DesiredLook = UKismetMathLibrary::FindLookAtRotation(NewLocation, RefereeLook);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredLook, DeltaSeconds, 8.f));
		return;
	}

	if (!PlayerFighter.IsValid())
	{
		AutoFindFighters();
	}
	if (!PlayerFighter.IsValid())
	{
		return;
	}

	if (!OpponentFighter.IsValid() && !bTrainingFramed)
	{
		for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
		{
			if (!IsValid(*It) || !It->IsActiveBag())
			{
				continue;
			}
			const FVector Line = (It->GetStrikeCenter() - PlayerFighter->GetActorLocation()).GetSafeNormal2D();
			if (!Line.IsNearlyZero())
			{
				FVector Side(-Line.Y, Line.X, 0.f);
				if (FVector::DotProduct(Side, FVector(0.f, -1.f, 0.f)) < 0.f)
				{
					Side *= -1.f;
				}
				OrbitYaw = Side.Rotation().Yaw;
				OrbitPitch = 10.f;
			}
			bTrainingFramed = true;
			bHasFramed = true;
			SnapToFrame();
			break;
		}
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
		const FVector Line = (OpponentFighter->GetActorLocation() - PlayerFighter->GetActorLocation()).GetSafeNormal2D();
		if (!Line.IsNearlyZero())
		{
			FVector DesiredSide(-Line.Y, Line.X, 0.f);
			const FVector CurrentSide = FRotator(0.f, OrbitYaw, 0.f).Vector();
			if (FVector::DotProduct(CurrentSide, DesiredSide) < 0.f)
			{
				DesiredSide *= -1.f;
			}
			const float DesiredYaw = DesiredSide.Rotation().Yaw;
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(OrbitYaw, DesiredYaw);
			const float MaxStep = AutoAlignSpeed * 40.f * DeltaSeconds;
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
