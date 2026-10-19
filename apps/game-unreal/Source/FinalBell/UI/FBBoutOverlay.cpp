#include "UI/FBBoutOverlay.h"
#include "Combat/FBSparringGameState.h"
#include "Combat/FBCombatTypes.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor Ink(0.035f, 0.039f, 0.047f, 0.94f);
	const FLinearColor Gold(0.839f, 0.627f, 0.235f, 1.f);
	const FLinearColor Cream(0.957f, 0.945f, 0.918f, 1.f);
	const FLinearColor Muted(0.655f, 0.627f, 0.580f, 1.f);
	const FLinearColor BlueCorner(0.122f, 0.294f, 0.600f, 1.f);
	const FLinearColor RedCorner(0.557f, 0.165f, 0.165f, 1.f);
	const FLinearColor Dim(0.f, 0.f, 0.f, 0.62f);

	const FSlateBrush* White()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	FSlateFontInfo Font(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
	}
}

void SFBGetUpDial::Construct(const FArguments& InArgs)
{
	Accent = InArgs._Accent;
}

void SFBGetUpDial::SetAngles(float InTargetDegrees, float InStickDegrees, bool bInStickActive)
{
	TargetDegrees = InTargetDegrees;
	StickDegrees = InStickDegrees;
	bStickActive = bInStickActive;
}

FVector2D SFBGetUpDial::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	(void)LayoutScaleMultiplier;
	return FVector2D(168.f, 168.f);
}

int32 SFBGetUpDial::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	(void)Args;
	(void)MyCullingRect;
	(void)InWidgetStyle;
	(void)bParentEnabled;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center = Size * 0.5f;
	const float Radius = FMath::Min(Size.X, Size.Y) * 0.42f;

	auto Tip = [Center, Radius](float Degrees, float Length)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return Center + FVector2D(FMath::Cos(Radians), -FMath::Sin(Radians)) * (Radius * Length);
	};

	TArray<FVector2D> Ring;
	Ring.Reserve(49);
	for (int32 Step = 0; Step <= 48; ++Step)
	{
		Ring.Add(Tip(Step * 7.5f, 1.f));
	}
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Ring, ESlateDrawEffect::None, FLinearColor(Gold.R, Gold.G, Gold.B, 0.85f), true, 2.5f);

	const TArray<FVector2D> Mark = { Center, Tip(TargetDegrees, 0.92f) };
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Mark, ESlateDrawEffect::None, Cream, true, 4.f);

	if (bStickActive)
	{
		const TArray<FVector2D> Stick = { Center, Tip(StickDegrees, 0.72f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Stick, ESlateDrawEffect::None, Accent, true, 5.f);
	}

	return LayerId + 3;
}

void SFBBoutOverlay::Construct(const FArguments& InArgs)
{
	(void)InArgs;
	SetVisibility(EVisibility::HitTestInvisible);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 22.f, 0.f, 0.f))
		[
			SAssignNew(ClockRoot, SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(Ink)
			.Padding(FMargin(28.f, 10.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("3:00 BOUT")))
					.ColorAndOpacity(Gold)
					.Font(Font(11))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 6.f)
				[
					SAssignNew(ClockText, STextBlock)
					.Text(FText::FromString(TEXT("3:00")))
					.ColorAndOpacity(Cream)
					.Font(Font(36))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
				[
					SAssignNew(RefereeText, STextBlock)
					.ColorAndOpacity(Gold)
					.Font(Font(14))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SAssignNew(ClinchText, STextBlock)
					.ColorAndOpacity(Cream)
					.Font(Font(16))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(3.f)
					[
						SNew(SBorder)
						.BorderImage(White())
						.BorderBackgroundColor(Gold)
					]
				]
			]
		]
		+ SOverlay::Slot()
		[
			SAssignNew(KnockdownRoot, SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(Dim)
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 120.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("GET UP")))
					.ColorAndOpacity(Gold)
					.Font(Font(18))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
				[
					SAssignNew(CountText, STextBlock)
					.Text(FText::FromString(TEXT("10")))
					.ColorAndOpacity(Cream)
					.Font(Font(92))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(18.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("LEFT")))
							.ColorAndOpacity(BlueCorner)
							.Font(Font(13))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).WidthOverride(168.f).HeightOverride(168.f)
							[
								SAssignNew(LeftDial, SFBGetUpDial).Accent(BlueCorner)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(18.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RIGHT")))
							.ColorAndOpacity(RedCorner)
							.Font(Font(13))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).WidthOverride(168.f).HeightOverride(168.f)
							[
								SAssignNew(RightDial, SFBGetUpDial).Accent(RedCorner)
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 16.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Hold both sticks on the marks. WASD is the left circle. Arrows are the right.")))
					.ColorAndOpacity(Muted)
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 13))
					.Justification(ETextJustify::Center)
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(CardRoot, SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(Ink)
			.Padding(FMargin(36.f, 28.f))
			.Visibility(EVisibility::Collapsed)
			[
				SNew(SBox)
				.WidthOverride(640.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("OFFICIAL SCORECARD")))
						.ColorAndOpacity(Gold)
						.Font(Font(13))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SAssignNew(CardTitle, STextBlock)
						.Text(FText::FromString(TEXT("3:00 BOUT")))
						.ColorAndOpacity(Cream)
						.Font(Font(22))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
					[
						SNew(SBox).HeightOverride(2.f)
						[
							SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Gold)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 10.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("BLUE                         RED")))
						.ColorAndOpacity(Cream)
						.Font(Font(16))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
					[
						SAssignNew(PillarLines, STextBlock)
						.ColorAndOpacity(Cream)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 15))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(JudgeLines, STextBlock)
						.ColorAndOpacity(Cream)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
					[
						SNew(SBox).HeightOverride(1.f)
						[
							SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor(Gold.R, Gold.G, Gold.B, 0.45f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 14.f)
					[
						SAssignNew(TotalLine, STextBlock)
						.ColorAndOpacity(Cream)
						.Font(Font(20))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SAssignNew(CardDecision, STextBlock)
						.ColorAndOpacity(Gold)
						.Font(Font(22))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
					[
						SAssignNew(CardWinner, STextBlock)
						.ColorAndOpacity(Cream)
						.Font(Font(28))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
					[
						SAssignNew(DeductionText, STextBlock)
						.ColorAndOpacity(Gold)
						.Font(Font(14))
					]
				]
			]
		]
	];
}

FString SFBBoutOverlay::FormatClock(float Seconds) const
{
	const int32 Whole = FMath::Clamp(FMath::CeilToInt(Seconds - KINDA_SMALL_NUMBER), 0, 60 * 60);
	return FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60);
}

float SFBBoutOverlay::StickDegrees(FVector2D Stick) const
{
	return FMath::RadiansToDegrees(FMath::Atan2(Stick.Y, Stick.X));
}

void SFBBoutOverlay::Refresh(float DeltaSeconds, const AFBSparringGameState* State, FVector2D LeftStick, FVector2D RightStick, bool bClinch, float ClinchMeter, int32 ClinchTurns)
{
	const bool bLiveClock = State && (State->bRoundRunning || State->bKnockdown) && !State->bCardVisible;
	const bool bShowClock = bLiveClock || (State && !State->bCardVisible && (bClinch || !State->RefereeCall.IsEmpty()));
	const bool bShowCount = State && State->bKnockdown && !State->bCardVisible;
	const bool bShowCard = State && State->bCardVisible && State->OfficialCard.Result != EFBBoutResult::None;

	if (ClockRoot.IsValid())
	{
		ClockRoot->SetVisibility(bShowClock ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
	if (KnockdownRoot.IsValid())
	{
		KnockdownRoot->SetVisibility(bShowCount ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
	if (CardRoot.IsValid())
	{
		CardRoot->SetVisibility(bShowCard ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
	if (!State)
	{
		return;
	}

	const float ServerClock = State->BoutSecondsRemaining;
	if (!bHaveClock || FMath::Abs(DisplayClock - ServerClock) > 0.55f)
	{
		DisplayClock = ServerClock;
		bHaveClock = true;
	}
	else if (State->bRoundRunning)
	{
		DisplayClock = FMath::Max(0.f, DisplayClock - DeltaSeconds);
	}
	if (ClockText.IsValid())
	{
		ClockText->SetText(FText::FromString(FormatClock(DisplayClock)));
	}
	if (RefereeText.IsValid())
	{
		const FString Call = State ? State->RefereeCall : FString();
		RefereeText->SetText(FText::FromString(Call));
		RefereeText->SetVisibility(Call.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
	if (ClinchText.IsValid())
	{
		ClinchText->SetText(FText::FromString(FString::Printf(TEXT("CLINCH   %d%%    TURNS %d"), FMath::RoundToInt(ClinchMeter), ClinchTurns)));
		ClinchText->SetVisibility(bClinch ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}

	const float Catch = FMath::Clamp(DeltaSeconds * 14.f, 0.f, 1.f);
	DisplayLeft = FMath::UnwindDegrees(DisplayLeft + FMath::FindDeltaAngleDegrees(DisplayLeft, State->LeftTargetDegrees) * Catch);
	DisplayRight = FMath::UnwindDegrees(DisplayRight + FMath::FindDeltaAngleDegrees(DisplayRight, State->RightTargetDegrees) * Catch);

	const bool bLeftHeld = LeftStick.Size() >= 0.58f;
	const bool bRightHeld = RightStick.Size() >= 0.58f;
	if (LeftDial.IsValid())
	{
		LeftDial->SetAngles(DisplayLeft, StickDegrees(LeftStick), bLeftHeld);
	}
	if (RightDial.IsValid())
	{
		RightDial->SetAngles(DisplayRight, StickDegrees(RightStick), bRightHeld);
	}
	if (CountText.IsValid())
	{
		const int32 Count = FMath::Clamp(FMath::CeilToInt(State->CountRemaining - KINDA_SMALL_NUMBER), 0, 10);
		CountText->SetText(FText::AsNumber(Count));
	}

	if (!bShowCard)
	{
		return;
	}

	const FFBOfficialCard& Card = State->OfficialCard;
	const bool bKnockout = Card.Result == EFBBoutResult::Knockout;
	if (CardTitle.IsValid())
	{
		CardTitle->SetText(FText::FromString(bKnockout ? TEXT("THE COUNT REACHED TEN") : TEXT("3:00 BOUT")));
	}
	if (CardDecision.IsValid())
	{
		CardDecision->SetText(FText::FromString(Card.Decision));
	}
	if (CardWinner.IsValid())
	{
		CardWinner->SetText(FText::FromString(Card.Winner));
	}
	if (DeductionText.IsValid())
	{
		DeductionText->SetText(FText::FromString(Card.DeductionNote));
		DeductionText->SetVisibility(Card.DeductionNote.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}

	FString Pillars;
	if (!bKnockout)
	{
		Pillars = FString::Printf(TEXT("GENERALSHIP    %3.0f         %3.0f\n"), Card.BlueGeneralship, Card.RedGeneralship);
		Pillars += FString::Printf(TEXT("KNOCKDOWNS     %3d         %3d\n"), Card.BlueKnockdowns, Card.RedKnockdowns);
		Pillars += FString::Printf(TEXT("PUNCHES      %5.1f       %5.1f"), Card.BluePunches, Card.RedPunches);
	}
	if (PillarLines.IsValid())
	{
		PillarLines->SetText(FText::FromString(Pillars));
		PillarLines->SetVisibility(bKnockout ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}

	FString Rows;
	if (!bKnockout)
	{
		for (const FFBJudgeRow& Row : Card.Judges)
		{
			if (!Rows.IsEmpty())
			{
				Rows += TEXT("\n");
			}
			Rows += FString::Printf(TEXT("%-12s    %2d          %2d"), *Row.Name, Row.BlueScore, Row.RedScore);
		}
	}
	if (JudgeLines.IsValid())
	{
		JudgeLines->SetText(FText::FromString(Rows));
		JudgeLines->SetVisibility(bKnockout ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
	if (TotalLine.IsValid())
	{
		TotalLine->SetText(FText::FromString(FString::Printf(TEXT("TOTAL          %2d          %2d"), Card.BlueTotal, Card.RedTotal)));
		TotalLine->SetVisibility(bKnockout ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
}
