#include "Combat/FBSparringPlayerController.h"
#include "Camera/FBBroadcastCamera.h"
#include "Combat/FBBoxerCustomization.h"
#include "Ring/FBP1ArenaBootstrap.h"
#include "Combat/FBBoxerCharacter.h"
#include "GameFramework/SpectatorPawn.h"
#include "Combat/FBFighterSave.h"
#include "Combat/FBSparringGameMode.h"
#include "Network/FBPrivateRoomSubsystem.h"
#include "Session/FBSessionSubsystem.h"
#include "Training/FBHeavyBag.h"
#include "UI/FBFightMeters.h"
#include "UI/FBBoutOverlay.h"
#include "Combat/FBSparringGameState.h"
#include "UI/FBGameInstance.h"
#include "UI/FBMenuArt.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "GameInputDeveloperSettings.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Input/FBGameViewportClient.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SViewport.h"
#include "Widgets/SBoxPanel.h"
#include "Input/Reply.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/GameplayStatics.h"

class SFBMenuBridge : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFBMenuBridge) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, AFBSparringPlayerController* InOwner)
	{
		Owner = InOwner;
		SetCanTick(true);
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		if (AFBSparringPlayerController* Controller = Owner.Get())
		{
			Controller->PollMenuStick(InDeltaTime);
		}
	}

	virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override
	{
		if (AFBSparringPlayerController* Controller = Owner.Get())
		{
			if (Controller->RouteMenuKey(KeyEvent.GetKey(), KeyEvent.IsRepeat()))
			{
				return FReply::Handled();
			}
		}
		return SCompoundWidget::OnPreviewKeyDown(Geometry, KeyEvent);
	}

	TWeakObjectPtr<AFBSparringPlayerController> Owner;
};

AFBSparringPlayerController::AFBSparringPlayerController()
{
	bReplicates = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bShowMouseCursor = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AFBSparringPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (GEngine && GetWorld())
	{
		if (const FWorldContext* Ctx = GEngine->GetWorldContextFromWorld(GetWorld()))
		{
			if (Ctx->WorldType == EWorldType::PIE && Ctx->PIEInstance >= 0)
			{
				if (ULocalPlayer* LP = GetLocalPlayer())
				{
					LP->SetControllerId(Ctx->PIEInstance);
				}
				if (UFBGameViewportClient* VC = Cast<UFBGameViewportClient>(GetWorld()->GetGameViewport()))
				{
					VC->BindPieLocalPlayerToGamepad();
				}
			}
		}
	}
#endif

	if (IsLocalController())
	{
		if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
		{
			if (FrontEnd->IsAwaitingFrontEnd())
			{
				bShowMouseCursor = false;
				return;
			}
			if (FrontEnd->ConsumeOpenFighterSelect())
			{
				ChooseMode(FrontEnd->ConsumeCareerFighterSelect() ? EFBPlayMode::Career : EFBPlayMode::VersusAI);
				return;
			}
		}
		bool bAlreadyChosen = false;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
			{
				bAlreadyChosen = Session->HasChosenPlayMode();
			}
		}
		// Host / first local player picks mode. PIE client 2 skips menu.
		ULocalPlayer* LP = GetLocalPlayer();
		const bool bIsFirstLocal = LP && GetGameInstance() && LP == GetGameInstance()->GetFirstGamePlayer();
		const bool bShow = !bAlreadyChosen && bIsFirstLocal;
		if (bShow)
		{
			ShowModeMenu();
		}
		else
		{
			bShowMouseCursor = false;
		}
	}
}

void AFBSparringPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetPause(false);
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->SetPresentationPaused(false);
	}
	HidePauseMenu();
	HideModeMenu();
	HideFighterSelect();
	RemoveFightMeters();
	RemoveBoutOverlay();
	Super::EndPlay(EndPlayReason);
}

void AFBSparringPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateFightMeters();
	SubmitLocalGetUpAim();
	UpdateBoutOverlay(DeltaTime);
	SyncFightMappingToFocus();
	if (IsLocalController() && bFightMappingWanted && !IsMenuTakingInput())
	{
		if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
		{
			if (!Boxer->IsFightMappingApplied())
			{
				Boxer->SetFightMappingActive(true);
			}
		}
	}
	if (bFighterSelectVisible && bBuilderOpen)
	{
		if (bCreatorIdleSpin && !bCreatorDragging)
		{
			CreatorYaw += 14.f * DeltaTime;
		}
		EnsureCreatorCamera();
	}
	else if (bCreatorCameraOn)
	{
		ShutdownCreatorCamera();
	}
	KeepBroadcastView();
	EnsureLocalBagRoom();
}

void AFBSparringPlayerController::EnsureLocalBagRoom()
{
	if (!IsLocalController() || !GetWorld())
	{
		return;
	}
	UGameInstance* GI = GetGameInstance();
	UFBSessionSubsystem* Session = GI ? GI->GetSubsystem<UFBSessionSubsystem>() : nullptr;
	const bool bTraining = Session && Session->GetPlayMode() == EFBPlayMode::Training;
	if (!bTraining)
	{
		if (bBagRoomLookBuilt)
		{
			UFBP1ArenaBootstrap::DestroyBagRoom(GetWorld());
			bBagRoomLookBuilt = false;
		}
		return;
	}
	if (bBagRoomLookBuilt)
	{
		return;
	}
	bBagRoomLookBuilt = true;
	AFBHeavyBag* Bag = nullptr;
	UFBP1ArenaBootstrap::EnsureBagRoom(GetWorld(), Bag);
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
	{
		Boxer->RefreshFightCamera();
	}
}

UGameViewportClient* AFBSparringPlayerController::LocalViewport() const
{
	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		if (LP->ViewportClient)
		{
			return LP->ViewportClient;
		}
	}
	if (const UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			return Viewport;
		}
	}
	return GEngine ? GEngine->GameViewport : nullptr;
}

void AFBSparringPlayerController::BroadcastSoloPresentation()
{
	if (!GetWorld())
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFBSparringPlayerController* Other = Cast<AFBSparringPlayerController>(It->Get());
		if (Other && Other != this)
		{
			Other->ClientPresentSoloBout();
		}
	}
}

void AFBSparringPlayerController::ClientPresentSoloBout_Implementation()
{
	PresentSoloRing();
}

void AFBSparringPlayerController::PresentSoloRing()
{
	bSoloPresentation = true;
	HideFighterSelect();
	if (bModeMenuVisible)
	{
		HideModeMenu();
	}
	bShowMouseCursor = false;
	RestoreFightMapping();
	if (APawn* Spec = GetSpectatorPawn())
	{
		Spec->SetActorHiddenInGame(true);
		Spec->SetActorEnableCollision(false);
	}
	if (!bSoloLookBuilt)
	{
		bSoloLookBuilt = true;
		if (UWorld* World = GetWorld())
		{
			AFBBoxerCharacter* Human = Cast<AFBBoxerCharacter>(GetPawn());
			AFBBoxerCharacter* Opponent = nullptr;
			for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
			{
				if (!*It || *It == Human)
				{
					continue;
				}
				if (It->ActorHasTag(TEXT("FB_P1Opponent")))
				{
					Opponent = *It;
				}
				else if (!Human)
				{
					Human = *It;
				}
			}
			UFBP1ArenaBootstrap::EnsureMatchLook(World, true, Human, Opponent);
		}
	}
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
	{
		Boxer->RefreshFightCamera();
	}
	KeepBroadcastView();
}

void AFBSparringPlayerController::AddCreatorOrbit(float YawDelta, float PitchDelta)
{
	CreatorYaw += YawDelta;
	CreatorPitch = FMath::Clamp(CreatorPitch + PitchDelta, -6.f, 28.f);
}

void AFBSparringPlayerController::AddCreatorZoom(float ZoomDelta)
{
	CreatorZoom = FMath::Clamp(CreatorZoom + ZoomDelta, 0.f, 1.f);
}

void AFBSparringPlayerController::SetCreatorDrag(bool bDragging)
{
	bCreatorDragging = bDragging;
}

void AFBSparringPlayerController::SetCreatorMeasure(TSharedPtr<STextBlock> Measure)
{
	CreatorMeasure = Measure;
}

FString AFBSparringPlayerController::GetCreatorMeasureText() const
{
	const EFBWeightClassId ClassId = FFBFighterBalance::ClassForDivision(Draft.Division);
	return FString::Printf(
		TEXT("%s\n%.0f cm     reach %.0f cm\nThe capsule and punch spheres stay on this class."),
		*FFBFighterBalance::ClassName(ClassId),
		FFBBoxerCustomization::DisplayHeightCm(Draft.Look, ClassId),
		FFBBoxerCustomization::DisplayReachCm(Draft.Look, ClassId));
}

void AFBSparringPlayerController::EnsureCreatorCamera()
{
	if (!IsLocalController() || !GetWorld())
	{
		return;
	}
	AFBBoxerCharacter* Human = Cast<AFBBoxerCharacter>(GetPawn());
	AFBBroadcastCamera* Shared = nullptr;
	for (TActorIterator<AFBBroadcastCamera> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		if (!It->GetOwner() || It->GetOwner() == GetPawn())
		{
			Shared = *It;
			if (!It->GetOwner())
			{
				break;
			}
		}
	}
	if (!Shared || !Human)
	{
		return;
	}
	if (!bCreatorCameraOn)
	{
		CreatorYaw = Human->GetActorRotation().Yaw + 180.f;
		CreatorPitch = 6.f;
		CreatorZoom = 0.62f;
		bCreatorCameraOn = true;
		Shared->SetCreatorPreview(true, Human);
	}
	Shared->UpdateCreatorOrbit(CreatorYaw, CreatorPitch, CreatorZoom);
}

void AFBSparringPlayerController::ShutdownCreatorCamera()
{
	bCreatorCameraOn = false;
	bCreatorDragging = false;
	if (!GetWorld())
	{
		return;
	}
	for (TActorIterator<AFBBroadcastCamera> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It))
		{
			It->SetCreatorPreview(false, nullptr);
		}
	}
}

void AFBSparringPlayerController::KeepBroadcastView()
{
	if (!bSoloPresentation || !IsLocalController() || bFighterSelectVisible || bModeMenuVisible || bPauseOpen)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (APawn* Spec = GetSpectatorPawn())
	{
		Spec->SetActorHiddenInGame(true);
		Spec->SetActorEnableCollision(false);
	}
	AFBBoxerCharacter* Human = Cast<AFBBoxerCharacter>(GetPawn());
	AFBBoxerCharacter* Opponent = nullptr;
	for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
	{
		if (!*It || *It == Human)
		{
			continue;
		}
		if (It->ActorHasTag(TEXT("FB_P1Opponent")))
		{
			Opponent = *It;
		}
		else if (!Human)
		{
			Human = *It;
		}
	}
	AFBBroadcastCamera* Shared = nullptr;
	for (TActorIterator<AFBBroadcastCamera> It(World); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		if (!It->GetOwner() || It->GetOwner() == GetPawn())
		{
			Shared = *It;
			if (!It->GetOwner())
			{
				break;
			}
		}
	}
	if (!Shared)
	{
		return;
	}
	if (Human || Opponent)
	{
		Shared->SetFighters(Human, Opponent);
	}
	AActor* View = GetViewTarget();
	const bool bViewIsCamera = IsValid(View) && View->IsA(AFBBroadcastCamera::StaticClass());
	if (!bViewIsCamera)
	{
		Shared->ForceViewForController(this);
	}
}

void AFBSparringPlayerController::SubmitLocalGetUpAim()
{
	if (!IsLocalController())
	{
		return;
	}
	const AFBSparringGameState* BoutState = GetWorld() ? GetWorld()->GetGameState<AFBSparringGameState>() : nullptr;
	const AFBBoxerCharacter* Local = Cast<AFBBoxerCharacter>(GetPawn());
	if (!BoutState || !Local || !BoutState->bKnockdown || BoutState->DownedBoxer != Local)
	{
		return;
	}

	const FVector2D Left = Local->GetCachedLeftStick();
	const FVector2D Right = Local->GetCachedRightStick();
	if (HasAuthority())
	{
		if (AFBSparringGameMode* Mode = GetWorld()->GetAuthGameMode<AFBSparringGameMode>())
		{
			Mode->SubmitGetUpAim(this, Left, Right);
		}
		return;
	}
	ServerSubmitGetUpAim(Left, Right);
}

void AFBSparringPlayerController::ServerSubmitGetUpAim_Implementation(FVector2D LeftStick, FVector2D RightStick)
{
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->SubmitGetUpAim(this, LeftStick, RightStick);
	}
}

void AFBSparringPlayerController::RemoveBoutOverlay()
{
	if (BoutOverlay.IsValid() && GEngine && LocalViewport())
	{
		LocalViewport()->RemoveViewportWidgetContent(BoutOverlay.ToSharedRef());
	}
	BoutOverlay.Reset();
}

void AFBSparringPlayerController::UpdateBoutOverlay(float DeltaTime)
{
	if (!IsLocalController() || bModeMenuVisible || bFighterSelectVisible || bPauseOpen)
	{
		RemoveBoutOverlay();
		return;
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		if (FrontEnd->IsAwaitingFrontEnd())
		{
			RemoveBoutOverlay();
			return;
		}
	}
	if (!GEngine || !LocalViewport())
	{
		return;
	}

	const AFBSparringGameState* BoutState = GetWorld() ? GetWorld()->GetGameState<AFBSparringGameState>() : nullptr;
	const AFBBoxerCharacter* Local = Cast<AFBBoxerCharacter>(GetPawn());
	const bool bDownedLocal = BoutState && Local && BoutState->DownedBoxer == Local;
	const FVector2D Left = bDownedLocal ? Local->GetCachedLeftStick() : FVector2D::ZeroVector;
	const FVector2D Right = bDownedLocal ? Local->GetCachedRightStick() : FVector2D::ZeroVector;
	const bool bClinch = Local && Local->IsInClinch();
	const float ClinchMeter = Local ? Local->GetClinchTurnMeter() : 0.f;
	const int32 ClinchTurns = Local ? Local->GetClinchTurnsLanded() : 0;

	if (!BoutOverlay.IsValid())
	{
		BoutOverlay = SNew(SFBBoutOverlay);
		LocalViewport()->AddViewportWidgetContent(BoutOverlay.ToSharedRef(), 30);
	}
	if (TSharedPtr<SFBBoutOverlay> Overlay = StaticCastSharedPtr<SFBBoutOverlay>(BoutOverlay))
	{
		Overlay->Refresh(DeltaTime, BoutState, Left, Right, bClinch, ClinchMeter, ClinchTurns);
	}
}

void AFBSparringPlayerController::RemoveFightMeters()
{
	if (FightMeters.IsValid() && GEngine && LocalViewport())
	{
		LocalViewport()->RemoveViewportWidgetContent(FightMeters.ToSharedRef());
	}
	FightMeters.Reset();
}

void AFBSparringPlayerController::UpdateFightMeters()
{
	if (!IsLocalController() || bModeMenuVisible || bFighterSelectVisible || bPauseOpen)
	{
		RemoveFightMeters();
		return;
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		if (FrontEnd->IsAwaitingFrontEnd())
		{
			RemoveFightMeters();
			return;
		}
	}
	if (!GEngine || !LocalViewport())
	{
		return;
	}

	const AFBBoxerCharacter* Local = Cast<AFBBoxerCharacter>(GetPawn());
	if (!Local)
	{
		return;
	}

	if (!FightMeters.IsValid())
	{
		FightMeters = SNew(SFBFightMeters);
		LocalViewport()->AddViewportWidgetContent(FightMeters.ToSharedRef(), 20);
	}

	const AFBBoxerCharacter* Other = nullptr;
	FString OtherLabel = TEXT("OPPONENT");
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFBBoxerCharacter> It(World); It; ++It)
		{
			if (*It && *It != Local)
			{
				Other = *It;
				if (!It->GetFighterDisplayName().IsEmpty())
				{
					OtherLabel = It->GetFighterDisplayName();
				}
				if (It->ActorHasTag(TEXT("FB_P1Opponent")))
				{
					break;
				}
			}
		}
		if (!Other)
		{
			for (TActorIterator<AFBHeavyBag> It(World); It; ++It)
			{
				OtherLabel = TEXT("BAG");
				break;
			}
		}
	}

	if (TSharedPtr<SFBFightMeters> Meters = StaticCastSharedPtr<SFBFightMeters>(FightMeters))
	{
		Meters->Refresh(Local, Other, OtherLabel);
	}
}

void AFBSparringPlayerController::ShowModeMenu()
{
	if (bModeMenuVisible || !GEngine || !LocalViewport())
	{
		return;
	}

	bShowMouseCursor = true;
	ModeRows.Reset();
	ModeFocus = FMath::Clamp(ModeFocus, 0, 3);

	const FLinearColor Paper = FBMenuArt::Paper();
	const FLinearColor Accent = FBMenuArt::Accent();
	const FLinearColor Quiet = FBMenuArt::Quiet();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

	static FButtonStyle ClearButton = []()
	{
		FButtonStyle Style;
		const FSlateColorBrush Clear(FLinearColor::Transparent);
		Style.SetNormal(Clear);
		Style.SetHovered(Clear);
		Style.SetPressed(Clear);
		Style.SetDisabled(Clear);
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);
		return Style;
	}();

	const struct FChoice
	{
		const TCHAR* Label;
		EFBPlayMode Mode;
	} Choices[] = {
		{ TEXT("1V1 SPARRING (AI)"), EFBPlayMode::VersusAI },
		{ TEXT("1V1 MULTIPLAYER"), EFBPlayMode::Multiplayer },
		{ TEXT("HEAVY BAG TRAINING"), EFBPlayMode::Training },
		{ TEXT("1V1 CAREER"), EFBPlayMode::Career }
	};

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Choices); ++Index)
	{
		FFBRowChrome Row;
		SAssignNew(Row.Fill, SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(FMargin(0.f))
			[
				SNew(SBox)
				.HeightOverride(52.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.f, 10.f, 14.f, 10.f)
					[
						SNew(SBox)
						.WidthOverride(4.f)
						[
							SAssignNew(Row.Tick, SBorder)
							.BorderImage(White)
							.BorderBackgroundColor(FLinearColor::Transparent)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f, 0.f, 18.f, 0.f)
					[
						SAssignNew(Row.Label, STextBlock)
						.Text(FText::FromString(Choices[Index].Label))
						.Font(FBMenuArt::PosterFont(22, 140))
						.ColorAndOpacity(Quiet)
					]
				]
			];
		const EFBPlayMode Mode = Choices[Index].Mode;
		List->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SButton)
			.ButtonStyle(&ClearButton)
			.OnHovered_Lambda([this, Index]()
			{
				ModeFocus = Index;
				PaintChrome(ModeRows, ModeFocus);
			})
			.OnClicked_Lambda([this, Mode]()
			{
				ChooseMode(Mode);
				return FReply::Handled();
			})
			[
				Row.Fill.ToSharedRef()
			]
		];
		ModeRows.Add(Row);
	}
	PaintChrome(ModeRows, ModeFocus);

	ModeMenuWidget = WrapForPad(SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		.Alignment(FVector2D(0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor(0.012f, 0.014f, 0.018f, 1.f))
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		.Alignment(FVector2D(0.f, 0.f))
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage)
				.Image(FBMenuArt::GetArenaBrush())
			]
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.06f, 0.11f, 0.46f, 0.9f))
		.Offset(FMargin(0.f))
		.Alignment(FVector2D(0.f, 0.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("FINAL")))
					.Font(FBMenuArt::PosterFont(84, 180))
					.ColorAndOpacity(Paper)
					.ShadowOffset(FVector2D(0.f, 3.f))
					.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(18.f, 8.f, 0.f, 0.f))
				[
					SNew(SBox)
					.WidthOverride(92.f)
					.HeightOverride(10.f)
					[
						SNew(SBorder)
						.BorderImage(White)
						.BorderBackgroundColor(Accent)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 34.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("BELL")))
				.Font(FBMenuArt::PosterFont(84, 180))
				.ColorAndOpacity(Paper)
				.ShadowOffset(FVector2D(0.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				List
			]
		]);

	LocalViewport()->AddViewportWidgetContent(ModeMenuWidget.ToSharedRef(), 100);
	bModeMenuVisible = true;
	FocusMenuWidget(ModeMenuWidget);
}

void AFBSparringPlayerController::HideModeMenu()
{
	if (bModeMenuVisible && ModeMenuWidget.IsValid() && GEngine && LocalViewport())
	{
		LocalViewport()->RemoveViewportWidgetContent(ModeMenuWidget.ToSharedRef());
	}
	ModeMenuWidget.Reset();
	ModeRows.Reset();
	bModeMenuVisible = false;
	bShowMouseCursor = false;
	if (!IsMenuTakingInput())
	{
		RestoreFightMapping();
	}
}

void AFBSparringPlayerController::ChooseMode(EFBPlayMode Mode)
{
	bSoloPresentation = false;
	bSoloLookBuilt = false;
	if (Mode == EFBPlayMode::VersusAI || Mode == EFBPlayMode::Career)
	{
		bCareerSelect = Mode == EFBPlayMode::Career;
		// Do not store the mode yet. A stored mode makes the arena timer call StartBout
		// before Fight, and StartBout then refuses to run a second time.
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
			{
				Session->ClearPlayMode();
			}
			if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GI))
			{
				FrontEnd->ClearPendingBout();
			}
		}
		if (AFBSparringGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
		{
			GM->SetAwaitingFighterConfirm(true);
		}
		if (bModeMenuVisible && ModeMenuWidget.IsValid() && GEngine && LocalViewport())
		{
			LocalViewport()->RemoveViewportWidgetContent(ModeMenuWidget.ToSharedRef());
		}
		ModeMenuWidget.Reset();
		ModeRows.Reset();
		bModeMenuVisible = false;
		ShowFighterSelect();
		return;
	}
	bCareerSelect = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ChoosePlayMode(Mode);
		}
	}
	if (HasAuthority())
	{
		if (AFBSparringGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
		{
			GM->SetAwaitingFighterConfirm(false);
			GM->ApplyPlayMode(Mode);
		}
	}
	HideModeMenu();
}

void AFBSparringPlayerController::FBMode(int32 ModeIndex)
{
	EFBPlayMode Mode = EFBPlayMode::Unset;
	switch (ModeIndex)
	{
	case 1: Mode = EFBPlayMode::VersusAI; break;
	case 2: Mode = EFBPlayMode::Multiplayer; break;
	case 3: Mode = EFBPlayMode::Training; break;
	case 4: Mode = EFBPlayMode::Career; break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("FBMode usage: 1=Sparring 2=Multiplayer 3=Training 4=Career"));
		return;
	}
	ChooseMode(Mode);
}

void AFBSparringPlayerController::FBHost(int32 Port)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBPrivateRoomSubsystem* Rooms = GI->GetSubsystem<UFBPrivateRoomSubsystem>())
		{
			Rooms->HostListenServer(Port > 0 ? Port : 7777);
			return;
		}
	}
}

void AFBSparringPlayerController::FBJoin(const FString& HostAddress)
{
	if (HostAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("FinalBell FBJoin usage: FBJoin <ip>[:port]"));
		return;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBPrivateRoomSubsystem* Rooms = GI->GetSubsystem<UFBPrivateRoomSubsystem>())
		{
			Rooms->JoinByAddress(HostAddress, 7777);
		}
	}
}

void AFBSparringPlayerController::FBRoom()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBPrivateRoomSubsystem* Rooms = GI->GetSubsystem<UFBPrivateRoomSubsystem>())
		{
			const FFBPrivateRoomInfo Info = Rooms->CreatePrivateRoom();
			UE_LOG(LogTemp, Warning, TEXT("FinalBell room %s — partner: FBJoin %s"), *Info.RoomCode, *Info.HostAddress);
		}
	}
}

bool AFBSparringPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if ((Params.Event == IE_Pressed || Params.Event == IE_Repeat) && RouteMenuKey(Params.Key, Params.Event == IE_Repeat))
	{
		return true;
	}
	return Super::InputKey(Params);
}

TSharedRef<SWidget> AFBSparringPlayerController::WrapForPad(TSharedRef<SWidget> Content)
{
	return SNew(SFBMenuBridge, this)
		[
			Content
		];
}

bool AFBSparringPlayerController::IsMenuTakingInput() const
{
	return bPauseOpen || bModeMenuVisible || bFighterSelectVisible;
}

bool AFBSparringPlayerController::WantsFightMapping() const
{
	return bFightMappingWanted && !IsMenuTakingInput();
}

bool AFBSparringPlayerController::ViewportHasFocus() const
{
	// FSceneViewport::IsForegroundWindow() asks the viewport HWND if it is the
	// foreground window. In the editor that HWND is not, even while this
	// controller is receiving the pad. That reading removed the fight mapping
	// at the bell and never put it back. Slate focus is the signal instead.
	if (!FSlateApplication::IsInitialized())
	{
		return true;
	}
	const UGameViewportClient* ViewportClient = LocalViewport();
	const TSharedPtr<SViewport> ViewportWidget = ViewportClient ? ViewportClient->GetGameViewportWidget() : nullptr;
	if (!ViewportWidget.IsValid())
	{
		return true;
	}

	int32 UserIndex = 0;
	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		const int32 Mapped = IPlatformInputDeviceMapper::Get().GetUserIndexForPlatformUser(LP->GetPlatformUserId());
		if (Mapped >= 0)
		{
			UserIndex = Mapped;
		}
	}
	const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetUserFocusedWidget(UserIndex);
	if (!Focused.IsValid())
	{
		return true;
	}

	const TSharedPtr<SWidget> ViewportParent = ViewportWidget->GetParentWidget();
	for (TSharedPtr<SWidget> Walk = Focused; Walk.IsValid(); Walk = Walk->GetParentWidget())
	{
		if (Walk.Get() == ViewportWidget.Get() || Walk == ViewportParent)
		{
			return true;
		}
	}
	return !Focused->GetParentWidget().IsValid();
}

void AFBSparringPlayerController::SuspendFightMapping()
{
	bFightMappingWanted = false;
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
	{
		Boxer->SetFightMappingActive(false);
	}
}

void AFBSparringPlayerController::RestoreFightMapping()
{
	if (!IsLocalController() || IsMenuTakingInput())
	{
		return;
	}
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (FSlateApplication::IsInitialized())
	{
		if (const UGameViewportClient* ViewportClient = LocalViewport())
		{
			if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
			{
				int32 UserIndex = 0;
				if (const ULocalPlayer* LP = GetLocalPlayer())
				{
					const int32 Mapped = IPlatformInputDeviceMapper::Get().GetUserIndexForPlatformUser(LP->GetPlatformUserId());
					if (Mapped >= 0)
					{
						UserIndex = Mapped;
					}
				}
				FSlateApplication::Get().SetUserFocus(UserIndex, ViewportWidget.ToSharedRef(), EFocusCause::SetDirectly);
			}
		}
	}
	bFightMappingWanted = true;
	bSawViewportFocus = true;
	if (AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(GetPawn()))
	{
		Boxer->SetFightMappingActive(true);
	}
}

void AFBSparringPlayerController::RefocusOpenMenu()
{
	if (bPauseOpen && PauseWidget.IsValid())
	{
		FocusMenuWidget(PauseWidget);
	}
	else if (bFighterSelectVisible && FighterSelectWidget.IsValid())
	{
		FocusMenuWidget(FighterSelectWidget);
	}
	else if (bModeMenuVisible && ModeMenuWidget.IsValid())
	{
		FocusMenuWidget(ModeMenuWidget);
	}
}

void AFBSparringPlayerController::SyncFightMappingToFocus()
{
	if (!IsLocalController())
	{
		return;
	}
	const bool bFocused = ViewportHasFocus();
	if (bFocused == bSawViewportFocus)
	{
		return;
	}
	bSawViewportFocus = bFocused;
	if (!bFocused)
	{
		SuspendFightMapping();
		return;
	}
	if (IsMenuTakingInput())
	{
		RefocusOpenMenu();
		return;
	}
	RestoreFightMapping();
}

void AFBSparringPlayerController::FocusMenuWidget(const TSharedPtr<SWidget>& Widget)
{
	SuspendFightMapping();
	if (!Widget.IsValid())
	{
		return;
	}
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(Widget);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	if (FSlateApplication::IsInitialized())
	{
		int32 UserIndex = 0;
		if (const ULocalPlayer* LP = GetLocalPlayer())
		{
			const int32 Mapped = IPlatformInputDeviceMapper::Get().GetUserIndexForPlatformUser(LP->GetPlatformUserId());
			if (Mapped >= 0)
			{
				UserIndex = Mapped;
			}
		}
		FSlateApplication::Get().SetUserFocus(UserIndex, Widget.ToSharedRef(), EFocusCause::SetDirectly);
	}
}

void AFBSparringPlayerController::PaintChrome(TArray<FFBRowChrome>& Rows, int32 Selected) const
{
	const FLinearColor Paper = FBMenuArt::Paper();
	const FLinearColor Accent = FBMenuArt::Accent();
	const FLinearColor Quiet = FBMenuArt::Quiet();
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const bool bOn = Index == Selected;
		if (Rows[Index].Fill.IsValid())
		{
			Rows[Index].Fill->SetBorderBackgroundColor(bOn ? Accent : FLinearColor::Transparent);
		}
		if (Rows[Index].Tick.IsValid())
		{
			Rows[Index].Tick->SetBorderBackgroundColor(bOn ? Paper : FLinearColor::Transparent);
		}
		if (Rows[Index].Label.IsValid())
		{
			Rows[Index].Label->SetColorAndOpacity(bOn ? Paper : Quiet);
		}
	}
}

void AFBSparringPlayerController::PollMenuStick(float DeltaTime)
{
	if (!bPauseOpen && !bModeMenuVisible && !bFighterSelectVisible)
	{
		MenuStickRepeat = 0.f;
		return;
	}
	const float StickY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	const float StickX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float Axis = FMath::Abs(StickY) >= FMath::Abs(StickX) ? StickY : -StickX;
	if (FMath::Abs(Axis) < 0.35f)
	{
		MenuStickRepeat = 0.f;
		return;
	}
	MenuStickRepeat -= DeltaTime;
	if (MenuStickRepeat > 0.f)
	{
		return;
	}
	MenuStickRepeat = 0.28f;
	NudgeMenu(Axis > 0.f ? -1 : 1);
}

void AFBSparringPlayerController::NudgeMenu(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	if (bPauseOpen && PauseRows.Num() > 0)
	{
		PauseFocus = (PauseFocus + Delta + PauseRows.Num()) % PauseRows.Num();
		PaintChrome(PauseRows, PauseFocus);
		return;
	}
	if (bFighterSelectVisible && SelectRows.Num() > 0)
	{
		SelectFocus = (SelectFocus + Delta + SelectRows.Num()) % SelectRows.Num();
		PaintChrome(SelectRows, SelectFocus);
		return;
	}
	if (bModeMenuVisible && ModeRows.Num() > 0)
	{
		ModeFocus = (ModeFocus + Delta + ModeRows.Num()) % ModeRows.Num();
		PaintChrome(ModeRows, ModeFocus);
	}
}

void AFBSparringPlayerController::ActivateMenu()
{
	if (bPauseOpen)
	{
		if (bPauseConfirmQuit)
		{
			if (PauseFocus == 0)
			{
				FPlatformMisc::RequestExit(false);
			}
			else
			{
				bPauseConfirmQuit = false;
				PauseFocus = 3;
				ShowPauseMenu();
			}
			return;
		}
		if (bPauseForResult)
		{
			switch (PauseFocus)
			{
			case 0: RequestRematch(); break;
			case 1: ReturnToMainMenu(); break;
			default:
				bPauseConfirmQuit = true;
				PauseFocus = 0;
				ShowPauseMenu();
				break;
			}
			return;
		}
		switch (PauseFocus)
		{
		case 0: ResumeFromPause(); break;
		case 1: ReturnToMainMenu(); break;
		case 2: ChangeModeFromPause(); break;
		default:
			bPauseConfirmQuit = true;
			PauseFocus = 0;
			ShowPauseMenu();
			break;
		}
		return;
	}
	if (bFighterSelectVisible)
	{
		ActivateSelectRow();
		return;
	}
	if (bModeMenuVisible)
	{
		const EFBPlayMode Modes[] = { EFBPlayMode::VersusAI, EFBPlayMode::Multiplayer, EFBPlayMode::Training, EFBPlayMode::Career };
		ChooseMode(Modes[FMath::Clamp(ModeFocus, 0, UE_ARRAY_COUNT(Modes) - 1)]);
	}
}

void AFBSparringPlayerController::BackMenu(bool bFromOptions)
{
	if (bPauseOpen)
	{
		if (bPauseConfirmQuit && !bFromOptions)
		{
			bPauseConfirmQuit = false;
			PauseFocus = 0;
			ShowPauseMenu();
			return;
		}
		ResumeFromPause();
		return;
	}
	if (bFighterSelectVisible)
	{
		if (bBuilderOpen)
		{
			bBuilderOpen = false;
			SelectStatus.Reset();
			SelectFocus = 0;
			RebuildFighterSelect();
			return;
		}
		HideFighterSelect();
		ShowModeMenu();
		return;
	}
	if (bModeMenuVisible && !bFromOptions)
	{
		ReturnToMainMenu();
	}
}

bool AFBSparringPlayerController::RouteMenuKey(const FKey& Key, bool bRepeat)
{
	const uint64 Frame = GFrameCounter;
	if (bHandledMenuKeyThisFrame && LastMenuFrame == Frame)
	{
		return true;
	}
	if (bReturningToMenu)
	{
		return false;
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		if (FrontEnd->IsAwaitingFrontEnd())
		{
			return false;
		}
	}

	const bool bMenu = bPauseOpen || bModeMenuVisible || bFighterSelectVisible;
	const bool bAccept = Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom;
	const bool bBack = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
	const bool bOptions = Key == EKeys::Gamepad_Special_Right;
	const bool bUp = Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up;
	const bool bDown = Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down;
	const bool bPrev = Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left;
	const bool bNext = Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right;

	auto Mark = [this, Frame]()
	{
		bHandledMenuKeyThisFrame = true;
		LastMenuFrame = Frame;
	};

	if (!bMenu)
	{
		if (bRepeat || !(bOptions || Key == EKeys::Escape))
		{
			return false;
		}
		ShowPauseMenu();
		Mark();
		return true;
	}
	if (bUp || bPrev)
	{
		NudgeMenu(-1);
		Mark();
		return true;
	}
	if (bDown || bNext)
	{
		NudgeMenu(1);
		Mark();
		return true;
	}
	if (bRepeat)
	{
		Mark();
		return true;
	}
	if (bAccept)
	{
		ActivateMenu();
		Mark();
		return true;
	}
	if (bBack || bOptions)
	{
		BackMenu(bOptions);
		Mark();
		return true;
	}
	return false;
}

void AFBSparringPlayerController::ShowPauseMenu()
{
	if (bReturningToMenu || !IsLocalController() || !GEngine || !LocalViewport())
	{
		return;
	}
	if (PauseWidget.IsValid())
	{
		LocalViewport()->RemoveViewportWidgetContent(PauseWidget.ToSharedRef());
		PauseWidget.Reset();
	}

	if (!HasAuthority() && PauseStatus.IsEmpty())
	{
		PauseStatus = TEXT("The bout keeps going on the host.");
	}

	const FLinearColor Paper = FBMenuArt::Paper();
	const FLinearColor Accent = FBMenuArt::Accent();
	const FLinearColor Quiet = FBMenuArt::Quiet();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	static FButtonStyle ClearButton = []()
	{
		FButtonStyle Style;
		const FSlateColorBrush Clear(FLinearColor::Transparent);
		Style.SetNormal(Clear);
		Style.SetHovered(Clear);
		Style.SetPressed(Clear);
		Style.SetDisabled(Clear);
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);
		return Style;
	}();

	bool bCardUp = false;
	if (const AFBSparringGameState* BoutState = GetWorld() ? GetWorld()->GetGameState<AFBSparringGameState>() : nullptr)
	{
		bCardUp = BoutState->bCardVisible;
	}
	bPauseForResult = bCardUp && !bPauseConfirmQuit;
	const TCHAR* FightLabels[] = { TEXT("RESUME"), TEXT("MAIN MENU"), TEXT("CHANGE MODE"), TEXT("QUIT") };
	const TCHAR* ResultLabels[] = { TEXT("REMATCH"), TEXT("MAIN MENU"), TEXT("QUIT") };
	const TCHAR* ConfirmLabels[] = { TEXT("QUIT"), TEXT("STAY") };
	const TCHAR* const* Shown = bPauseConfirmQuit ? ConfirmLabels : (bPauseForResult ? ResultLabels : FightLabels);
	const int32 Count = bPauseConfirmQuit ? 2 : (bPauseForResult ? 3 : 4);
	PauseFocus = FMath::Clamp(PauseFocus, 0, Count - 1);
	PauseRows.Reset();

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FFBRowChrome Row;
		SAssignNew(Row.Fill, SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(0.f)
			[
				SNew(SBox).HeightOverride(52.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.f, 10.f, 14.f, 10.f)
					[
						SNew(SBox).WidthOverride(4.f)
						[
							SAssignNew(Row.Tick, SBorder)
							.BorderImage(White)
							.BorderBackgroundColor(FLinearColor::Transparent)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f, 0.f, 18.f, 0.f)
					[
						SAssignNew(Row.Label, STextBlock)
						.Text(FText::FromString(Shown[Index]))
						.Font(FBMenuArt::PosterFont(22, 140))
						.ColorAndOpacity(Quiet)
					]
				]
			];
		List->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SButton)
			.ButtonStyle(&ClearButton)
			.OnHovered_Lambda([this, Index]()
			{
				PauseFocus = Index;
				PaintChrome(PauseRows, PauseFocus);
			})
			.OnClicked_Lambda([this, Index]()
			{
				PauseFocus = Index;
				ActivateMenu();
				return FReply::Handled();
			})
			[
				Row.Fill.ToSharedRef()
			]
		];
		PauseRows.Add(Row);
	}
	PaintChrome(PauseRows, PauseFocus);

	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(bPauseConfirmQuit ? TEXT("QUIT GAME") : TEXT("PAUSED")))
			.Font(FBMenuArt::PosterFont(42, 80))
			.ColorAndOpacity(Paper)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			List
		];
	if (!PauseStatus.IsEmpty())
	{
		Panel->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(PauseStatus))
			.Font(FBMenuArt::PosterFont(14, 20))
			.ColorAndOpacity(Accent)
			.AutoWrapText(true)
		];
	}

	PauseWidget = WrapForPad(SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
		.Offset(FMargin(0.f))
		[
			SNew(SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(FLinearColor(0.012f, 0.014f, 0.018f, 0.72f))
		]
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.08f, 0.22f, 0.46f, 0.84f))
		.Offset(FMargin(0.f))
		[
			Panel
		]);

	LocalViewport()->AddViewportWidgetContent(PauseWidget.ToSharedRef(), 120);
	bPauseOpen = true;
	FocusMenuWidget(PauseWidget);
	if (HasAuthority())
	{
		if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
		{
			Mode->SetPresentationPaused(true);
		}
		SetPause(true);
	}
}

void AFBSparringPlayerController::HidePauseMenu()
{
	if (PauseWidget.IsValid() && GEngine && LocalViewport())
	{
		LocalViewport()->RemoveViewportWidgetContent(PauseWidget.ToSharedRef());
	}
	PauseWidget.Reset();
	PauseRows.Reset();
	bPauseOpen = false;
	bPauseConfirmQuit = false;
	bPauseForResult = false;
}

void AFBSparringPlayerController::RequestRematch()
{
	if (HasAuthority())
	{
		if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
		{
			Mode->RequestRematch();
		}
	}
	else
	{
		ServerRequestRematch();
	}
	ResumeFromPause();
}

void AFBSparringPlayerController::ServerRequestRematch_Implementation()
{
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->RequestRematch();
	}
}

void AFBSparringPlayerController::ResumeFromPause()
{
	PauseStatus.Reset();
	HidePauseMenu();
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->SetPresentationPaused(false);
	}
	SetPause(false);
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	bShowMouseCursor = false;
	RestoreFightMapping();
}

void AFBSparringPlayerController::ReturnToMainMenu()
{
	if (bReturningToMenu)
	{
		return;
	}
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->SetPresentationPaused(false);
		if (HasAuthority())
		{
			Mode->AbandonBoutForMenu();
		}
	}
	SetPause(false);
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	HidePauseMenu();
	HideFighterSelect();
	bSoloPresentation = false;
	bSoloLookBuilt = false;
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		FrontEnd->ClearPendingBout();
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ClearPlayMode();
			Session->SetReturnToMenuAllowed(false);
		}
	}
	bReturningToMenu = true;
	UFBGameInstance::TravelToFrontEnd(this);
}

void AFBSparringPlayerController::ChangeModeFromPause()
{
	if (!HasAuthority())
	{
		PauseStatus = TEXT("Only the host can change the mode.");
		ShowPauseMenu();
		return;
	}
	PauseStatus.Reset();
	if (AFBSparringGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AFBSparringGameMode>() : nullptr)
	{
		Mode->SetPresentationPaused(false);
		Mode->AbandonBoutForMenu();
	}
	SetPause(false);
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	HidePauseMenu();
	bSoloPresentation = false;
	bSoloLookBuilt = false;
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GetGameInstance()))
	{
		FrontEnd->ClearPendingBout();
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFBSessionSubsystem* Session = GI->GetSubsystem<UFBSessionSubsystem>())
		{
			Session->ClearPlayMode();
		}
	}
	ModeFocus = 0;
	ShowModeMenu();
}

TSharedRef<SWidget> AFBSparringPlayerController::MakeFocusRow(const FString& Label, TFunction<void()> Action)
{
	const int32 Index = SelectRows.Num();
	const FLinearColor Quiet = FBMenuArt::Quiet();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	static FButtonStyle ClearButton = []()
	{
		FButtonStyle Style;
		const FSlateColorBrush Clear(FLinearColor::Transparent);
		Style.SetNormal(Clear);
		Style.SetHovered(Clear);
		Style.SetPressed(Clear);
		Style.SetDisabled(Clear);
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);
		return Style;
	}();

	FFBRowChrome Row;
	SAssignNew(Row.Fill, SBorder)
		.BorderImage(White)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.Padding(0.f)
		[
			SNew(SBox).HeightOverride(46.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.f, 8.f, 12.f, 8.f)
				[
					SNew(SBox).WidthOverride(4.f)
					[
						SAssignNew(Row.Tick, SBorder)
						.BorderImage(White)
						.BorderBackgroundColor(FLinearColor::Transparent)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SAssignNew(Row.Label, STextBlock)
					.Text(FText::FromString(Label))
					.Font(FBMenuArt::PosterFont(16, 50))
					.ColorAndOpacity(Quiet)
				]
			]
		];
	SelectRows.Add(Row);
	SelectActions.Add(Action);

	return SNew(SButton)
		.ButtonStyle(&ClearButton)
		.OnHovered_Lambda([this, Index]()
		{
			SelectFocus = Index;
			PaintChrome(SelectRows, SelectFocus);
		})
		.OnClicked_Lambda([this, Index, Action]()
		{
			SelectFocus = Index;
			if (Action)
			{
				Action();
			}
			return FReply::Handled();
		})
		[
			Row.Fill.ToSharedRef()
		];
}

void AFBSparringPlayerController::ActivateSelectRow()
{
	if (!SelectActions.IsValidIndex(SelectFocus))
	{
		return;
	}
	TFunction<void()> Action = SelectActions[SelectFocus];
	if (Action)
	{
		Action();
	}
}


