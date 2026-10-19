#include "Input/FBRuntimeInputLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "InputCoreTypes.h"

namespace
{
	// One stick dead zone. AxisConfig is 0, so this is not stacked on a second gate.
	constexpr float StickDeadZone = 0.22f;

	UInputAction* MakeAction(UObject* Outer, const FName Name, EInputActionValueType Type, bool bHold = false)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		if (Type == EInputActionValueType::Boolean)
		{
			// Pressed reports Triggered for one frame, then None, so Completed
			// fires while the button is still down. A hold has to use Down.
			if (bHold)
			{
				UInputTriggerDown* Down = NewObject<UInputTriggerDown>(Action);
				// The 0.35 gate lives on the trigger axis. This only ignores leftover noise.
				Down->ActuationThreshold = 0.01f;
				Action->Triggers.Add(Down);
			}
			else
			{
				UInputTriggerPressed* Pressed = NewObject<UInputTriggerPressed>(Action);
				Action->Triggers.Add(Pressed);
			}
		}
		return Action;
	}

	void AddRadialDeadZone(FEnhancedActionKeyMapping& Mapping, UObject* Outer, float Threshold = StickDeadZone)
	{
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(Outer);
		DeadZone->LowerThreshold = Threshold;
		DeadZone->UpperThreshold = 1.f;
		DeadZone->Type = EDeadZoneType::Radial;
		Mapping.Modifiers.Add(DeadZone);
	}

	// A DualSense L2/R2 is an axis. Below 0.35 is up. Past that is held.
	void MapAnalogTrigger(UInputMappingContext* IMC, UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Action, Key);
		UInputModifierDeadZone* DeadZone = NewObject<UInputModifierDeadZone>(IMC);
		DeadZone->LowerThreshold = 0.35f;
		DeadZone->UpperThreshold = 1.f;
		DeadZone->Type = EDeadZoneType::Axial;
		Mapping.Modifiers.Add(DeadZone);
	}

	void MapAxisKey(UInputMappingContext* IMC, UInputAction* Action, const FKey& Key, bool bSwizzleY, bool bNegate, bool bApplyDeadZone)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Action, Key);
		if (bApplyDeadZone)
		{
			AddRadialDeadZone(Mapping, IMC);
		}
		if (bSwizzleY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(IMC));
		}
	}
}

FFBRuntimeInputBundle UFBRuntimeInputLibrary::EnsureBoxerInput(UObject* Outer)
{
	FFBRuntimeInputBundle Bundle;
	if (!Outer)
	{
		return Bundle;
	}

	Bundle.Move = MakeAction(Outer, TEXT("IA_FB_Move"), EInputActionValueType::Axis2D);
	Bundle.Look = MakeAction(Outer, TEXT("IA_FB_Look"), EInputActionValueType::Axis2D);
	Bundle.PunchStick = MakeAction(Outer, TEXT("IA_FB_PunchStick"), EInputActionValueType::Axis2D);
	Bundle.Turn = MakeAction(Outer, TEXT("IA_FB_Turn"), EInputActionValueType::Axis2D);
	Bundle.Jab = MakeAction(Outer, TEXT("IA_FB_Jab"), EInputActionValueType::Boolean, true);
	Bundle.Cross = MakeAction(Outer, TEXT("IA_FB_Cross"), EInputActionValueType::Boolean, true);
	Bundle.LeadHook = MakeAction(Outer, TEXT("IA_FB_LeadHook"), EInputActionValueType::Boolean, true);
	Bundle.RearHook = MakeAction(Outer, TEXT("IA_FB_RearHook"), EInputActionValueType::Boolean, true);
	Bundle.Block = MakeAction(Outer, TEXT("IA_FB_Block"), EInputActionValueType::Boolean, true);
	Bundle.Weave = MakeAction(Outer, TEXT("IA_FB_Weave"), EInputActionValueType::Boolean, true);
	Bundle.BodyMod = MakeAction(Outer, TEXT("IA_FB_BodyMod"), EInputActionValueType::Boolean, true);
	Bundle.PowerMod = MakeAction(Outer, TEXT("IA_FB_PowerMod"), EInputActionValueType::Boolean, true);
	Bundle.MappingContext = NewObject<UInputMappingContext>(Outer, TEXT("IMC_FB_Boxer_UndisputedCompetitor"));

	// Move — left stick / WASD
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::W, true, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::S, true, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::D, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::A, false, true, false);
	{
		FEnhancedActionKeyMapping& Left2D = Bundle.MappingContext->MapKey(Bundle.Move, EKeys::Gamepad_Left2D);
		AddRadialDeadZone(Left2D, Bundle.MappingContext);
	}
	// Camera orbit — mouse + Q/E only. The right stick is mapped once, below.
	{
		FEnhancedActionKeyMapping& Mouse = Bundle.MappingContext->MapKey(Bundle.Look, EKeys::Mouse2D);
		if (UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(Bundle.MappingContext))
		{
			Scalar->Scalar = FVector(0.045f, 0.045f, 0.045f);
			Mouse.Modifiers.Add(Scalar);
		}
	}
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::E, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::Q, false, true, false);

	// One mapping for the right stick. Below the flick threshold it only yaws the shoulders.
	// At or above that threshold the character throws one punch and does not also yaw.
	// There is no second 0.78 gate on this axis. Keyboard punches stay on the face keys.
	{
		FEnhancedActionKeyMapping& Turn2D = Bundle.MappingContext->MapKey(Bundle.Turn, EKeys::Gamepad_Right2D);
		AddRadialDeadZone(Turn2D, Bundle.MappingContext, 0.12f);
	}
	// Arrow keys are the keyboard right stick, so a knockdown can be beaten without a pad.
	MapAxisKey(Bundle.MappingContext, Bundle.Turn, EKeys::Up, true, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Turn, EKeys::Down, true, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Turn, EKeys::Right, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Turn, EKeys::Left, false, true, false);

	// Face buttons — Competitor: Square/X jab, Triangle/Y cross, X/A lead hook, Circle/B rear hook
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Left);   // Square / X
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::J);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::LeftMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::One);

	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_FaceButton_Top);  // Triangle / Y
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::K);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::RightMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Two);

	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::Gamepad_FaceButton_Bottom); // X / A
	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::U);
	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::Three);
	Bundle.MappingContext->MapKey(Bundle.LeadHook, EKeys::MiddleMouseButton);

	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::Gamepad_FaceButton_Right); // Circle / B
	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::I);
	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::Four);
	Bundle.MappingContext->MapKey(Bundle.RearHook, EKeys::ThumbMouseButton);

	Bundle.LeadUppercut = MakeAction(Outer, TEXT("IA_FB_LeadUppercut"), EInputActionValueType::Boolean, true);
	Bundle.RearUppercut = MakeAction(Outer, TEXT("IA_FB_RearUppercut"), EInputActionValueType::Boolean, true);
	Bundle.MappingContext->MapKey(Bundle.LeadUppercut, EKeys::Y);
	Bundle.MappingContext->MapKey(Bundle.LeadUppercut, EKeys::Five);
	Bundle.MappingContext->MapKey(Bundle.RearUppercut, EKeys::H);
	Bundle.MappingContext->MapKey(Bundle.RearUppercut, EKeys::Six);

	// L2 and R2. A DualSense sends these as axes (see DefaultInput.ini), not as
	// Gamepad_LeftTrigger / Gamepad_RightTrigger button events.
	MapAnalogTrigger(Bundle.MappingContext, Bundle.Weave, EKeys::Gamepad_LeftTriggerAxis);
	MapAnalogTrigger(Bundle.MappingContext, Bundle.Block, EKeys::Gamepad_RightTriggerAxis);
	Bundle.MappingContext->MapKey(Bundle.Block, EKeys::Gamepad_RightTrigger);
	Bundle.MappingContext->MapKey(Bundle.Block, EKeys::LeftShift);
	Bundle.MappingContext->MapKey(Bundle.Weave, EKeys::Gamepad_LeftTrigger);
	Bundle.MappingContext->MapKey(Bundle.Weave, EKeys::LeftControl);

	// Modifiers: L1/C body, R1/V power
	Bundle.MappingContext->MapKey(Bundle.BodyMod, EKeys::Gamepad_LeftShoulder);
	Bundle.MappingContext->MapKey(Bundle.BodyMod, EKeys::C);
	Bundle.MappingContext->MapKey(Bundle.PowerMod, EKeys::Gamepad_RightShoulder);
	Bundle.MappingContext->MapKey(Bundle.PowerMod, EKeys::V);

	// D-pad. Arrow keys stay on the right stick, so the keyboard uses letter keys.
	Bundle.Loose = MakeAction(Outer, TEXT("IA_FB_Loose"), EInputActionValueType::Boolean);
	Bundle.Stance = MakeAction(Outer, TEXT("IA_FB_Stance"), EInputActionValueType::Boolean);
	Bundle.Taunt = MakeAction(Outer, TEXT("IA_FB_Taunt"), EInputActionValueType::Boolean);
	Bundle.Headbutt = MakeAction(Outer, TEXT("IA_FB_Headbutt"), EInputActionValueType::Boolean);
	Bundle.LeftThumb = MakeAction(Outer, TEXT("IA_FB_LeftThumb"), EInputActionValueType::Boolean, true);
	Bundle.RightThumb = MakeAction(Outer, TEXT("IA_FB_RightThumb"), EInputActionValueType::Boolean, true);

	Bundle.MappingContext->MapKey(Bundle.Loose, EKeys::Gamepad_DPad_Up);
	Bundle.MappingContext->MapKey(Bundle.Loose, EKeys::B);
	Bundle.MappingContext->MapKey(Bundle.Stance, EKeys::Gamepad_DPad_Right);
	Bundle.MappingContext->MapKey(Bundle.Stance, EKeys::G);
	Bundle.MappingContext->MapKey(Bundle.Taunt, EKeys::Gamepad_DPad_Left);
	Bundle.MappingContext->MapKey(Bundle.Taunt, EKeys::T);
	Bundle.MappingContext->MapKey(Bundle.Headbutt, EKeys::Gamepad_DPad_Down);
	Bundle.MappingContext->MapKey(Bundle.Headbutt, EKeys::N);
	Bundle.MappingContext->MapKey(Bundle.LeftThumb, EKeys::Gamepad_LeftThumbstick);
	Bundle.MappingContext->MapKey(Bundle.LeftThumb, EKeys::LeftAlt);
	Bundle.MappingContext->MapKey(Bundle.RightThumb, EKeys::Gamepad_RightThumbstick);
	Bundle.MappingContext->MapKey(Bundle.RightThumb, EKeys::RightAlt);

	return Bundle;
}
