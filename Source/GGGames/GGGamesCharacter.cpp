// Copyright Epic Games, Inc. All Rights Reserved.

#include "GGGamesCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);

//////////////////////////////////////////////////////////////////////////
// AGGGamesCharacter

AGGGamesCharacter::AGGGamesCharacter()
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));


	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->SetUsingAbsoluteRotation(true);
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	// 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
	// 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
	// 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
	// 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	// 	TextComponent->SetupAttachment(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

void AGGGamesCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGGGamesCharacter, m_bDuringGGAttack);
	DOREPLIFETIME(AGGGamesCharacter, m_bDuringGGDefence);
}

//////////////////////////////////////////////////////////////////////////
// Animation

void AGGGamesCharacter::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = (PlayerSpeedSqr > 0.0f) ? RunningAnimation : IdleAnimation;
	if( GetSprite()->GetFlipbook() != DesiredAnimation 	)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void AGGGamesCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCharacter();
}


//////////////////////////////////////////////////////////////////////////
// Input

void AGGGamesCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AGGGamesCharacter::GGAttack);

	PlayerInputComponent->BindAction("Defence", IE_Pressed, this, &AGGGamesCharacter::GGDefence);

	PlayerInputComponent->BindAxis("MoveUp", this, &AGGGamesCharacter::GGMoveUp);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGGGamesCharacter::GGMoveRight);
}

void AGGGamesCharacter::GGMoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	if (Value != 0.0f && !IsDuringGGAction())
	{
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void AGGGamesCharacter::GGMoveUp(float Value)
{
	if (Value != 0.0f && !IsDuringGGAction())
	{
		AddMovementInput(FVector::UpVector, Value);
	}
}

void AGGGamesCharacter::GGAttack_Implementation()
{
	if (!IsDuringGGAction())
	{
		ServerRequestAttack();
	}
}

void AGGGamesCharacter::GGDefence_Implementation()
{
	if (!IsDuringGGAction())
	{
		ServerRequestDefence();
	}
}

void AGGGamesCharacter::ServerRequestAttack_Implementation()
{
	if (!IsDuringGGAction())
	{
		SetDuringGGAttack(true);

		GetCharacterMovement()->Velocity = m_vecGGCurrentForward * m_fGGAttackVelocityStrength;
	}
}

void AGGGamesCharacter::ServerRequestDefence_Implementation()
{
	if (!IsDuringGGAction())
	{
		SetDuringGGDefence(true);

		m_fGGDefenceTimeCounter = GetWorld()->GetTimeSeconds() + m_fGGDefenceTime;
	}
}

void AGGGamesCharacter::UpdateCharacter()
{
	// Update animation to match the motion
	if (!IsDuringGGAction())
	{
		UpdateAnimation();
	}

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();
	float TravelDirection = PlayerVelocity.X;

	if (PlayerSpeedSqr > 0.0f)
	{
		m_vecGGCurrentForward = PlayerVelocity.GetSafeNormal();
		m_vecGGCurrentForward.Y = 0.0f;
	}

	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}

		if (HasAuthority())
		{
			if (IsDuringGGAttack() && FMath::IsNearlyEqual(PlayerSpeedSqr, 0.0f))
			{
				SetDuringGGAttack(false);
			}

			if (IsDuringGGDefence() && m_fGGDefenceTimeCounter < GetWorld()->GetTimeSeconds())
			{
				SetDuringGGDefence(false);
			}
		}
	}
}

void AGGGamesCharacter::SetDuringGGAttack_Implementation(bool bGGAttack)
{
	m_bDuringGGAttack = bGGAttack;

	if (m_bDuringGGAttack)
	{
		GetSprite()->SetFlipbook(AttackAnimation);
	}
}

void AGGGamesCharacter::SetDuringGGDefence_Implementation(bool bGGDefence)
{
	m_bDuringGGDefence = bGGDefence;

	if (m_bDuringGGDefence)
	{
		GetSprite()->SetFlipbook(DefenceAnimation);
	}
}