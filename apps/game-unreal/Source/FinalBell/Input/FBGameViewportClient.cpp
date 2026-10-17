#include "Input/FBGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Widgets/SViewport.h"

namespace
{
	int32 CountPieViewports()
	{
		if (!GEngine)
		{
			return 0;
		}
		int32 Count = 0;
		for (const FWorldContext& WC : GEngine->GetWorldContexts())
		{
			if (WC.WorldType == EWorldType::PIE && WC.GameViewport)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 ResolvePadUser(const FInputKeyEventArgs& EventArgs)
	{
		int32 PadUser = EventArgs.ControllerId;
		const FPlatformUserId UserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(EventArgs.InputDevice);
		if (UserId.IsValid())
		{
			PadUser = UserId.GetInternalId();
		}
		return PadUser;
	}
}

ULocalPlayer* UFBGameViewportClient::SetupInitialLocalPlayer(FString& OutError)
{
	ULocalPlayer* LocalPlayer = Super::SetupInitialLocalPlayer(OutError);
	BindPieLocalPlayerToGamepad();
	return LocalPlayer;
}

void UFBGameViewportClient::BindPieLocalPlayerToGamepad()
{
#if WITH_EDITOR
	if (!GEngine)
	{
		return;
	}

	const FWorldContext* Ctx = GEngine->GetWorldContextFromGameViewport(this);
	if (!Ctx || Ctx->WorldType != EWorldType::PIE || Ctx->PIEInstance < 0)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
	{
		LocalPlayer->SetControllerId(Ctx->PIEInstance);
		bPieBindingApplied = true;
		UE_LOG(LogTemp, Warning,
			TEXT("FinalBell: PIE instance %d bound to controller/user %d"),
			Ctx->PIEInstance, Ctx->PIEInstance);
	}

	PieFocusRetrySeconds = 2.f;
	FocusRetryAccum = 0.f;
	EnsurePieGamepadFocus();
#endif
}

void UFBGameViewportClient::EnsurePieGamepadFocus()
{
#if WITH_EDITOR
	if (!GEngine || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const FWorldContext* Ctx = GEngine->GetWorldContextFromGameViewport(this);
	if (!Ctx || Ctx->WorldType != EWorldType::PIE || Ctx->PIEInstance < 0)
	{
		return;
	}

	const TSharedPtr<SViewport> Widget = GetGameViewportWidget();
	if (!Widget.IsValid())
	{
		return;
	}

	FSlateApplication::Get().SetUserFocus(
		static_cast<uint32>(Ctx->PIEInstance),
		Widget,
		EFocusCause::SetDirectly);
#endif
}

void UFBGameViewportClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
	if (PieFocusRetrySeconds > 0.f)
	{
		PieFocusRetrySeconds -= DeltaTime;
		FocusRetryAccum += DeltaTime;
		if (FocusRetryAccum >= 0.25f || PieFocusRetrySeconds <= 0.f)
		{
			FocusRetryAccum = 0.f;
			EnsurePieGamepadFocus();
		}
	}
#endif
}

void UFBGameViewportClient::RemapControllerInput(FInputKeyEventArgs& InOutEventArgs)
{
#if WITH_EDITOR
	if (InOutEventArgs.IsGamepad() && GEngine)
	{
		if (const FWorldContext* Ctx = GEngine->GetWorldContextFromGameViewport(this))
		{
			if (Ctx->WorldType == EWorldType::PIE && Ctx->PIEInstance >= 0)
			{
				const int32 PieCount = CountPieViewports();
				const int32 PadUser = ResolvePadUser(InOutEventArgs);

				// Solo PIE: any pad drives the one local player (so pad 2 isn't dead).
				if (PieCount <= 1)
				{
					if (ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
					{
						InOutEventArgs.ControllerId = LP->GetControllerId();
						const FInputDeviceId Primary =
							IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(LP->GetPlatformUserId());
						if (Primary.IsValid())
						{
							InOutEventArgs.InputDevice = Primary;
						}
					}
					return;
				}

				// Multi PIE: pad N stays on PIE instance N.
				if (PadUser == Ctx->PIEInstance)
				{
					return;
				}
			}
		}
	}
#endif
	Super::RemapControllerInput(InOutEventArgs);
}

bool UFBGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
#if WITH_EDITOR
	if (EventArgs.IsGamepad() && RouteGamepadToMatchingPieViewport(EventArgs, false))
	{
		return true;
	}
#endif
	return Super::InputKey(EventArgs);
}

bool UFBGameViewportClient::InputAxis(const FInputKeyEventArgs& EventArgs)
{
#if WITH_EDITOR
	if (EventArgs.IsGamepad() && RouteGamepadToMatchingPieViewport(EventArgs, true))
	{
		return true;
	}
#endif
	return Super::InputAxis(EventArgs);
}

bool UFBGameViewportClient::RouteGamepadToMatchingPieViewport(const FInputKeyEventArgs& EventArgs, bool bAxis)
{
	if (!GEngine || !EventArgs.Viewport || !EventArgs.Viewport->IsPlayInEditorViewport())
	{
		return false;
	}

	UWorld* PieWorld = GetWorld();
	if (!PieWorld)
	{
		return false;
	}

	const FWorldContext* OwnCtx = GEngine->GetWorldContextFromWorld(PieWorld);
	if (!OwnCtx || OwnCtx->WorldType != EWorldType::PIE)
	{
		return false;
	}

	if (CountPieViewports() < 2)
	{
		return false;
	}

	const int32 DesiredPie = ResolvePadUser(EventArgs);
	if (DesiredPie == OwnCtx->PIEInstance)
	{
		return false;
	}

	for (const FWorldContext& WC : GEngine->GetWorldContexts())
	{
		if (WC.WorldType == EWorldType::PIE && WC.PIEInstance == DesiredPie && WC.GameViewport && WC.GameViewport != this)
		{
			if (bAxis)
			{
				WC.GameViewport->InputAxis(EventArgs);
			}
			else
			{
				WC.GameViewport->InputKey(EventArgs);
			}
			return true;
		}
	}

	return false;
}
