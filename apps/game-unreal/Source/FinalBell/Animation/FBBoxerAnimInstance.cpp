#include "Animation/FBBoxerAnimInstance.h"
#include "Combat/FBBoxerCharacter.h"
#include "Combat/FBCombatComponent.h"
#include "Combat/FBCombatTypes.h"

void UFBBoxerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const AFBBoxerCharacter* Boxer = Cast<AFBBoxerCharacter>(TryGetPawnOwner());
	if (!Boxer)
	{
		return;
	}

	MoveForward = Boxer->GetSmoothedMoveForward();
	MoveRight = Boxer->GetSmoothedMoveRight();
	const FVector2D Head = Boxer->GetReplicatedHeadAxes();
	HeadAxisX = Head.X;
	HeadAxisY = Head.Y;
	DefensePitch = Boxer->GetShownDefensePitch();
	DefenseRoll = Boxer->GetShownDefenseRoll();
	KneeDrop = Boxer->GetShownKneeDrop();
	PunchAlpha = Boxer->GetPunchPoseAlpha();
	AttackId = static_cast<uint8>(Boxer->GetDisplayAttack());
	bSouthpaw = Boxer->Stance == EFBStance::Southpaw;
	bPunchMasksLegs = false;

	const UFBCombatComponent* Combat = Boxer->GetCombatComponent();
	const EFBGuardState GuardState = Combat ? Combat->GetGuardState() : EFBGuardState::Open;
	switch (GuardState)
	{
	case EFBGuardState::High:
		Guard = 1;
		break;
	case EFBGuardState::Low:
		Guard = 2;
		break;
	default:
		Guard = 0;
		break;
	}
}
