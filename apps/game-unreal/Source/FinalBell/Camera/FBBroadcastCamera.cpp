#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Training/FBHeavyBag.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

namespace
{
	/** 20 ft rope line. The lens stays outside this, plus RopeClearanceCm. */
	constexpr float RopeHalfCm = 305.f;
	constexpr float RopeClearanceCm = 120.f;
	/** Gym brick is centered at 1400 cm. This keeps the lens off that wall. The dark bowl is farther out. */
	constexpr float RoomHalfCm = 1300.f;
	/** Extra width past each fighter so a glove and an elbow stay inside the frame. */
	constexpr float GloveMarginCm = 85.f;
	constexpr float HeadPadCm = 32.f;
	constexpr float FeetPadCm = 20.f;
	/** Added to the clinch distance so the close shot is not glued to the crop line. */
	constexpr float ClosePadCm = 64.f;
	constexpr float FitPadCm = 40.f;
	/** Keep the lens this far from a fighter's center so it does not sit inside a glove. */
	constexpr float FighterClearCm = 170.f;
}

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
	SmoothedFOV = FightFOV;
	SmoothedSeparation = ZoomFarSeparation;
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
	else if (Player && !Opponent && !bTrainingFramed)
	{
		for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
		{
			if (!IsValid(*It) || !It->IsActiveBag())
			{
				continue;
			}
			const FVector BagLine = (It->GetStrikeCenter() - Player->GetActorLocation()).GetSafeNormal2D();
			if (!BagLine.IsNearlyZero())
			{
				FVector Side(-BagLine.Y, BagLine.X, 0.f);
				if (FVector::DotProduct(Side, FVector(0.f, -1.f, 0.f)) < 0.f)
				{
					Side *= -1.f;
				}
				OrbitYaw = Side.Rotation().Yaw;
				OrbitPitch = 6.f;
			}
			bTrainingFramed = true;
			bHasFramed = true;
			SnapToFrame();
			break;
		}
	}
}

void AFBBroadcastCamera::AddOrbitInput(FVector2D Axis)
{
	PendingOrbitInput += Axis;
}

bool AFBBroadcastCamera::IsLiveBoxer(const AFBBoxerCharacter* Boxer) const
{
	if (!IsValid(Boxer) || Boxer->IsActorBeingDestroyed() || Boxer->IsHidden())
	{
		return false;
	}
	// A benched PIE pawn is unpossessed and then destroyed. It is not the tagged opponent.
	if (Boxer->IsPlayerControlled() || Boxer->GetController() != nullptr)
	{
		return true;
	}
	if (Boxer->ActorHasTag(TEXT("FB_P1Opponent")))
	{
		return true;
	}
	// The other human's controller does not exist on this client. Their player state does.
	return Boxer->GetPlayerState() != nullptr && GetNetMode() == NM_Client;
}

bool AFBBroadcastCamera::ResolveLivePair(AFBBoxerCharacter*& OutPlayer, AFBBoxerCharacter*& OutOpponent) const
{
	OutPlayer = nullptr;
	OutOpponent = nullptr;
	if (!GetWorld())
	{
		return false;
	}

	AFBBoxerCharacter* StoredPlayer = IsLiveBoxer(PlayerFighter.Get()) ? PlayerFighter.Get() : nullptr;
	AFBBoxerCharacter* StoredOpponent = IsLiveBoxer(OpponentFighter.Get()) ? OpponentFighter.Get() : nullptr;
	AFBBoxerCharacter* Tagged = nullptr;
	AFBBoxerCharacter* Fallback = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		AFBBoxerCharacter* Boxer = *It;
		if (!IsLiveBoxer(Boxer))
		{
			continue;
		}
		if (Boxer->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			Tagged = Boxer;
		}
		else if (!Fallback)
		{
			Fallback = Boxer;
		}
	}

	OutPlayer = StoredPlayer ? StoredPlayer : Fallback;
	if (StoredOpponent && StoredOpponent != OutPlayer)
	{
		OutOpponent = StoredOpponent;
	}
	else if (Tagged && Tagged != OutPlayer)
	{
		OutOpponent = Tagged;
	}
	else if (Fallback && Fallback != OutPlayer)
	{
		OutOpponent = Fallback;
	}
	if (OutPlayer && !OutOpponent)
	{
		for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
		{
			if (IsLiveBoxer(*It) && *It != OutPlayer)
			{
				OutOpponent = *It;
				break;
			}
		}
	}
	return OutPlayer && OutOpponent && OutPlayer != OutOpponent;
}

float AFBBroadcastCamera::ResolveViewAspect() const
{
	float Aspect = (Camera && Camera->AspectRatio > 0.1f) ? Camera->AspectRatio : (16.f / 9.f);
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size(0.f, 0.f);
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.Y > 1.f)
		{
			Aspect = Size.X / Size.Y;
		}
	}
	return FMath::Clamp(Aspect, 1.f, 3.f);
}

void AFBBroadcastCamera::UpdateSmoothedSeparation(float DeltaSeconds)
{
	AFBBoxerCharacter* A = nullptr;
	AFBBoxerCharacter* B = nullptr;
	if (!ResolveLivePair(A, B))
	{
		return;
	}
	const float Raw = FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation());
	if (!bZoomInitialized)
	{
		SmoothedSeparation = Raw;
		bZoomInitialized = true;
		return;
	}
	SmoothedSeparation = FMath::FInterpTo(SmoothedSeparation, Raw, DeltaSeconds, ZoomInterpSpeed);
}

void AFBBroadcastCamera::AutoFindFighters()
{
	AFBBoxerCharacter* Local = nullptr;
	AFBBoxerCharacter* Other = nullptr;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		Local = Cast<AFBBoxerCharacter>(PC->GetPawn());
		if (!IsLiveBoxer(Local))
		{
			Local = nullptr;
		}
	}

	if (GetWorld())
	{
		for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
		{
			AFBBoxerCharacter* Boxer = *It;
			if (!IsLiveBoxer(Boxer) || Boxer == Local)
			{
				continue;
			}
			if (Boxer->ActorHasTag(TEXT("FB_P1Opponent")))
			{
				Other = Boxer;
				break;
			}
			if (!Other)
			{
				Other = Boxer;
			}
		}
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

void AFBBroadcastCamera::ComputeFrame(FVector& OutLocation, FVector& OutLookAt, float& OutFOV) const
{
	OutFOV = FightFOV;

	AFBBoxerCharacter* LivePlayer = nullptr;
	AFBBoxerCharacter* LiveOpponent = nullptr;
	const bool bPair = ResolveLivePair(LivePlayer, LiveOpponent);
	const AFBBoxerCharacter* Player = bPair ? LivePlayer : (IsLiveBoxer(PlayerFighter.Get()) ? PlayerFighter.Get() : nullptr);
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
	if (!bPair)
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
	if (bPair && LiveOpponent)
	{
		OppLoc = LiveOpponent->GetActorLocation();
		OppHalf = FMath::Max(LiveOpponent->GetStandingHalfHeight(), 1.f);
	}
	else if (TrainingBag)
	{
		OppLoc = TrainingBag->GetStrikeCenter();
		OppHalf = 0.f;
	}
	const FVector OppFeet = OppLoc - FVector(0.f, 0.f, OppHalf);
	const float FeetZ = FMath::Min(PlayerFeet.Z, OppFeet.Z);
	const FVector Mid = (PlayerLoc + OppLoc) * 0.5f;

	FVector Line = (OppLoc - PlayerLoc).GetSafeNormal2D();
	if (Line.IsNearlyZero())
	{
		Line = Player->GetActorForwardVector().GetSafeNormal2D();
	}
	FVector Side = FRotator(0.f, OrbitYaw, 0.f).Vector().GetSafeNormal2D();
	if (Side.IsNearlyZero())
	{
		Side = FVector(0.f, -1.f, 0.f);
	}

	if (TrainingBag)
	{
		// Side-rear view of one fighter and one bag. The look point is the chest,
		// so the head and the shoes stay in the shot when the fighter steps.
		const FFBBagRoomLayout Room = UFBP1ArenaBootstrap::GetBagRoomLayout();
		const float BodyHeight = PlayerHalf * 2.f;
		const FVector Chest = PlayerFeet + FVector(0.f, 0.f, PlayerHalf);
		const FVector Back = -Line;
		FVector Cam = PlayerLoc + Side * Room.CameraSide + Back * Room.CameraBack;
		Cam.Z = PlayerFeet.Z + Room.CameraHeight + FMath::Max(0.f, OrbitPitch) * 3.f;
		Cam.X = FMath::Clamp(Cam.X, -Room.InnerHalfX + 70.f, Room.InnerHalfX - 70.f);
		Cam.Y = FMath::Clamp(Cam.Y, Room.SouthInnerY + 70.f, Room.NorthInnerY - 90.f);

		FVector Look = Chest;
		Look.X = FMath::Lerp(Chest.X, OppLoc.X, 0.12f);
		Look.Y = FMath::Lerp(Chest.Y, OppLoc.Y, 0.12f);

		const float Dist = FMath::Max(FVector::Dist(Cam, Look), 80.f);
		const float Aspect = FMath::Max(ResolveViewAspect(), 1.f);
		const float HalfTan = FMath::Tan(FMath::DegreesToRadians(FMath::Max(FightFOV, 45.f) * 0.5f));
		const float VerticalNeed = (BodyHeight * 0.5f + 48.f) / Dist;
		const float BagNeed = (FVector::Dist2D(Look, OppLoc) + AFBHeavyBag::BodyDiameterCm() + 40.f) / Dist;
		const float NeededTan = FMath::Max(VerticalNeed * Aspect, FMath::Max(BagNeed, HalfTan));
		OutFOV = FMath::Clamp(2.f * FMath::RadiansToDegrees(FMath::Atan(NeededTan)), FightFOV, 74.f);
		OutLocation = Cam;
		OutLookAt = Look;
		return;
	}

	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(FMath::Max(FightFOV, 45.f) * 0.5f));
	const float WideRadius = (RingHalfExtent / FMath::Max(HalfTan, 0.25f)) + RingHalfExtent * 0.35f + OutsideMargin;
	const float Along = FMath::Clamp(FVector::DotProduct(Mid, Line), -DollyLimit, DollyLimit);
	const FVector Dolly = Line * (Along * DollyFollow);

	if (!bPair)
	{
		// No second live boxer. Stay on the wide ringside offset instead of zooming in.
		OutLocation = Side * WideRadius + Dolly;
		OutLocation.Z = FeetZ + HeightAboveFeet + FMath::Max(0.f, OrbitPitch) * 6.f;
		const FVector RingCenter(0.f, 0.f, FeetZ);
		OutLookAt = FVector(
			FMath::Lerp(RingCenter.X, Mid.X, 0.65f),
			FMath::Lerp(RingCenter.Y, Mid.Y, 0.65f),
			FeetZ + (PlayerHalf * 2.f) * LookHeightFraction);
		return;
	}

	const float Aspect = ResolveViewAspect();
	const float VertTan = HalfTan / FMath::Max(Aspect, 1.f);
	const float BodyHeight = FMath::Max(PlayerHalf, OppHalf) * 2.f;
	const float HeadAbove = BodyHeight * (1.f - LookHeightFraction) + HeadPadCm;
	const float FeetBelow = BodyHeight * LookHeightFraction + FeetPadCm;
	const float VerticalExtent = FMath::Max(HeadAbove, FeetBelow);
	const float CloseRadius = VerticalExtent / FMath::Max(VertTan, 0.05f) + ClosePadCm;

	const float Separation = FMath::Max(SmoothedSeparation, ZoomNearSeparation);
	const float Span = FMath::Max(ZoomFarSeparation - ZoomNearSeparation, 1.f);
	const float Alpha = FMath::Clamp((Separation - ZoomNearSeparation) / Span, 0.f, 1.f);
	const float HalfSpan = Separation * 0.5f + GloveMarginCm;
	const float FitDist = FMath::Max(
		HalfSpan / FMath::Max(HalfTan, 0.05f),
		VerticalExtent / FMath::Max(VertTan, 0.05f)) + FitPadCm;

	const float SideAbs = FMath::Max(FMath::Max(FMath::Abs(Side.X), FMath::Abs(Side.Y)), 0.15f);
	const float MinRadius = (RopeHalfCm + RopeClearanceCm) / SideAbs;
	const float MaxRadius = RoomHalfCm / SideAbs;

	float Radius = FMath::Lerp(CloseRadius, WideRadius, Alpha);
	const FVector MidXY(Mid.X, Mid.Y, 0.f);
	for (int32 Step = 0; Step < 4; ++Step)
	{
		const float Dist = FVector::Dist2D(Side * Radius + Dolly, MidXY);
		if (Dist >= FitDist)
		{
			break;
		}
		Radius += FitDist - Dist;
	}
	const float UnclampedRadius = Radius;
	Radius = FMath::Clamp(Radius, MinRadius, MaxRadius);

	OutLocation = Side * Radius + Dolly;
	OutLocation.Z = FeetZ + HeightAboveFeet + FMath::Max(0.f, OrbitPitch) * 6.f;

	const FVector Fighters[2] = { PlayerLoc, OppLoc };
	for (const FVector& Fighter : Fighters)
	{
		const FVector Flat(OutLocation.X - Fighter.X, OutLocation.Y - Fighter.Y, 0.f);
		const float Dist = Flat.Size();
		if (Dist < FighterClearCm && Dist > 1.f)
		{
			const FVector Shift = Flat / Dist * (FighterClearCm - Dist);
			OutLocation.X += Shift.X;
			OutLocation.Y += Shift.Y;
		}
	}
	if (FMath::Abs(OutLocation.X) < RopeHalfCm + RopeClearanceCm
		&& FMath::Abs(OutLocation.Y) < RopeHalfCm + RopeClearanceCm)
	{
		const FVector Pushed = Side * MinRadius;
		OutLocation.X = Pushed.X;
		OutLocation.Y = Pushed.Y;
	}
	OutLocation.X = FMath::Clamp(OutLocation.X, -RoomHalfCm, RoomHalfCm);
	OutLocation.Y = FMath::Clamp(OutLocation.Y, -RoomHalfCm, RoomHalfCm);

	const FVector RingCenter(0.f, 0.f, FeetZ);
	OutLookAt = FVector(
		FMath::Lerp(RingCenter.X, Mid.X, 0.65f),
		FMath::Lerp(RingCenter.Y, Mid.Y, 0.65f),
		FeetZ + BodyHeight * LookHeightFraction);

	const float ActualDist = FMath::Max(FVector::Dist2D(OutLocation, MidXY), 50.f);
	const float NeededTan = FMath::Max(HalfSpan / ActualDist, (VerticalExtent * Aspect) / ActualDist);
	const float NeededFOV = 2.f * FMath::RadiansToDegrees(FMath::Atan(NeededTan));
	if (NeededFOV > FightFOV + 0.25f)
	{
		OutFOV = FMath::Min(NeededFOV, FightFOV + ZoomFOVWiden);
	}
	else if (UnclampedRadius < MinRadius - 1.f && Alpha < 0.5f)
	{
		OutFOV = FMath::Clamp(NeededFOV, FightFOV - ZoomFOVTighten, FightFOV);
	}
}

void AFBBroadcastCamera::SnapToFrame()
{
	AFBBoxerCharacter* A = nullptr;
	AFBBoxerCharacter* B = nullptr;
	if (ResolveLivePair(A, B))
	{
		SmoothedSeparation = FVector::Dist2D(A->GetActorLocation(), B->GetActorLocation());
		bZoomInitialized = true;
	}
	float FOV = FightFOV;
	FVector Location;
	FVector LookAt;
	ComputeFrame(Location, LookAt, FOV);
	SmoothedFOV = FOV;
	if (Camera && !bRefereeView)
	{
		Camera->SetFieldOfView(SmoothedFOV);
	}
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
		Camera->SetFieldOfView(bZoomInitialized ? SmoothedFOV : FightFOV);
	}
}

void AFBBroadcastCamera::SetCreatorPreview(bool bEnabled, AFBBoxerCharacter* Subject)
{
	bCreatorPreview = bEnabled;
	if (Subject)
	{
		CreatorSubject = Subject;
		PlayerFighter = Subject;
	}
	if (bEnabled)
	{
		ForcePossessedView();
		return;
	}
	CreatorSubject = nullptr;
	if (Camera)
	{
		Camera->SetFieldOfView(FightFOV);
	}
	SnapToFrame();
}

void AFBBroadcastCamera::UpdateCreatorOrbit(float Yaw, float Pitch, float Zoom)
{
	CreatorYaw = Yaw;
	CreatorPitch = FMath::Clamp(Pitch, -6.f, 28.f);
	CreatorZoom = FMath::Clamp(Zoom, 0.f, 1.f);
}

void AFBBroadcastCamera::UpdateCreatorFrame(float DeltaSeconds)
{
	AFBBoxerCharacter* Subject = CreatorSubject.Get();
	if (!IsLiveBoxer(Subject))
	{
		Subject = IsLiveBoxer(PlayerFighter.Get()) ? PlayerFighter.Get() : nullptr;
	}
	if (!Subject)
	{
		return;
	}
	const float Half = FMath::Max(Subject->GetStandingHalfHeight(), 90.f);
	const FVector Chest = Subject->GetActorLocation() + FVector(0.f, 0.f, Half * 0.05f);
	const FRotator Orbit(CreatorPitch, CreatorYaw, 0.f);
	const float Dist = FMath::Lerp(190.f, 520.f, CreatorZoom);
	const FVector Right = FRotationMatrix(Orbit).GetUnitAxis(EAxis::Y);
	const FVector Look = Chest + Right * Dist * 0.28f - FVector(0.f, 0.f, Half * 0.12f);
	const FVector Eye = Chest - Orbit.Vector() * Dist + FVector(0.f, 0.f, Dist * 0.02f);
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), Eye, DeltaSeconds, 8.f);
	SetActorLocation(NewLocation);
	const FRotator Desired = UKismetMathLibrary::FindLookAtRotation(NewLocation, Look);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, 8.f));
	if (Camera)
	{
		const float FOV = FMath::Lerp(38.f, 52.f, CreatorZoom);
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, FOV, DeltaSeconds, 8.f));
	}
}

void AFBBroadcastCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCreatorPreview)
	{
		UpdateCreatorFrame(DeltaSeconds);
		return;
	}

	// Keep the fight gap current during a knockdown, but do not move this shot.
	UpdateSmoothedSeparation(DeltaSeconds);

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
	AFBBoxerCharacter* LiveA = nullptr;
	AFBBoxerCharacter* LiveB = nullptr;
	const bool bLivePair = ResolveLivePair(LiveA, LiveB);
	if (!PlayerFighter.IsValid() && !bLivePair)
	{
		return;
	}

	if (!bLivePair && PlayerFighter.IsValid())
	{
		for (TActorIterator<AFBHeavyBag> It(GetWorld()); It; ++It)
		{
			if (!IsValid(*It) || !It->IsActiveBag())
			{
				continue;
			}
			const float Dist = FVector::Dist2D(PlayerFighter->GetActorLocation(), It->GetStrikeCenter());
			if (bTrainingFramed && Dist < 1500.f)
			{
				break;
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
				OrbitPitch = 6.f;
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

	if (bAutoAlignWhenIdle && OrbitInputHoldTime <= 0.f && bLivePair)
	{
		const FVector Line = (LiveB->GetActorLocation() - LiveA->GetActorLocation()).GetSafeNormal2D();
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
	float DesiredFOV = FightFOV;
	ComputeFrame(DesiredLocation, LookAt, DesiredFOV);
	if (!bZoomInitialized)
	{
		SmoothedFOV = DesiredFOV;
	}
	else
	{
		SmoothedFOV = FMath::FInterpTo(SmoothedFOV, DesiredFOV, DeltaSeconds, ZoomInterpSpeed);
	}
	if (Camera)
	{
		Camera->SetFieldOfView(SmoothedFOV);
	}

	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaSeconds, LocationInterpSpeed);
	SetActorLocation(NewLocation);

	const FRotator DesiredLook = UKismetMathLibrary::FindLookAtRotation(NewLocation, LookAt);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredLook, DeltaSeconds, RotationInterpSpeed));
}
