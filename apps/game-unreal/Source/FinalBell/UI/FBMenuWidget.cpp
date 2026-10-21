#include "UI/FBMenuWidget.h"
#include "UI/FBMenuArt.h"
#include "UI/FBMenuDeveloperSettings.h"
#include "UI/FBGameInstance.h"
#include "UI/FBPlayerSettings.h"
#include "Session/FBSessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "GeneralProjectSettings.h"
#include "GenericPlatform/GenericApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

namespace
{
	const UFBMenuDeveloperSettings* Style()
	{
		return UFBMenuDeveloperSettings::Get();
	}

	FLinearColor Charcoal()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->Charcoal : FLinearColor(0.0667f, 0.0706f, 0.0784f, 0.94f);
	}

	FLinearColor WarmWhite()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->WarmWhite : FLinearColor(0.9569f, 0.9412f, 0.9020f, 1.f);
	}

	FLinearColor BoxingRed()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->BoxingRed : FLinearColor(0.7765f, 0.1765f, 0.1765f, 1.f);
	}

	FLinearColor MutedGold()
	{
		const UFBMenuDeveloperSettings* Settings = Style();
		return Settings ? Settings->MutedGold : FLinearColor(0.7137f, 0.6039f, 0.3529f, 1.f);
	}

	FSlateFontInfo Font(int32 Size, bool bBold, int32 Tracking)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
		Info.LetterSpacing = Tracking;
		return Info;
	}

	void SetLayer(UWidget* Widget, ESlateVisibility Visibility)
	{
		if (Widget)
		{
			Widget->SetVisibility(Visibility);
		}
	}
}

void UFBMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (FSlateApplication::IsInitialized())
	{
		bUsingGamepad = FSlateApplication::Get().IsGamepadAttached();
	}
	BuildResolutionList();
	BuildInterface();
	LoadSettingsDraft();
	MainIndex = 0;
	for (int32 Index = 0; Index < MainAvailable.Num(); ++Index)
	{
		if (MainAvailable[Index])
		{
			MainIndex = Index;
			break;
		}
	}
	ShowScreen(EFBMenuScreen::Main);
}

void UFBMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const UFBMenuDeveloperSettings* MenuStyle = Style();
	const UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate();
	float Duration = MenuStyle ? MenuStyle->FadeSeconds : 0.35f;
	if (Saved && Saved->bReducedMotion)
	{
		Duration = 0.f;
	}

	if (bTitleFading)
	{
		TitleFade = Duration <= 0.f ? 1.f : FMath::Min(1.f, TitleFade + InDeltaTime / Duration);
		if (TitleLayer)
		{
			TitleLayer->SetRenderOpacity(1.f - TitleFade);
		}
		if (MainLayer)
		{
			MainLayer->SetRenderOpacity(TitleFade);
		}
		if (TitleFade >= 1.f)
		{
			bTitleFading = false;
			SetLayer(TitleLayer, ESlateVisibility::Collapsed);
			SetLayer(MainLayer, ESlateVisibility::SelfHitTestInvisible);
		}
	}

	if (bConsumeActivation && bSawRelease && !bTitleFading)
	{
		bConsumeActivation = false;
		if (MainButtons.IsValidIndex(MainIndex))
		{
			FocusButton(MainButtons[MainIndex]);
		}
	}

	if (Screen == EFBMenuScreen::DisplayConfirm)
	{
		DisplaySecondsLeft = FMath::Max(0.f, DisplaySecondsLeft - InDeltaTime);
		RefreshModal();
		if (DisplaySecondsLeft <= 0.f)
		{
			RevertDisplayChanges();
		}
	}
}

FReply UFBMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	NoteInputDevice(Key.IsGamepadKey());
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Confirm();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Left || Key == EKeys::Gamepad_Special_Right)
	{
		Back();
		return FReply::Handled();
	}
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		Navigate(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		Navigate(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
	{
		Horizontal(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
	{
		Horizontal(1);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UFBMenuWidget::Confirm()
{
	if (!ConsumeFrame(ConfirmFrame) || bConsumeActivation || bTravelRequested)
	{
		return;
	}
	if (Screen == EFBMenuScreen::Title)
	{
		bConsumeActivation = true;
		bSawRelease = false;
		bTitleFading = true;
		TitleFade = 0.f;
		Screen = EFBMenuScreen::Main;
		if (MainLayer)
		{
			MainLayer->SetRenderOpacity(0.f);
			MainLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		MainIndex = 0;
		for (int32 Index = 0; Index < MainAvailable.Num(); ++Index)
		{
			if (MainAvailable[Index])
			{
				MainIndex = Index;
				break;
			}
		}
		RefreshMain();
		return;
	}
	ActivateCurrent();
}

void UFBMenuWidget::ReleaseConfirm()
{
	bSawRelease = true;
}

void UFBMenuWidget::Back()
{
	if (!ConsumeFrame(BackFrame) || bConsumeActivation || bTravelRequested)
	{
		return;
	}
	if (Screen == EFBMenuScreen::Settings)
	{
		RestoreAudioDraft();
		ShowScreen(EFBMenuScreen::Main);
		return;
	}
	if (Screen == EFBMenuScreen::Credits)
	{
		ShowScreen(EFBMenuScreen::Main);
		return;
	}
	if (Screen == EFBMenuScreen::QuitConfirm)
	{
		ShowScreen(EFBMenuScreen::Main);
		return;
	}
	if (Screen == EFBMenuScreen::DisplayConfirm)
	{
		RevertDisplayChanges();
	}
}

void UFBMenuWidget::Navigate(int32 Delta)
{
	if (!ConsumeFrame(NavFrame) || Delta == 0 || bConsumeActivation || bTravelRequested)
	{
		return;
	}
	if (Screen == EFBMenuScreen::Main && MainButtons.Num() > 0)
	{
		MainIndex = (MainIndex + Delta + MainButtons.Num()) % MainButtons.Num();
		RefreshMain();
		FocusButton(MainButtons[MainIndex]);
		return;
	}
	if (Screen == EFBMenuScreen::Settings && SettingsButtons.Num() > 0)
	{
		SettingsIndex = (SettingsIndex + Delta + SettingsButtons.Num()) % SettingsButtons.Num();
		RefreshSettings();
		FocusButton(SettingsButtons[SettingsIndex]);
		return;
	}
	if ((Screen == EFBMenuScreen::QuitConfirm || Screen == EFBMenuScreen::DisplayConfirm) && ModalButtons.Num() > 0)
	{
		ModalIndex = (ModalIndex + Delta + ModalButtons.Num()) % ModalButtons.Num();
		RefreshModal();
		FocusButton(ModalButtons[ModalIndex]);
	}
}

void UFBMenuWidget::Horizontal(int32 Delta)
{
	if (Screen == EFBMenuScreen::Settings)
	{
		Adjust(Delta);
		return;
	}
	Navigate(Delta);
}

void UFBMenuWidget::Adjust(int32 Delta)
{
	if (!ConsumeFrame(AdjustFrame) || Delta == 0 || Screen != EFBMenuScreen::Settings || bTravelRequested)
	{
		return;
	}
	if (SettingsIndex == 0)
	{
		DraftVolume = FMath::Clamp(DraftVolume + Delta * 0.05f, 0.f, 1.f);
		if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
		{
			Saved->MasterVolume = DraftVolume;
			Saved->ApplyAudio(this);
		}
	}
	else if (SettingsIndex == 1)
	{
		DraftWindowMode = (DraftWindowMode + Delta + 3) % 3;
	}
	else if (SettingsIndex == 2 && Resolutions.Num() > 0)
	{
		ResolutionIndex = (ResolutionIndex + Delta + Resolutions.Num()) % Resolutions.Num();
		DraftResolution = FIntPoint(Resolutions[ResolutionIndex].X, Resolutions[ResolutionIndex].Y);
	}
	else if (SettingsIndex == 3)
	{
		DraftQuality = (DraftQuality + Delta + 5) % 5;
	}
	else if (SettingsIndex == 4)
	{
		bDraftMotion = !bDraftMotion;
		if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
		{
			Saved->bReducedMotion = bDraftMotion;
		}
	}
	else
	{
		Navigate(Delta);
		return;
	}
	RefreshSettings();
}

void UFBMenuWidget::NoteInputDevice(bool bGamepad)
{
	bDeviceKnown = true;
	if (bUsingGamepad == bGamepad)
	{
		return;
	}
	bUsingGamepad = bGamepad;
	RefreshPrompt();
}

void UFBMenuWidget::HandleTitleClicked()
{
	NoteInputDevice(false);
	Confirm();
	bSawRelease = true;
}

void UFBMenuWidget::HandleMainClicked()
{
	const int32 Pressed = FindActiveButton(MainButtons);
	if (Pressed != INDEX_NONE)
	{
		MainIndex = Pressed;
	}
	RefreshMain();
	Confirm();
}

void UFBMenuWidget::HandleMainHovered()
{
	const int32 Hovered = FindActiveButton(MainButtons);
	if (Hovered == INDEX_NONE || Hovered == MainIndex || bConsumeActivation)
	{
		return;
	}
	MainIndex = Hovered;
	RefreshMain();
}

void UFBMenuWidget::HandleSettingsClicked()
{
	const int32 Pressed = FindActiveButton(SettingsButtons);
	if (Pressed != INDEX_NONE)
	{
		SettingsIndex = Pressed;
	}
	RefreshSettings();
	Confirm();
}

void UFBMenuWidget::HandleSettingsHovered()
{
	const int32 Hovered = FindActiveButton(SettingsButtons);
	if (Hovered == INDEX_NONE || Hovered == SettingsIndex)
	{
		return;
	}
	SettingsIndex = Hovered;
	RefreshSettings();
}

void UFBMenuWidget::HandleModalClicked()
{
	const int32 Pressed = FindActiveButton(ModalButtons);
	if (Pressed != INDEX_NONE)
	{
		ModalIndex = Pressed;
	}
	RefreshModal();
	Confirm();
}

void UFBMenuWidget::BeginTravel()
{
	const UFBMenuDeveloperSettings* MenuStyle = Style();
	const FString Map = (MenuStyle && !MenuStyle->TrainingMap.IsEmpty()) ? MenuStyle->TrainingMap : TEXT("/Game/Maps/Ring_P1");
	UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("game=/Script/FinalBell.FBSparringGameMode"));
}

void UFBMenuWidget::BuildInterface()
{
	if (!WidgetTree)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	WidgetTree->RootWidget = Root;

	auto Fill = [](UOverlay* Overlay, UWidget* Child)
	{
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Child))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
	};

	UBorder* Letterbox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Letterbox->SetBrushColor(FLinearColor(0.012f, 0.014f, 0.018f, 1.f));
	Fill(Root, Letterbox);

	UScaleBox* Frame = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	Frame->SetStretch(EStretch::ScaleToFill);
	UImage* Photo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Arena = FBMenuArt::GetArenaTexture())
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Arena);
		Brush.ImageSize = FVector2D(Arena->GetSizeX(), Arena->GetSizeY());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Photo->SetBrush(Brush);
	}
	Frame->SetContent(Photo);
	Fill(Root, Frame);

	UButton* TitleButton = MakeButton(nullptr);
	TitleButton->OnClicked.AddDynamic(this, &UFBMenuWidget::HandleTitleClicked);
	FButtonStyle ClearStyle;
	const FSlateColorBrush ClearBrush(FLinearColor::Transparent);
	ClearStyle.SetNormal(ClearBrush);
	ClearStyle.SetHovered(ClearBrush);
	ClearStyle.SetPressed(ClearBrush);
	TitleButton->SetStyle(ClearStyle);

	UOverlay* TitleOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	TitleButton->SetContent(TitleOverlay);
	UBorder* TitleColumn = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TitleColumn->SetBrushColor(Charcoal());
	TitleColumn->SetPadding(FMargin(48.f, 56.f));
	USizeBox* TitleWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TitleWidth->SetWidthOverride(600.f);
	TitleWidth->SetContent(TitleColumn);
	if (UOverlaySlot* TitleSlot = TitleOverlay->AddChildToOverlay(TitleWidth))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Left);
		TitleSlot->SetVerticalAlignment(VAlign_Fill);
		TitleSlot->SetPadding(FMargin(28.f, 36.f));
	}

	UVerticalBox* TitleStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TitleColumn->SetContent(TitleStack);
	const UFBMenuDeveloperSettings* MenuStyle = Style();
	const FString GameTitle = (MenuStyle && !MenuStyle->GameTitle.IsEmpty()) ? MenuStyle->GameTitle : TEXT("FINAL BELL");
	UTextBlock* Kicker = MakeText(TEXT("FIGHT NIGHT"), 14, MutedGold(), true);
	UTextBlock* Title = MakeText(GameTitle, 64, WarmWhite(), true);
	UTextBlock* About = MakeText(TEXT("Training and career."), 18, WarmWhite(), false);
	TitlePrompt = MakeText(TEXT(""), 18, MutedGold(), false);
	TitleStack->AddChildToVerticalBox(Kicker)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	TitleStack->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	UBorder* Rule = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Rule->SetBrushColor(BoxingRed());
	USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RuleSize->SetHeightOverride(3.f);
	RuleSize->SetWidthOverride(96.f);
	RuleSize->SetContent(Rule);
	TitleStack->AddChildToVerticalBox(RuleSize)->SetPadding(FMargin(0.f, 8.f, 0.f, 22.f));
	TitleStack->AddChildToVerticalBox(About)->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	USpacer* TitleGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	TitleGap->SetSize(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* GapSlot = TitleStack->AddChildToVerticalBox(TitleGap))
	{
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	TitleStack->AddChildToVerticalBox(TitlePrompt);
	TitleLayer = TitleButton;
	Fill(Root, TitleButton);

	UBorder* MainColumn = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	MainColumn->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	MainColumn->SetPadding(FMargin(8.f, 8.f));
	USizeBox* MainWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MainWidth->SetWidthOverride(680.f);
	MainWidth->SetContent(MainColumn);
	UVerticalBox* MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	MainColumn->SetContent(MainStack);
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* FinalWord = MakeText(TEXT("FINAL"), 84, WarmWhite(), true);
	FinalWord->SetFont(Font(84, true, 180));
	FinalWord->SetShadowOffset(FVector2D(0.f, 3.f));
	FinalWord->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	UBorder* Accent = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Accent->SetBrushColor(BoxingRed());
	USizeBox* AccentSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	AccentSize->SetWidthOverride(92.f);
	AccentSize->SetHeightOverride(10.f);
	AccentSize->SetContent(Accent);
	if (UHorizontalBoxSlot* WordSlot = TitleRow->AddChildToHorizontalBox(FinalWord))
	{
		WordSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* BarSlot = TitleRow->AddChildToHorizontalBox(AccentSize))
	{
		BarSlot->SetVerticalAlignment(VAlign_Center);
		BarSlot->SetPadding(FMargin(18.f, 8.f, 0.f, 0.f));
	}
	MainStack->AddChildToVerticalBox(TitleRow);
	UTextBlock* BellWord = MakeText(TEXT("BELL"), 84, WarmWhite(), true);
	BellWord->SetFont(Font(84, true, 180));
	BellWord->SetShadowOffset(FVector2D(0.f, 3.f));
	BellWord->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	MainStack->AddChildToVerticalBox(BellWord)->SetPadding(FMargin(0.f, 0.f, 0.f, 34.f));
	BuildMainEntries();
	for (int32 Index = 0; Index < MainButtons.Num(); ++Index)
	{
		MainStack->AddChildToVerticalBox(MainButtons[Index])->SetPadding(FMargin(0.f, 5.f));
	}
	MainStatus = MakeText(TEXT(""), 15, WarmWhite(), false);
	MainStatus->SetAutoWrapText(true);
	MainStack->AddChildToVerticalBox(MainStatus);
	USpacer* MainGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UVerticalBoxSlot* MainGapSlot = MainStack->AddChildToVerticalBox(MainGap))
	{
		MainGapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	VersionLabel = MakeText(TEXT(""), 12, MutedGold(), false);
	MainStack->AddChildToVerticalBox(VersionLabel);
	if (UOverlaySlot* MainSlot = Root->AddChildToOverlay(MainWidth))
	{
		MainSlot->SetHorizontalAlignment(HAlign_Left);
		MainSlot->SetVerticalAlignment(VAlign_Fill);
		MainSlot->SetPadding(FMargin(72.f, 64.f));
	}
	MainLayer = MainWidth;

	UBorder* SettingsColumn = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SettingsColumn->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	SettingsColumn->SetPadding(FMargin(8.f, 8.f));
	USizeBox* SettingsWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SettingsWidth->SetWidthOverride(680.f);
	SettingsWidth->SetContent(SettingsColumn);
	UVerticalBox* SettingsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SettingsColumn->SetContent(SettingsStack);
	SettingsStack->AddChildToVerticalBox(MakeText(TEXT("SETTINGS"), 14, MutedGold(), true))->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	SettingsStack->AddChildToVerticalBox(MakeText(TEXT("Audio and display"), 36, WarmWhite(), true))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	SettingsStack->AddChildToVerticalBox(MakeText(TEXT("Left and right change a setting. Apply saves it."), 16, WarmWhite(), false))->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));
	BuildSettingsEntries();
	for (UButton* Button : SettingsButtons)
	{
		SettingsStack->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 3.f));
	}
	SettingsStatus = MakeText(TEXT(""), 14, MutedGold(), false);
	SettingsStatus->SetAutoWrapText(true);
	SettingsStack->AddChildToVerticalBox(SettingsStatus)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	if (UOverlaySlot* SettingsSlot = Root->AddChildToOverlay(SettingsWidth))
	{
		SettingsSlot->SetHorizontalAlignment(HAlign_Left);
		SettingsSlot->SetVerticalAlignment(VAlign_Fill);
		SettingsSlot->SetPadding(FMargin(72.f, 64.f));
	}
	SettingsLayer = SettingsWidth;

	UBorder* CreditsColumn = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CreditsColumn->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	CreditsColumn->SetPadding(FMargin(8.f, 8.f));
	USizeBox* CreditsWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CreditsWidth->SetWidthOverride(640.f);
	CreditsWidth->SetContent(CreditsColumn);
	UVerticalBox* CreditsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CreditsColumn->SetContent(CreditsStack);
	CreditsStack->AddChildToVerticalBox(MakeText(TEXT("CREDITS"), 14, MutedGold(), true))->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	const UGeneralProjectSettings* Project = GetDefault<UGeneralProjectSettings>();
	auto AddCredit = [&](const FString& Heading, const FString& Value)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		CreditsStack->AddChildToVerticalBox(MakeText(Heading, 13, MutedGold(), true))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
		UTextBlock* Line = MakeText(Value, 20, WarmWhite(), false);
		Line->SetAutoWrapText(true);
		CreditsStack->AddChildToVerticalBox(Line);
	};
	if (Project)
	{
		AddCredit(TEXT("TITLE"), Project->ProjectName);
		AddCredit(TEXT("DESCRIPTION"), Project->Description);
		AddCredit(TEXT("STUDIO"), Project->CompanyName);
		AddCredit(TEXT("COPYRIGHT"), Project->CopyrightNotice);
		if (!Project->ProjectVersion.IsEmpty())
		{
			AddCredit(TEXT("VERSION"), Project->ProjectVersion);
		}
	}
	AddCredit(TEXT("ENGINE"), TEXT("Unreal Engine"));
	CreditsStack->AddChildToVerticalBox(MakeText(TEXT("Edit these lines in Project Settings, or in Config/DefaultGame.ini."), 14, WarmWhite(), false))->SetPadding(FMargin(0.f, 28.f, 0.f, 18.f));
	CreditsBackButton = MakeButton(MakeText(TEXT("Back"), 20, WarmWhite(), true));
	CreditsBackButton->OnClicked.AddDynamic(this, &UFBMenuWidget::HandleMainClicked);
	CreditsStack->AddChildToVerticalBox(CreditsBackButton)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	if (UOverlaySlot* CreditsSlot = Root->AddChildToOverlay(CreditsWidth))
	{
		CreditsSlot->SetHorizontalAlignment(HAlign_Left);
		CreditsSlot->SetVerticalAlignment(VAlign_Fill);
		CreditsSlot->SetPadding(FMargin(72.f, 64.f));
	}
	CreditsLayer = CreditsWidth;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dimmer->SetBrushColor(FLinearColor(Charcoal().R, Charcoal().G, Charcoal().B, 0.86f));
	UBorder* Dialog = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Dialog->SetBrushColor(Charcoal());
	Dialog->SetPadding(FMargin(36.f));
	USizeBox* DialogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DialogSize->SetWidthOverride(560.f);
	DialogSize->SetContent(Dialog);
	UVerticalBox* DialogStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Dialog->SetContent(DialogStack);
	ModalTitle = MakeText(TEXT(""), 28, WarmWhite(), true);
	ModalBody = MakeText(TEXT(""), 16, WarmWhite(), false);
	ModalBody->SetAutoWrapText(true);
	DialogStack->AddChildToVerticalBox(ModalTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	DialogStack->AddChildToVerticalBox(ModalBody)->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));
	UHorizontalBox* ModalRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UBorder* Marker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		USizeBox* MarkerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MarkerSize->SetWidthOverride(4.f);
		MarkerSize->SetContent(Marker);
		UTextBlock* Label = MakeText(TEXT(""), 18, WarmWhite(), true);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MarkerSize)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		Row->AddChildToHorizontalBox(Label);
		UButton* Button = MakeButton(Row);
		Button->OnClicked.AddDynamic(this, &UFBMenuWidget::HandleModalClicked);
		ModalButtons.Add(Button);
		ModalMarkers.Add(Marker);
		ModalLabels.Add(Label);
		if (UHorizontalBoxSlot* ButtonSlot = ModalRow->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(Index == 0 ? 0.f : 12.f, 0.f, 0.f, 0.f));
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
	DialogStack->AddChildToVerticalBox(ModalRow);
	UOverlay* ModalOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Fill(ModalOverlay, Dimmer);
	if (UOverlaySlot* DialogSlot = ModalOverlay->AddChildToOverlay(DialogSize))
	{
		DialogSlot->SetHorizontalAlignment(HAlign_Center);
		DialogSlot->SetVerticalAlignment(VAlign_Center);
	}
	Fill(Root, ModalOverlay);
	ModalLayer = ModalOverlay;

	UBorder* LoadingDim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LoadingDim->SetBrushColor(FLinearColor(Charcoal().R, Charcoal().G, Charcoal().B, 0.78f));
	UVerticalBox* LoadingStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LoadingStack->AddChildToVerticalBox(MakeText(TEXT("ENTERING THE RING"), 14, WarmWhite(), true))->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	LoadingTitle = MakeText(TEXT("HEAVY BAG TRAINING"), 36, WarmWhite(), true);
	LoadingStack->AddChildToVerticalBox(LoadingTitle);
	LoadingDim->SetContent(LoadingStack);
	LoadingDim->SetPadding(FMargin(80.f));
	Fill(Root, LoadingDim);
	LoadingLayer = LoadingDim;

	const FString ProjectVersion = Project ? Project->ProjectVersion : FString();
	const FString BuildVersion = FApp::GetBuildVersion();
	if (VersionLabel)
	{
		if (!ProjectVersion.IsEmpty())
		{
			VersionLabel->SetText(FText::FromString(ProjectVersion));
		}
		else if (!BuildVersion.IsEmpty())
		{
			VersionLabel->SetText(FText::FromString(FString::Printf(TEXT("Build %s"), *BuildVersion)));
		}
		else
		{
			VersionLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshPrompt();
}

void UFBMenuWidget::BuildMainEntries()
{
	const UFBMenuDeveloperSettings* MenuStyle = Style();
	const FString TrainingMap = (MenuStyle && !MenuStyle->TrainingMap.IsEmpty()) ? MenuStyle->TrainingMap : TEXT("/Game/Maps/Ring_P1");
	bTrainingAvailable = FPackageName::DoesPackageExist(TrainingMap);

	struct FEntry
	{
		EFBMainAction Action;
		const TCHAR* Title;
		const TCHAR* Description;
		bool bAvailable;
	};
	const FEntry Entries[] = {
		{ EFBMainAction::VersusAI, TEXT("1V1 SPARRING (AI)"), TEXT("Pick two fighters. The bout starts after that."), bTrainingAvailable },
		{ EFBMainAction::Multiplayer, TEXT("1V1 MULTIPLAYER"), TEXT("Two players in one ring."), bTrainingAvailable },
		{ EFBMainAction::Training, TEXT("HEAVY BAG TRAINING"), bTrainingAvailable ? TEXT("Train on the hanging bag. No bout clock and no knockdowns.") : TEXT("The training level is not available."), bTrainingAvailable },
		{ EFBMainAction::Career, TEXT("1V1 CAREER"), TEXT("Three 3:00 rounds. Your created fighter against a premade in the same class. The result is saved on that fighter."), bTrainingAvailable },
		{ EFBMainAction::Settings, TEXT("SETTINGS"), TEXT("Master volume, display, and reduced motion."), true },
		{ EFBMainAction::Credits, TEXT("CREDITS"), TEXT("Project title, studio, and copyright."), true },
#if PLATFORM_DESKTOP
		{ EFBMainAction::Quit, TEXT("QUIT GAME"), TEXT("Close Final Bell."), true }
#endif
	};

	for (const FEntry& Entry : Entries)
	{
		UBorder* Marker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		USizeBox* MarkerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MarkerSize->SetWidthOverride(4.f);
		MarkerSize->SetContent(Marker);
		UTextBlock* Label = MakeText(Entry.Title, 22, WarmWhite(), true);
		Label->SetFont(Font(22, true, 140));
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UHorizontalBoxSlot* MarkerSlot = Row->AddChildToHorizontalBox(MarkerSize))
		{
			MarkerSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
			MarkerSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		UButton* Button = MakeButton(Row);
		Button->OnClicked.AddDynamic(this, &UFBMenuWidget::HandleMainClicked);
		Button->OnHovered.AddDynamic(this, &UFBMenuWidget::HandleMainHovered);
		MainButtons.Add(Button);
		MainMarkers.Add(Marker);
		MainLabels.Add(Label);
		MainActions.Add(Entry.Action);
		MainTitles.Add(Entry.Title);
		MainDescriptions.Add(Entry.Description);
		MainAvailable.Add(Entry.bAvailable);
	}
}

void UFBMenuWidget::BuildSettingsEntries()
{
	SettingsNames = {
		TEXT("Master Volume"),
		TEXT("Display Mode"),
		TEXT("Resolution"),
		TEXT("Graphics Quality"),
		TEXT("Reduced Motion"),
		TEXT("Restore Defaults"),
		TEXT("Apply"),
		TEXT("Back")
	};
	for (const FString& Name : SettingsNames)
	{
		UBorder* Marker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		USizeBox* MarkerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MarkerSize->SetWidthOverride(4.f);
		MarkerSize->SetContent(Marker);
		UTextBlock* Label = MakeText(Name, 18, WarmWhite(), true);
		UTextBlock* Value = MakeText(TEXT(""), 18, MutedGold(), false);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MarkerSize)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
		if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Label))
		{
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}
		Row->AddChildToHorizontalBox(Value)->SetVerticalAlignment(VAlign_Center);
		UButton* Button = MakeButton(Row);
		Button->OnClicked.AddDynamic(this, &UFBMenuWidget::HandleSettingsClicked);
		Button->OnHovered.AddDynamic(this, &UFBMenuWidget::HandleSettingsHovered);
		SettingsButtons.Add(Button);
		SettingsMarkers.Add(Marker);
		SettingsLabels.Add(Label);
		SettingsValues.Add(Value);
	}
}

void UFBMenuWidget::BuildResolutionList()
{
	const FFBResolution Candidates[] = {
		{ 1280, 720, TEXT("1280 x 720") },
		{ 1600, 900, TEXT("1600 x 900") },
		{ 1920, 1080, TEXT("1920 x 1080") },
		{ 2560, 1080, TEXT("2560 x 1080") },
		{ 2560, 1440, TEXT("2560 x 1440") },
		{ 3440, 1440, TEXT("3440 x 1440") },
		{ 3840, 2160, TEXT("3840 x 2160") }
	};
	FDisplayMetrics Metrics;
	FDisplayMetrics::RebuildDisplayMetrics(Metrics);
	const int32 DesktopW = Metrics.PrimaryDisplayWidth;
	const int32 DesktopH = Metrics.PrimaryDisplayHeight;
	for (const FFBResolution& Candidate : Candidates)
	{
		const bool bKnownDesktop = DesktopW > 0 && DesktopH > 0;
		if (!bKnownDesktop || (Candidate.X <= DesktopW + 32 && Candidate.Y <= DesktopH + 32))
		{
			Resolutions.Add(Candidate);
		}
	}
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const FIntPoint Current = UserSettings->GetScreenResolution();
		const bool bAlreadyListed = Resolutions.ContainsByPredicate([&](const FFBResolution& Item)
		{
			return Item.X == Current.X && Item.Y == Current.Y;
		});
		if (Current.X > 0 && Current.Y > 0 && !bAlreadyListed)
		{
			Resolutions.Add({ Current.X, Current.Y, FString::Printf(TEXT("%d x %d"), Current.X, Current.Y) });
		}
	}
	if (Resolutions.Num() == 0)
	{
		Resolutions.Add({ 1920, 1080, TEXT("1920 x 1080") });
	}
}

void UFBMenuWidget::ShowScreen(EFBMenuScreen NewScreen)
{
	Screen = NewScreen;
	SetLayer(TitleLayer, NewScreen == EFBMenuScreen::Title ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetLayer(MainLayer, NewScreen == EFBMenuScreen::Main ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	SetLayer(SettingsLayer, NewScreen == EFBMenuScreen::Settings ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	SetLayer(CreditsLayer, NewScreen == EFBMenuScreen::Credits ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	SetLayer(ModalLayer, (NewScreen == EFBMenuScreen::QuitConfirm || NewScreen == EFBMenuScreen::DisplayConfirm) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetLayer(LoadingLayer, NewScreen == EFBMenuScreen::Loading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (NewScreen == EFBMenuScreen::Main)
	{
		RefreshMain();
		if (MainButtons.IsValidIndex(MainIndex))
		{
			FocusButton(MainButtons[MainIndex]);
		}
	}
	else if (NewScreen == EFBMenuScreen::Settings)
	{
		LoadSettingsDraft();
		SettingsIndex = 0;
		if (SettingsStatus)
		{
			SettingsStatus->SetText(FText::GetEmpty());
		}
		RefreshSettings();
		if (SettingsButtons.IsValidIndex(0))
		{
			FocusButton(SettingsButtons[0]);
		}
	}
	else if (NewScreen == EFBMenuScreen::Credits)
	{
		ApplyRowStyle(CreditsBackButton, nullptr, nullptr, true, true);
		FocusButton(CreditsBackButton);
	}
	else if (NewScreen == EFBMenuScreen::QuitConfirm)
	{
		ModalIndex = 1;
		RefreshModal();
		if (ModalButtons.IsValidIndex(ModalIndex))
		{
			FocusButton(ModalButtons[ModalIndex]);
		}
	}
	else if (NewScreen == EFBMenuScreen::DisplayConfirm)
	{
		DisplaySecondsLeft = 15.f;
		ModalIndex = 0;
		RefreshModal();
		if (ModalButtons.IsValidIndex(ModalIndex))
		{
			FocusButton(ModalButtons[ModalIndex]);
		}
	}
	else if (NewScreen == EFBMenuScreen::Title)
	{
		RefreshPrompt();
		ArmInitialFocus();
	}
}

void UFBMenuWidget::ArmInitialFocus()
{
	if (Screen == EFBMenuScreen::Main && MainButtons.IsValidIndex(MainIndex))
	{
		FocusButton(MainButtons[MainIndex]);
		return;
	}
	if (UButton* TitleButton = Cast<UButton>(TitleLayer))
	{
		FocusButton(TitleButton);
	}
}

void UFBMenuWidget::RefreshMain()
{
	for (int32 Index = 0; Index < MainButtons.Num(); ++Index)
	{
		const bool bSelected = Index == MainIndex;
		if (MainLabels.IsValidIndex(Index))
		{
			MainLabels[Index]->SetText(FText::FromString(MainTitles[Index]));
		}
		ApplyRowStyle(MainButtons[Index], MainMarkers.IsValidIndex(Index) ? MainMarkers[Index] : nullptr, MainLabels.IsValidIndex(Index) ? MainLabels[Index] : nullptr, bSelected, MainAvailable.IsValidIndex(Index) ? MainAvailable[Index] : true);
	}
	if (MainDescription && MainDescriptions.IsValidIndex(MainIndex))
	{
		MainDescription->SetText(FText::FromString(MainDescriptions[MainIndex]));
		MainDescription->SetColorAndOpacity(MainAvailable.IsValidIndex(MainIndex) && MainAvailable[MainIndex] ? WarmWhite() : MutedGold());
	}
}

void UFBMenuWidget::RefreshSettings()
{
	const TCHAR* Modes[] = { TEXT("Windowed"), TEXT("Borderless"), TEXT("Fullscreen") };
	const TCHAR* Quality[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
	FString ResolutionLabel = FString::Printf(TEXT("%d x %d"), DraftResolution.X, DraftResolution.Y);
	if (Resolutions.IsValidIndex(ResolutionIndex))
	{
		ResolutionLabel = Resolutions[ResolutionIndex].Label;
	}
	const FString Values[] = {
		FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftVolume * 100.f)),
		Modes[FMath::Clamp(DraftWindowMode, 0, 2)],
		ResolutionLabel,
		Quality[FMath::Clamp(DraftQuality, 0, 4)],
		bDraftMotion ? TEXT("On") : TEXT("Off"),
		TEXT(""),
		TEXT(""),
		TEXT("")
	};
	for (int32 Index = 0; Index < SettingsButtons.Num(); ++Index)
	{
		const bool bSelected = Index == SettingsIndex;
		if (SettingsLabels.IsValidIndex(Index) && SettingsNames.IsValidIndex(Index))
		{
			SettingsLabels[Index]->SetText(FText::FromString(SettingsNames[Index]));
		}
		if (SettingsValues.IsValidIndex(Index))
		{
			SettingsValues[Index]->SetText(FText::FromString(Values[Index]));
			SettingsValues[Index]->SetColorAndOpacity(bSelected ? MutedGold() : WarmWhite());
		}
		ApplyRowStyle(SettingsButtons[Index], SettingsMarkers.IsValidIndex(Index) ? SettingsMarkers[Index] : nullptr, SettingsLabels.IsValidIndex(Index) ? SettingsLabels[Index] : nullptr, bSelected, true);
	}
}

void UFBMenuWidget::RefreshPrompt()
{
	if (!TitlePrompt)
	{
		return;
	}
	if (!bDeviceKnown && bUsingGamepad)
	{
		TitlePrompt->SetText(FText::FromString(TEXT("Press Enter, click, or A to continue")));
	}
	else if (bUsingGamepad)
	{
		TitlePrompt->SetText(FText::FromString(TEXT("Press A to continue")));
	}
	else
	{
		TitlePrompt->SetText(FText::FromString(TEXT("Press Enter or click to continue")));
	}
}

void UFBMenuWidget::RefreshModal()
{
	const bool bQuit = Screen == EFBMenuScreen::QuitConfirm;
	if (ModalTitle)
	{
		ModalTitle->SetText(FText::FromString(bQuit ? TEXT("Quit Game") : TEXT("Keep this display?")));
	}
	if (ModalBody)
	{
		if (bQuit)
		{
			ModalBody->SetText(FText::FromString(TEXT("Close Final Bell and return to the desktop.")));
		}
		else
		{
			ModalBody->SetText(FText::FromString(FString::Printf(TEXT("These display settings revert in %d seconds unless you keep them."), FMath::Max(1, FMath::CeilToInt(DisplaySecondsLeft)))));
		}
	}
	const TCHAR* Labels[] = {
		bQuit ? TEXT("Quit") : TEXT("Keep"),
		bQuit ? TEXT("Stay") : TEXT("Revert")
	};
	for (int32 Index = 0; Index < ModalButtons.Num(); ++Index)
	{
		const bool bSelected = Index == ModalIndex;
		if (ModalLabels.IsValidIndex(Index))
		{
			ModalLabels[Index]->SetText(FText::FromString(Labels[Index]));
		}
		ApplyRowStyle(ModalButtons[Index], ModalMarkers.IsValidIndex(Index) ? ModalMarkers[Index] : nullptr, ModalLabels.IsValidIndex(Index) ? ModalLabels[Index] : nullptr, bSelected, true);
	}
}

void UFBMenuWidget::ApplyRowStyle(UButton* Button, UBorder* Marker, UTextBlock* Label, bool bSelected, bool bAvailable)
{
	if (!Button)
	{
		return;
	}
	const FLinearColor Idle = FLinearColor(0.f, 0.f, 0.f, 0.f);
	const bool bShowBar = bSelected && bAvailable;
	const FLinearColor Bar = BoxingRed();
	auto Brush = [&](const FLinearColor& Fill)
	{
		FSlateColorBrush Result(Fill);
		Result.DrawAs = ESlateBrushDrawType::Box;
		return Result;
	};
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(Brush(bShowBar ? Bar : Idle));
	ButtonStyle.SetHovered(Brush(bAvailable ? Bar : Idle));
	ButtonStyle.SetPressed(Brush(bAvailable ? Bar : Idle));
	ButtonStyle.SetDisabled(Brush(Idle));
	ButtonStyle.NormalPadding = FMargin(0.f, 10.f, 18.f, 10.f);
	ButtonStyle.PressedPadding = FMargin(0.f, 10.f, 18.f, 10.f);
	Button->SetStyle(ButtonStyle);
	if (Marker)
	{
		Marker->SetBrushColor(bShowBar ? WarmWhite() : FLinearColor(1.f, 1.f, 1.f, 0.f));
	}
	if (Label)
	{
		FLinearColor TextColor = bAvailable ? FLinearColor(WarmWhite().R, WarmWhite().G, WarmWhite().B, 0.62f) : FLinearColor(WarmWhite().R, WarmWhite().G, WarmWhite().B, 0.32f);
		if (bShowBar)
		{
			TextColor = WarmWhite();
		}
		Label->SetColorAndOpacity(TextColor);
	}
}

void UFBMenuWidget::FocusButton(UButton* Button)
{
	if (Button)
	{
		Button->SetKeyboardFocus();
	}
}

void UFBMenuWidget::ActivateCurrent()
{
	if (bConsumeActivation || bTravelRequested)
	{
		return;
	}
	if (Screen == EFBMenuScreen::Main && MainActions.IsValidIndex(MainIndex))
	{
		if (!MainAvailable.IsValidIndex(MainIndex) || !MainAvailable[MainIndex])
		{
			if (MainStatus && MainDescriptions.IsValidIndex(MainIndex))
			{
				MainStatus->SetText(FText::FromString(MainDescriptions[MainIndex]));
			}
			return;
		}
		if (MainStatus)
		{
			MainStatus->SetText(FText::GetEmpty());
		}
		switch (MainActions[MainIndex])
		{
		case EFBMainAction::VersusAI:
			TravelToRing(EFBPlayMode::VersusAI, true);
			break;
		case EFBMainAction::Multiplayer:
			TravelToRing(EFBPlayMode::Multiplayer, false);
			break;
		case EFBMainAction::Training:
			TravelToRing(EFBPlayMode::Training, false);
			break;
		case EFBMainAction::Career:
			TravelToRing(EFBPlayMode::Career, true);
			break;
		case EFBMainAction::Settings:
			ShowScreen(EFBMenuScreen::Settings);
			break;
		case EFBMainAction::Credits:
			ShowScreen(EFBMenuScreen::Credits);
			break;
		case EFBMainAction::Quit:
			ShowScreen(EFBMenuScreen::QuitConfirm);
			break;
		default:
			break;
		}
		return;
	}
	if (Screen == EFBMenuScreen::Settings)
	{
		if (SettingsIndex == 5)
		{
			ApplyDefaults();
		}
		else if (SettingsIndex == 6)
		{
			ApplySettingsDraft();
		}
		else if (SettingsIndex == 7)
		{
			Back();
		}
		return;
	}
	if (Screen == EFBMenuScreen::QuitConfirm)
	{
		if (ModalIndex == 0)
		{
			FPlatformMisc::RequestExit(false);
		}
		else
		{
			ShowScreen(EFBMenuScreen::Main);
		}
		return;
	}
	if (Screen == EFBMenuScreen::DisplayConfirm)
	{
		if (ModalIndex == 0)
		{
			KeepDisplayChanges();
		}
		else
		{
			RevertDisplayChanges();
		}
		return;
	}
	if (Screen == EFBMenuScreen::Credits)
	{
		ShowScreen(EFBMenuScreen::Main);
	}
}

void UFBMenuWidget::TravelToRing(EFBPlayMode Mode, bool bOpenFighterSelect)
{
	if (bTravelRequested || Mode == EFBPlayMode::Unset)
	{
		return;
	}
	UGameInstance* GI = GetGameInstance();
	UFBSessionSubsystem* Session = GI ? GI->GetSubsystem<UFBSessionSubsystem>() : nullptr;
	if (!Session || !Session->TryLockTravel())
	{
		return;
	}
	const UFBMenuDeveloperSettings* MenuStyle = Style();
	const FString Map = (MenuStyle && !MenuStyle->TrainingMap.IsEmpty()) ? MenuStyle->TrainingMap : TEXT("/Game/Maps/Ring_P1");
	if (!FPackageName::DoesPackageExist(Map))
	{
		Session->UnlockTravel();
		bTrainingAvailable = false;
		for (int32 Index = 0; Index < MainActions.Num(); ++Index)
		{
			const bool bRingMode = MainActions[Index] == EFBMainAction::VersusAI
				|| MainActions[Index] == EFBMainAction::Multiplayer
				|| MainActions[Index] == EFBMainAction::Training
				|| MainActions[Index] == EFBMainAction::Career;
			if (!bRingMode)
			{
				continue;
			}
			if (MainDescriptions.IsValidIndex(Index))
			{
				MainDescriptions[Index] = TEXT("The training level is not available.");
			}
			if (MainAvailable.IsValidIndex(Index))
			{
				MainAvailable[Index] = false;
			}
		}
		RefreshMain();
		if (MainStatus)
		{
			MainStatus->SetText(FText::FromString(TEXT("The training level is not available.")));
		}
		return;
	}
	if (UFBGameInstance* FrontEnd = Cast<UFBGameInstance>(GI))
	{
		if (bOpenFighterSelect)
		{
			FrontEnd->ClearPendingBout();
		}
		FrontEnd->SetOpenFighterSelect(bOpenFighterSelect);
		FrontEnd->SetCareerFighterSelect(Mode == EFBPlayMode::Career && bOpenFighterSelect);
	}
	if (bOpenFighterSelect)
	{
		Session->ClearPlayMode();
	}
	else
	{
		Session->ChoosePlayMode(Mode);
	}
	Session->SetReturnToMenuAllowed(true);
	bTravelRequested = true;
	if (LoadingTitle)
	{
		const TCHAR* Heading =
			Mode == EFBPlayMode::VersusAI ? TEXT("1V1 SPARRING (AI)") :
			Mode == EFBPlayMode::Multiplayer ? TEXT("1V1 MULTIPLAYER") :
			Mode == EFBPlayMode::Career ? TEXT("1V1 CAREER") :
			TEXT("HEAVY BAG TRAINING");
		LoadingTitle->SetText(FText::FromString(Heading));
	}
	ShowScreen(EFBMenuScreen::Loading);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TravelTimer, this, &UFBMenuWidget::BeginTravel, 0.12f, false);
	}
	else
	{
		BeginTravel();
	}
}

void UFBMenuWidget::ApplySettingsDraft()
{
	if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
	{
		Saved->MasterVolume = DraftVolume;
		Saved->bReducedMotion = bDraftMotion;
		Saved->WriteToDisk();
		Saved->ApplyAudio(this);
	}
	BaselineVolume = DraftVolume;
	bBaselineMotion = bDraftMotion;

	const bool bDisplayChanged = DraftResolution != BaselineResolution || DraftWindowMode != BaselineWindowMode || DraftQuality != BaselineQuality;
	if (!bDisplayChanged)
	{
		if (SettingsStatus)
		{
			SettingsStatus->SetText(FText::FromString(TEXT("Settings saved.")));
		}
		return;
	}

	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type Modes[] = { EWindowMode::Windowed, EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen };
		UserSettings->SetScreenResolution(DraftResolution);
		UserSettings->SetFullscreenMode(Modes[FMath::Clamp(DraftWindowMode, 0, 2)]);
		UserSettings->SetOverallScalabilityLevel(DraftQuality);
		UserSettings->ApplyResolutionSettings(false);
		UserSettings->ApplyNonResolutionSettings();
		ShowScreen(EFBMenuScreen::DisplayConfirm);
		return;
	}
	if (SettingsStatus)
	{
		SettingsStatus->SetText(FText::FromString(TEXT("Display settings are not available.")));
	}
}

void UFBMenuWidget::KeepDisplayChanges()
{
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		UserSettings->ConfirmVideoMode();
		UserSettings->SaveSettings();
	}
	BaselineResolution = DraftResolution;
	BaselineWindowMode = DraftWindowMode;
	BaselineQuality = DraftQuality;
	ShowScreen(EFBMenuScreen::Settings);
	if (SettingsStatus)
	{
		SettingsStatus->SetText(FText::FromString(TEXT("Settings saved.")));
	}
}

void UFBMenuWidget::RevertDisplayChanges()
{
	DraftResolution = BaselineResolution;
	DraftWindowMode = BaselineWindowMode;
	DraftQuality = BaselineQuality;
	ResolutionIndex = 0;
	for (int32 Index = 0; Index < Resolutions.Num(); ++Index)
	{
		if (Resolutions[Index].X == DraftResolution.X && Resolutions[Index].Y == DraftResolution.Y)
		{
			ResolutionIndex = Index;
			break;
		}
	}
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type Modes[] = { EWindowMode::Windowed, EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen };
		UserSettings->SetScreenResolution(BaselineResolution);
		UserSettings->SetFullscreenMode(Modes[FMath::Clamp(BaselineWindowMode, 0, 2)]);
		UserSettings->SetOverallScalabilityLevel(BaselineQuality);
		UserSettings->ApplyResolutionSettings(false);
		UserSettings->ApplyNonResolutionSettings();
		UserSettings->ConfirmVideoMode();
		UserSettings->SaveSettings();
	}
	ShowScreen(EFBMenuScreen::Settings);
	if (SettingsStatus)
	{
		SettingsStatus->SetText(FText::FromString(TEXT("Display changes reverted.")));
	}
}

void UFBMenuWidget::RestoreAudioDraft()
{
	DraftVolume = BaselineVolume;
	bDraftMotion = bBaselineMotion;
	if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
	{
		Saved->MasterVolume = BaselineVolume;
		Saved->bReducedMotion = bBaselineMotion;
		Saved->ApplyAudio(this);
	}
}

void UFBMenuWidget::LoadSettingsDraft()
{
	if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
	{
		BaselineVolume = Saved->MasterVolume;
		bBaselineMotion = Saved->bReducedMotion;
		DraftVolume = BaselineVolume;
		bDraftMotion = bBaselineMotion;
		Saved->ApplyAudio(this);
	}
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		BaselineResolution = UserSettings->GetLastConfirmedScreenResolution();
		if (BaselineResolution.X <= 0 || BaselineResolution.Y <= 0)
		{
			BaselineResolution = UserSettings->GetScreenResolution();
		}
		switch (UserSettings->GetLastConfirmedFullscreenMode())
		{
		case EWindowMode::Windowed: BaselineWindowMode = 0; break;
		case EWindowMode::WindowedFullscreen: BaselineWindowMode = 1; break;
		default: BaselineWindowMode = 2; break;
		}
		BaselineQuality = UserSettings->GetOverallScalabilityLevel();
		if (BaselineQuality < 0 || BaselineQuality > 4)
		{
			BaselineQuality = 3;
		}
	}
	DraftResolution = BaselineResolution;
	DraftWindowMode = BaselineWindowMode;
	DraftQuality = BaselineQuality;
	ResolutionIndex = 0;
	for (int32 Index = 0; Index < Resolutions.Num(); ++Index)
	{
		if (Resolutions[Index].X == DraftResolution.X && Resolutions[Index].Y == DraftResolution.Y)
		{
			ResolutionIndex = Index;
			break;
		}
	}
}

void UFBMenuWidget::ApplyDefaults()
{
	DraftVolume = 1.f;
	bDraftMotion = false;
	DraftWindowMode = 1;
	DraftQuality = 3;
	DraftResolution = FIntPoint(1920, 1080);
	ResolutionIndex = 0;
	for (int32 Index = 0; Index < Resolutions.Num(); ++Index)
	{
		if (Resolutions[Index].X == 1920 && Resolutions[Index].Y == 1080)
		{
			ResolutionIndex = Index;
			DraftResolution = FIntPoint(Resolutions[Index].X, Resolutions[Index].Y);
			break;
		}
	}
	if (Resolutions.IsValidIndex(ResolutionIndex) && !(Resolutions[ResolutionIndex].X == 1920 && Resolutions[ResolutionIndex].Y == 1080))
	{
		DraftResolution = FIntPoint(Resolutions[0].X, Resolutions[0].Y);
		ResolutionIndex = 0;
	}
	if (UFBPlayerSettingsSave* Saved = UFBPlayerSettingsSave::LoadOrCreate())
	{
		Saved->MasterVolume = DraftVolume;
		Saved->ApplyAudio(this);
	}
	RefreshSettings();
	if (SettingsStatus)
	{
		SettingsStatus->SetText(FText::FromString(TEXT("Defaults are ready. Apply saves them.")));
	}
}

int32 UFBMenuWidget::FindActiveButton(const TArray<TObjectPtr<UButton>>& Buttons) const
{
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		if (Buttons[Index] && (Buttons[Index]->IsPressed() || Buttons[Index]->IsHovered()))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool UFBMenuWidget::ConsumeFrame(int32& LastFrame)
{
	const int32 Frame = static_cast<int32>(GFrameCounter);
	if (LastFrame == Frame)
	{
		return false;
	}
	LastFrame = Frame;
	return true;
}

UTextBlock* UFBMenuWidget::MakeText(const FString& Value, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Value));
	Text->SetFont(Font(Size, bBold, Size >= 36 ? 8 : 0));
	Text->SetColorAndOpacity(Color);
	return Text;
}

UButton* UFBMenuWidget::MakeButton(UWidget* Content)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	if (Content)
	{
		Button->SetContent(Content);
	}
	return Button;
}
