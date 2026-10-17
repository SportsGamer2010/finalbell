#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AFBBroadcastCamera::AFBBroadcastCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength = 600.f;
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

void AFBBroadcastCamera::BeginPlay()
{
	Super::BeginPlay();
	AutoFindFighters();

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTargetWithBlend(this, 0.f);
	}
}

void AFBBroadcastCamera::SetFighters(AFBBoxerCharacter* A, AFBBoxerCharacter* B)
{
	FighterA = A;
	FighterB = B;
}

void AFBBroadcastCamera::AutoFindFighters()
{
	TArray<AFBBoxerCharacter*> Found;
	for (TActorIterator<AFBBoxerCharacter> It(GetWorld()); It; ++It)
	{
		Found.Add(*It);
	}
	if (Found.Num() >= 1)
	{
		FighterA = Found[0];
	}
	if (Found.Num() >= 2)
	{
		FighterB = Found[1];
	}
}

void AFBBroadcastCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FighterA.IsValid())
	{
		AutoFindFighters();
	}
	if (!FighterA.IsValid())
	{
		return;
	}

	const FVector LocA = FighterA->GetActorLocation();
	const FVector LocB = FighterB.IsValid() ? FighterB->GetActorLocation() : LocA + FVector(200.f, 0.f, 0.f);
	const FVector Mid = (LocA + LocB) * 0.5f + FVector(0.f, 0.f, HeightOffset);
	const float Separation = FVector::Dist(LocA, LocB);
	SpringArm->TargetArmLength = FMath::Clamp(Separation + DistancePadding, 450.f, 1200.f);

	const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), Mid, DeltaSeconds, InterpSpeed);
	SetActorLocation(NewLoc);

	const FVector LookDir = ((LocA + LocB) * 0.5f - NewLoc).GetSafeNormal();
	if (!LookDir.IsNearlyZero())
	{
		SetActorRotation(LookDir.Rotation());
	}
}
