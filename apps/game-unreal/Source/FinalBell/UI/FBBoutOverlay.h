// Final Bell — 3:00 clock, knockdown dials, and the official card.
// Slate, same broadcast colors as the corner meters. It reads the game state.
// It does not start the count, score the sticks, or pick a winner.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBorder.h"

class AFBSparringGameState;

/** One rotating mark and the stick you are holding against it. */
class SFBGetUpDial : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFBGetUpDial) {}
		SLATE_ARGUMENT(FLinearColor, Accent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetAngles(float InTargetDegrees, float InStickDegrees, bool bInStickActive);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	FLinearColor Accent = FLinearColor::White;
	float TargetDegrees = 0.f;
	float StickDegrees = 0.f;
	bool bStickActive = false;
};

class SFBBoutOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFBBoutOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Why: the server owns the clock, the count, and the card. This only draws them.
	 * What: smooths the 3:00 between updates, spins the two circles toward the server
	 * marks, and shows the scorecard or the knockout card when the bout is over.
	 */
	void Refresh(float DeltaSeconds, const AFBSparringGameState* State, FVector2D LeftStick, FVector2D RightStick, bool bClinch, float ClinchMeter, int32 ClinchTurns);

private:
	FString FormatClock(float Seconds) const;
	float StickDegrees(FVector2D Stick) const;

	TSharedPtr<SBorder> ClockRoot;
	TSharedPtr<SBorder> KnockdownRoot;
	TSharedPtr<SBorder> CardRoot;
	TSharedPtr<class STextBlock> ClockText;
	TSharedPtr<class STextBlock> ClockCaption;
	TSharedPtr<class STextBlock> CountText;
	TSharedPtr<SFBGetUpDial> LeftDial;
	TSharedPtr<SFBGetUpDial> RightDial;
	TSharedPtr<class STextBlock> CardTitle;
	TSharedPtr<class STextBlock> CardDecision;
	TSharedPtr<class STextBlock> CardWinner;
	TSharedPtr<class STextBlock> PillarLines;
	TSharedPtr<class STextBlock> JudgeLines;
	TSharedPtr<class STextBlock> TotalLine;
	TSharedPtr<class STextBlock> DeductionText;
	TSharedPtr<class STextBlock> RefereeText;
	TSharedPtr<class STextBlock> ClinchText;

	float DisplayClock = 180.f;
	float DisplayLeft = 0.f;
	float DisplayRight = 0.f;
	bool bHaveClock = false;
};
