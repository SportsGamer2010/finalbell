#include "Combat/FBHeadMovement.h"

namespace FFBHeadMovement
{
	FVector2D ClampAxes(FVector2D Axis)
	{
		Axis.X = FMath::Clamp(Axis.X, -1.f, 1.f);
		Axis.Y = FMath::Clamp(Axis.Y, -1.f, 1.f);
		return Axis;
	}

	bool StateAllows(bool bKnockedDown, bool bInClinch)
	{
		return !bKnockedDown && !bInClinch;
	}

	bool ConsumesLocomotionStick(bool bWeaveHeld, bool bKnockedDown, bool bInClinch)
	{
		return bWeaveHeld && StateAllows(bKnockedDown, bInClinch);
	}

	EFBHeadMoveKind Classify(FVector2D Axis, bool bRollUnderHold)
	{
		if (bRollUnderHold)
		{
			return EFBHeadMoveKind::RollUnder;
		}
		const float X = FMath::Abs(Axis.X);
		const float Y = FMath::Abs(Axis.Y);
		if (X < 0.04f && Y < 0.04f)
		{
			return EFBHeadMoveKind::Neutral;
		}
		if (X >= Y)
		{
			return EFBHeadMoveKind::Slip;
		}
		if (Axis.Y < 0.f)
		{
			return EFBHeadMoveKind::LeanBack;
		}
		return EFBHeadMoveKind::LevelChange;
	}

	bool CountsAsCompletedEvasion(EFBHeadMoveKind Kind, FVector2D Axis, float CompletedAxis)
	{
		const float Need = FMath::Clamp(CompletedAxis, 0.3f, 0.9f);
		switch (Kind)
		{
		case EFBHeadMoveKind::RollUnder:
			return true;
		case EFBHeadMoveKind::Slip:
			return FMath::Abs(Axis.X) >= Need && FMath::Abs(Axis.X) >= FMath::Abs(Axis.Y);
		case EFBHeadMoveKind::LeanBack:
			return Axis.Y <= -Need && FMath::Abs(Axis.Y) >= FMath::Abs(Axis.X);
		default:
			return false;
		}
	}

	bool EvasionOnTime(
		float Now,
		float SlipStart,
		float SlipEnd,
		float HeadSince,
		EFBHeadMoveKind Kind,
		FVector2D Axis,
		float CompletedAxis,
		float ActiveWindowStart)
	{
		const bool bExplicit = SlipStart >= 0.f && Now < SlipEnd && SlipStart <= ActiveWindowStart;
		const bool bPlanted = HeadSince >= 0.f
			&& CountsAsCompletedEvasion(Kind, Axis, CompletedAxis)
			&& HeadSince <= ActiveWindowStart;
		return bExplicit || bPlanted;
	}

	FFBHeadPose PoseFromAxes(FVector2D Axis, EFBHeadMoveKind Kind, float RollUnderSign, const FFBHeadMovementLimits& Limits)
	{
		FFBHeadPose Pose;
		const float X = FMath::Clamp(Axis.X, -1.f, 1.f);
		const float Y = FMath::Clamp(Axis.Y, -1.f, 1.f);
		const float AbsX = FMath::Abs(X);
		const float AbsY = FMath::Abs(Y);
		switch (Kind)
		{
		case EFBHeadMoveKind::Neutral:
			Pose.Pitch = Limits.NeutralPitch;
			break;
		case EFBHeadMoveKind::Slip:
			Pose.Roll = -X * Limits.SlipRoll;
			Pose.Pitch = Limits.NeutralPitch * (1.f - AbsX);
			break;
		case EFBHeadMoveKind::LeanBack:
			Pose.Pitch = FMath::Lerp(Limits.NeutralPitch, Limits.LeanBackPitch, AbsY);
			Pose.Roll = -X * Limits.SlipRoll * 0.25f;
			break;
		case EFBHeadMoveKind::LevelChange:
			Pose.Pitch = FMath::Lerp(Limits.NeutralPitch, Limits.LevelPitch, FMath::Max(0.f, Y));
			Pose.KneeDropUU = Limits.LevelKneeDropUU * FMath::Max(0.f, Y);
			Pose.Roll = -X * Limits.SlipRoll * 0.20f;
			break;
		case EFBHeadMoveKind::RollUnder:
			Pose.Pitch = Limits.RollUnderPitch;
			Pose.Roll = -FMath::Clamp(RollUnderSign, -1.f, 1.f) * Limits.RollUnderRoll;
			Pose.KneeDropUU = Limits.LevelKneeDropUU * 0.65f;
			break;
		default:
			break;
		}
		return Pose;
	}

	FFBHeadPose ExplicitSlipPose(FVector2D Axis, float LeadSideSign, const FFBHeadMovementLimits& Limits)
	{
		FFBHeadPose Pose;
		if (Axis.SizeSquared() < FMath::Square(0.04f))
		{
			return Pose;
		}
		const FVector2D N = Axis.GetSafeNormal();
		if (FMath::Abs(N.X) >= FMath::Abs(N.Y))
		{
			Pose.Roll = -N.X * Limits.SlipRoll;
			Pose.Pitch = Limits.NeutralPitch * 0.5f;
			return Pose;
		}
		if (N.Y < 0.f)
		{
			Pose.Pitch = Limits.LeanBackPitch;
			Pose.Roll = -N.X * Limits.SlipRoll * 0.25f;
			return Pose;
		}
		const float Side = FMath::Abs(N.X) > 0.20f ? N.X : FMath::Clamp(LeadSideSign, -1.f, 1.f);
		Pose.Pitch = Limits.RollUnderPitch;
		Pose.Roll = -Side * Limits.RollUnderRoll;
		Pose.KneeDropUU = Limits.LevelKneeDropUU * 0.5f;
		return Pose;
	}

	FVector2D PresentationAxes(bool bOwner, FVector2D LocalStick, FVector2D ReplicatedAxes)
	{
		return bOwner ? LocalStick : ReplicatedAxes;
	}

	EFBGuardState SelectBlockGuard(bool bBlockHeld, bool bHeadConsumesStick, bool bLatchedLow, float StickY, float LowGuardStickY)
	{
		if (!bBlockHeld)
		{
			return EFBGuardState::Open;
		}
		if (bHeadConsumesStick)
		{
			return bLatchedLow ? EFBGuardState::Low : EFBGuardState::High;
		}
		return StickY < LowGuardStickY ? EFBGuardState::Low : EFBGuardState::High;
	}

	void TrimHistory(TArray<FFBHeadStickSample>& History, float Now, float WindowSeconds)
	{
		const float Keep = FMath::Max(0.05f, WindowSeconds);
		History.RemoveAll([Now, Keep](const FFBHeadStickSample& Sample)
		{
			return Now - Sample.Time > Keep;
		});
	}

	bool DetectRollUnderArc(const TArray<FFBHeadStickSample>& History, float Now, float& OutSign)
	{
		OutSign = 0.f;
		constexpr float Side = 35.f;
		constexpr float ForwardCone = 25.f;

		struct FAngle
		{
			float Time = 0.f;
			float Degrees = 0.f;
		};
		TArray<FAngle, TInlineAllocator<24>> Angles;
		for (const FFBHeadStickSample& Sample : History)
		{
			if (Now - Sample.Time > ArcWindowSeconds || Sample.Axis.Size() < ArcMinMagnitude)
			{
				continue;
			}
			FAngle Angle;
			Angle.Time = Sample.Time;
			Angle.Degrees = FMath::RadiansToDegrees(FMath::Atan2(Sample.Axis.X, Sample.Axis.Y));
			Angles.Add(Angle);
		}

		for (int32 I = 0; I < Angles.Num(); ++I)
		{
			for (int32 J = I + 1; J < Angles.Num(); ++J)
			{
				if (FMath::Abs(Angles[J].Degrees) > ForwardCone)
				{
					continue;
				}
				for (int32 K = J + 1; K < Angles.Num(); ++K)
				{
					const float Enter = Angles[I].Degrees;
					const float Exit = Angles[K].Degrees;
					if (Enter <= -Side && Exit >= Side)
					{
						OutSign = 1.f;
						return true;
					}
					if (Enter >= Side && Exit <= -Side)
					{
						OutSign = -1.f;
						return true;
					}
				}
			}
		}
		return false;
	}
}
