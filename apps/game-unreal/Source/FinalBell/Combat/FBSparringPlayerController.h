#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Combat/FBCombatTypes.h"
#include "Combat/FBFighterBalance.h"
#include "Combat/FBTacticalTypes.h"
#include "InputCoreTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "FBSparringPlayerController.generated.h"

class SWidget;
struct FInputKeyEventArgs;

struct FFBRowChrome
{
	TSharedPtr<SBorder> Fill;
	TSharedPtr<SBorder> Tick;
	TSharedPtr<STextBlock> Label;
};

UCLASS()
class FINALBELL_API AFBSparringPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFBSparringPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Console: FBMode 1=Sparring 2=Multiplayer 3=Training 4=Career */
	UFUNCTION(Exec)
	void FBMode(int32 ModeIndex);

	UFUNCTION(Exec)
	void FBHost(int32 Port = 7777);

	UFUNCTION(Exec)
	void FBJoin(const FString& HostAddress);

	UFUNCTION(Exec)
	void FBRoom();

	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	/** Menu keys. Returns true when the key belongs to a menu, or to pause while a bout is up. */
	bool RouteMenuKey(const FKey& Key, bool bRepeat);
	void PollMenuStick(float DeltaTime);
	TSharedRef<SWidget> WrapForPad(TSharedRef<SWidget> Content);
	void FocusMenuWidget(const TSharedPtr<SWidget>& Widget);
	bool IsMenuTakingInput() const;
	/** True when this local player should own the fight mapping. Menus and a lost window are false. */
	bool WantsFightMapping() const;

	void AddCreatorOrbit(float YawDelta, float PitchDelta);
	void AddCreatorZoom(float ZoomDelta);
	void SetCreatorDrag(bool bDragging);
	void SetCreatorMeasure(TSharedPtr<STextBlock> Measure);
	FString GetCreatorMeasureText() const;

protected:
	void ShowModeMenu();
	void HideModeMenu();
	void ChooseMode(EFBPlayMode Mode);
	void ShowFighterSelect();
	void HideFighterSelect();
	void RebuildFighterSelect();
	void ShowPauseMenu();
	void HidePauseMenu();
	void ResumeFromPause();
	void RequestRematch();
	void ReturnToMainMenu();
	void ChangeModeFromPause();
	void ActivateMenu();
	void NudgeMenu(int32 Delta);
	void BackMenu(bool bFromOptions);
	void PaintChrome(TArray<FFBRowChrome>& Rows, int32 Selected) const;
	void SuspendFightMapping();
	void RestoreFightMapping();
	void RefocusOpenMenu();
	bool ViewportHasFocus() const;
	void SyncFightMappingToFocus();
	TSharedRef<SWidget> MakeFocusRow(const FString& Label, TFunction<void()> Action);
	void ActivateSelectRow();
	void LoadSelectRoster();
	void SaveCurrentDraft();
	void ConfirmCompetitiveBout();
	void EnsureOpponentInClass();
	void CycleOpponent();
	bool IsLegalOpponentIndex(int32 Index) const;
	void CollectLegalOpponents(TArray<int32>& Out) const;

	UFUNCTION(Server, Reliable)
	void ServerStartCompetitiveBout(FFBFighterRecord PlayerBuild, FFBFighterRecord OpponentBuild, EFBPlayMode RequestedMode, EFBAIDifficulty Difficulty);

	UFUNCTION(Client, Reliable)
	void ClientBoutAccepted();

	UFUNCTION(Client, Reliable)
	void ClientBoutRefused(const FString& Why);

	/** Second PIE window: gym, movable lights, ringside camera. No extra fighter. */
	UFUNCTION(Client, Reliable)
	void ClientPresentSoloBout();

	class UGameViewportClient* LocalViewport() const;
	void PresentSoloRing();
	void KeepBroadcastView();
	void BroadcastSoloPresentation();

	/**
	 * Why: damage and stamina already replicate. The meters only draw them.
	 * What: adds the corner bars once the mode menu is gone, then refreshes
	 * them from the local pawn and the other fighter.
	 */
	void UpdateFightMeters();
	void RemoveFightMeters();
	void UpdateBoutOverlay(float DeltaTime);
	void RemoveBoutOverlay();
	void SubmitLocalGetUpAim();

	/**
	 * Why: the rotating marks live on the server. The downed fighter only sends where the sticks are.
	 * What: forwards both stick axes so the server can decide if the hold beats the count.
	 */
	UFUNCTION(Server, Reliable)
	void ServerSubmitGetUpAim(FVector2D LeftStick, FVector2D RightStick);

	UFUNCTION(Server, Reliable)
	void ServerRequestRematch();

	TSharedPtr<SWidget> ModeMenuWidget;
	TSharedPtr<SWidget> FightMeters;
	TSharedPtr<SWidget> BoutOverlay;
	bool bModeMenuVisible = false;
	bool bReturningToMenu = false;
	bool bFighterSelectVisible = false;
	bool bPauseOpen = false;
	bool bPauseConfirmQuit = false;
	bool bPauseForResult = false;
	bool bBoutStartPending = false;
	/** 1v1 vs AI or career is on this window. Keeps the ringside camera as the view. */
	bool bSoloPresentation = false;
	bool bSoloLookBuilt = false;
	/** This window has built the bag-room shell. The shell does not replicate. */
	bool bBagRoomLookBuilt = false;
	void EnsureLocalBagRoom();
	bool bBuilderOpen = false;
	float CreatorYaw = 180.f;
	float CreatorPitch = 6.f;
	float CreatorZoom = 0.62f;
	bool bCreatorCameraOn = false;
	bool bCreatorDragging = false;
	bool bCreatorIdleSpin = true;
	TSharedPtr<STextBlock> CreatorMeasure;
	void EnsureCreatorCamera();
	void ShutdownCreatorCamera();
	/** Career fighter select locks the opponent to a premade in the same class. */
	bool bCareerSelect = false;
	/** 0 to 1. The fighter screen slides in from this value so a rebuild does not restart it. */
	float SelectReveal = 1.f;
	int32 BuilderPage = 0;
	int32 PlayerPick = 0;
	int32 OpponentPick = 1;
	EFBAIDifficulty SelectDifficulty = EFBAIDifficulty::Club;
	TArray<FFBFighterRecord> SelectRoster;
	FFBFighterRecord Draft;
	FString SelectStatus;
	TSharedPtr<SWidget> FighterSelectWidget;
	TSharedPtr<SWidget> PauseWidget;
	int32 ModeFocus = 0;
	int32 PauseFocus = 0;
	int32 SelectFocus = 0;
	float MenuStickRepeat = 0.f;
	bool bFightMappingWanted = true;
	bool bSawViewportFocus = true;
	uint64 LastMenuFrame = 0;
	bool bHandledMenuKeyThisFrame = false;
	FString PauseStatus;
	TArray<FFBRowChrome> ModeRows;
	TArray<FFBRowChrome> PauseRows;
	TArray<FFBRowChrome> SelectRows;
	TArray<TFunction<void()>> SelectActions;
};
