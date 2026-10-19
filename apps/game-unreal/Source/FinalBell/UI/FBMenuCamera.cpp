#include "UI/FBMenuCamera.h"
#include "UI/FBMenuDeveloperSettings.h"
#include "UI/FBPlayerSettings.h"
#include "Camera/CameraComponent.h"

AFBMenuCamera::AFBMenuCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(48.f);
	SetActorLocation(Pivot);
	SetActorRotation(FRotator(-6.f, 90.f, 0.f));
}

void AFBMenuCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UFBMenuDeveloperSettings* Style = UFBMenuDeveloperSettings::Get();
	const UFBPlayerSettingsSave* Settings = UFBPlayerSettingsSave::LoadOrCreate();
	const bool bReduced = Settings && Settings->bReducedMotion;
	Elapsed += DeltaSeconds;
	float Yaw = 90.f;
	if (!bReduced && Style && Style->MenuCameraDriftDegrees > 0.f)
	{
		Yaw += FMath::Sin(Elapsed * 0.15f) * Style->MenuCameraDriftDegrees;
	}
	SetActorLocation(Pivot);
	SetActorRotation(FRotator(-6.f, Yaw, 0.f));
}
