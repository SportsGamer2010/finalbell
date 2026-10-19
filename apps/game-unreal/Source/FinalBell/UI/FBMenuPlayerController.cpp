#include "UI/FBMenuPlayerController.h"
#include "UI/FBMenuCamera.h"
#include "UI/FBMenuWidget.h"
#include "UI/FBPlayerSettings.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"

AFBMenuPlayerController::AFBMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AFBMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	if (UFBPlayerSettingsSave* Settings = UFBPlayerSettingsSave::LoadOrCreate())
	{
		Settings->ApplyAudio(this);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MenuCamera = GetWorld()->SpawnActor<AFBMenuCamera>(AFBMenuCamera::StaticClass(), FVector(0.f, -430.f, 155.f), FRotator(-6.f, 90.f, 0.f), Params);
	if (MenuCamera)
	{
		SetViewTarget(MenuCamera);
	}
	if (APawn* Controlled = GetPawn())
	{
		Controlled->SetActorHiddenInGame(true);
	}

	MenuWidget = CreateWidget<UFBMenuWidget>(this, UFBMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(20);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(MenuWidget->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		MenuWidget->ArmInitialFocus();
	}
}

void AFBMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	Super::EndPlay(EndPlayReason);
}

void AFBMenuPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MenuCamera && GetViewTarget() != MenuCamera)
	{
		SetViewTarget(MenuCamera);
		bShowMouseCursor = true;
	}
	if (!MenuWidget || !IsLocalController())
	{
		return;
	}
	const float Y = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Abs(Y) < 0.55f)
	{
		StickRepeat = 0.f;
		return;
	}
	StickRepeat -= DeltaSeconds;
	if (StickRepeat > 0.f)
	{
		return;
	}
	StickRepeat = 0.28f;
	MenuWidget->Navigate(Y > 0.f ? -1 : 1);
}

bool AFBMenuPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (MenuWidget && Params.Event == IE_Pressed)
	{
		MenuWidget->NoteInputDevice(Params.IsGamepad());
		const FKey& Key = Params.Key;
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			MenuWidget->Confirm();
			return true;
		}
		if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Left || Key == EKeys::Gamepad_Special_Right)
		{
			MenuWidget->Back();
			return true;
		}
		if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
		{
			MenuWidget->Navigate(-1);
			return true;
		}
		if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
		{
			MenuWidget->Navigate(1);
			return true;
		}
		if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
		{
			MenuWidget->Horizontal(-1);
			return true;
		}
		if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
		{
			MenuWidget->Horizontal(1);
			return true;
		}
	}
	if (MenuWidget && Params.Event == IE_Released)
	{
		const FKey& Key = Params.Key;
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::LeftMouseButton)
		{
			MenuWidget->ReleaseConfirm();
		}
	}
	return Super::InputKey(Params);
}
