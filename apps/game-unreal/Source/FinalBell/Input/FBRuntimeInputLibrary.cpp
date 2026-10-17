#include "Input/FBRuntimeInputLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float StickDeadZone = 0.38f;

	UInputAction* MakeAction(UObject* Outer, const FName Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
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
	Bundle.Jab = MakeAction(Outer, TEXT("IA_FB_Jab"), EInputActionValueType::Boolean);
	Bundle.Cross = MakeAction(Outer, TEXT("IA_FB_Cross"), EInputActionValueType::Boolean);
	Bundle.Guard = MakeAction(Outer, TEXT("IA_FB_Guard"), EInputActionValueType::Boolean);
	Bundle.MappingContext = NewObject<UInputMappingContext>(Outer, TEXT("IMC_FB_Boxer"));

	// Keyboard WASD (no stick deadzone)
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::W, true, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::S, true, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::D, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::A, false, true, false);

	// Gamepad left stick + D-pad
	{
		FEnhancedActionKeyMapping& Left2D = Bundle.MappingContext->MapKey(Bundle.Move, EKeys::Gamepad_Left2D);
		AddRadialDeadZone(Left2D, Bundle.MappingContext);
	}
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftY, true, false, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_LeftX, false, false, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Up, true, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Down, true, true, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Right, false, false, false);
	MapAxisKey(Bundle.MappingContext, Bundle.Move, EKeys::Gamepad_DPad_Left, false, true, false);

	// Orbit look: right stick + mouse + Q/E yaw
	{
		FEnhancedActionKeyMapping& Right2D = Bundle.MappingContext->MapKey(Bundle.Look, EKeys::Gamepad_Right2D);
		AddRadialDeadZone(Right2D, Bundle.MappingContext, 0.42f);
	}
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::Gamepad_RightY, true, true, true);
	MapAxisKey(Bundle.MappingContext, Bundle.Look, EKeys::Gamepad_RightX, false, false, true);
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

	// Jab / Cross / Guard
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::LeftMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::J);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Left);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_FaceButton_Bottom);
	Bundle.MappingContext->MapKey(Bundle.Jab, EKeys::Gamepad_RightTrigger);

	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::RightMouseButton);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::K);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_FaceButton_Top);
	Bundle.MappingContext->MapKey(Bundle.Cross, EKeys::Gamepad_RightShoulder);

	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::LeftShift);
	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::Gamepad_LeftShoulder);
	Bundle.MappingContext->MapKey(Bundle.Guard, EKeys::Gamepad_LeftTrigger);

	return Bundle;
}
