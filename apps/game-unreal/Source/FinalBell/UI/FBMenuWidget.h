// Poster main menu, settings, credits, and quit confirmation.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/FBCombatTypes.h"
#include "TimerManager.h"
#include "FBMenuWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UWidget;

UENUM()
enum class EFBMenuScreen : uint8
{
	Title,
	Main,
	Settings,
	DisplayConfirm,
	Credits,
	QuitConfirm,
	Loading
};

UENUM()
enum class EFBMainAction : uint8
{
	VersusAI,
	Multiplayer,
	Training,
	Career,
	Settings,
	Credits,
	Quit
};

UCLASS()
class FINALBELL_API UFBMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Confirm();
	void ReleaseConfirm();
	void Back();
	void Navigate(int32 Delta);
	void Adjust(int32 Delta);
	void Horizontal(int32 Delta);
	void NoteInputDevice(bool bGamepad);
	void ArmInitialFocus();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleTitleClicked();

	UFUNCTION()
	void HandleMainClicked();

	UFUNCTION()
	void HandleMainHovered();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleSettingsHovered();

	UFUNCTION()
	void HandleModalClicked();

	UFUNCTION()
	void BeginTravel();

private:
	struct FFBResolution
	{
		int32 X = 1920;
		int32 Y = 1080;
		FString Label;
	};

	void BuildInterface();
	void BuildMainEntries();
	void BuildSettingsEntries();
	void BuildResolutionList();
	void ShowScreen(EFBMenuScreen NewScreen);
	void RefreshMain();
	void RefreshSettings();
	void RefreshPrompt();
	void RefreshModal();
	void ApplyRowStyle(UButton* Button, UBorder* Marker, UTextBlock* Label, bool bSelected, bool bAvailable);
	void FocusButton(UButton* Button);
	void ActivateCurrent();
	void TravelToRing(EFBPlayMode Mode, bool bOpenFighterSelect);
	void ApplySettingsDraft();
	void KeepDisplayChanges();
	void RevertDisplayChanges();
	void RestoreAudioDraft();
	void LoadSettingsDraft();
	void ApplyDefaults();
	int32 FindActiveButton(const TArray<TObjectPtr<UButton>>& Buttons) const;
	bool ConsumeFrame(int32& LastFrame);
	UTextBlock* MakeText(const FString& Value, int32 Size, const FLinearColor& Color, bool bBold);
	UButton* MakeButton(UWidget* Content);

	UPROPERTY()
	TObjectPtr<UWidget> TitleLayer;

	UPROPERTY()
	TObjectPtr<UWidget> MainLayer;

	UPROPERTY()
	TObjectPtr<UWidget> SettingsLayer;

	UPROPERTY()
	TObjectPtr<UWidget> CreditsLayer;

	UPROPERTY()
	TObjectPtr<UWidget> ModalLayer;

	UPROPERTY()
	TObjectPtr<UWidget> LoadingLayer;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitlePrompt;

	UPROPERTY()
	TObjectPtr<UTextBlock> MainDescription;

	UPROPERTY()
	TObjectPtr<UTextBlock> MainStatus;

	UPROPERTY()
	TObjectPtr<UTextBlock> VersionLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> LoadingTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> SettingsStatus;

	UPROPERTY()
	TObjectPtr<UTextBlock> ModalTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> ModalBody;

	UPROPERTY()
	TObjectPtr<UButton> CreditsBackButton;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> MainButtons;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> MainMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> MainLabels;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> SettingsButtons;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> SettingsMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SettingsLabels;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SettingsValues;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> ModalButtons;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> ModalMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ModalLabels;

	TArray<EFBMainAction> MainActions;
	TArray<FString> MainTitles;
	TArray<FString> MainDescriptions;
	TArray<bool> MainAvailable;
	TArray<FFBResolution> Resolutions;
	TArray<FString> SettingsNames;

	EFBMenuScreen Screen = EFBMenuScreen::Title;
	int32 MainIndex = 0;
	int32 SettingsIndex = 0;
	int32 ModalIndex = 0;
	bool bUsingGamepad = false;
	bool bDeviceKnown = false;
	bool bTitleFading = false;
	bool bConsumeActivation = false;
	bool bSawRelease = false;
	bool bTravelRequested = false;
	bool bTrainingAvailable = false;
	float TitleFade = 0.f;
	float DisplaySecondsLeft = 0.f;

	float DraftVolume = 1.f;
	bool bDraftMotion = false;
	float BaselineVolume = 1.f;
	bool bBaselineMotion = false;
	FIntPoint DraftResolution = FIntPoint(1920, 1080);
	FIntPoint BaselineResolution = FIntPoint(1920, 1080);
	int32 DraftWindowMode = 0;
	int32 BaselineWindowMode = 0;
	int32 DraftQuality = 3;
	int32 BaselineQuality = 3;
	int32 ResolutionIndex = 0;

	int32 ConfirmFrame = -1;
	int32 BackFrame = -1;
	int32 NavFrame = -1;
	int32 AdjustFrame = -1;

	FTimerHandle TravelTimer;
};
