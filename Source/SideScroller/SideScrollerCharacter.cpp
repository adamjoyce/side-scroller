// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SideScroller.h"
#include "SideScrollerCharacter.h"
#include <EngineGlobals.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>

ASideScrollerCharacter::ASideScrollerCharacter() 
	: SideViewCameraComponent(CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"))),
	  CameraBoom(CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")))
{
	// Set size for collision capsule.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom attached to the root (capsule).
	//CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Rotation of the character should not affect rotation of boom
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 1000.f;
	CameraBoom->SocketOffset = FVector(0.f,0.f,75.f);
	CameraBoom->RelativeRotation = FRotator(0.f,180.f,0.f);

	// Create a camera and attach to boom.
	//SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false; // We don't want the controller rotating the camera

	// Configure character movement.
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASideScrollerCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings.
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAction("Dash", IE_Pressed, this, &ASideScrollerCharacter::Dash);
	InputComponent->BindAxis("MoveRight", this, &ASideScrollerCharacter::MoveRight);

	InputComponent->BindTouch(IE_Pressed, this, &ASideScrollerCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &ASideScrollerCharacter::TouchStopped);
}

void ASideScrollerCharacter::MoveRight(float Value)
{
	// Add movement in that direction.
	AddMovementInput(FVector(0.f,-1.f,0.f), Value);
}

void ASideScrollerCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch.
	Jump();
}

void ASideScrollerCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}

void ASideScrollerCharacter::Dash()
{
	UWorld* const World = GetWorld();
	if (World)
	{
		APlayerController* const MouseController = World->GetFirstPlayerController();

		float MouseXLoc = .0f;
		float MouseYLoc = .0f;
		MouseController->GetMousePosition(MouseXLoc, MouseYLoc);
		FVector WorldLoc = FVector(MouseXLoc, MouseYLoc, 0);
		FVector WorldDir = FVector(0, 0, 0);
		MouseController->DeprojectMousePositionToWorld(WorldLoc, WorldDir);

		// Have the character face the mouse position.
		FRotator CharacterRotation = GetActorRotation();
		int NewYaw = -90;
		if ((WorldLoc.Y - GetActorLocation().Y) > 0)
		{
			NewYaw = 90;
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("MouseXLoc: " + FString::FromInt(WorldLoc.Y)));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("CharacterXLoc: " + FString::FromInt(GetActorLocation().Y)));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("NewYaw: " + FString::FromInt(NewYaw)));

		FRotator NewRotation = FRotator(CharacterRotation.Pitch, NewYaw, CharacterRotation.Roll);
		SetActorRotation(NewRotation);
	}
}