// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SideScroller.h"
#include "SideScrollerCharacter.h"
#include "DrawDebugHelpers.h"

//#include "UnrealString.h"
#include <EngineGlobals.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>

ASideScrollerCharacter::ASideScrollerCharacter() 
	: bIsDashing(false),
	  InterpSpeed(20.0f),
	  EndDashLocation(0, 0, 0),
	  SideViewCameraComponent(CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"))),
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


void ASideScrollerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*if (bIsDashing)
	{
		if ((GetActorLocation().X - EndDashLocation.X) < 1 && (GetActorLocation().Y - EndDashLocation.Y) < 1 && (GetActorLocation().Z - EndDashLocation.Z) < 1)
		{
			bIsDashing = false;
		}
		else
		{
			FVector NewLocation = FMath::VInterpTo(GetActorLocation(), EndDashLocation, GetWorld()->GetTimeSeconds(), InterpSpeed);
			SetActorLocation(NewLocation, true);
		}
	}*/
}

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

		// Get the mouse location in world space.
		FVector2D MouseScreenLoc = FVector2D(0.0f, 0.0f);
		MouseController->GetMousePosition(MouseScreenLoc.X, MouseScreenLoc.Y);

		// Convert mouse location to world space.
		FVector WorldLoc = FVector(0, MouseScreenLoc.X, MouseScreenLoc.Y);
		FVector WorldDir = FVector(-1, 0, 0);
		MouseController->DeprojectMousePositionToWorld(WorldLoc, WorldDir);

		// Deal with camera boom offset.
		WorldLoc.Z -= 75.0f;
		//WorldLocation += WorldDirection;
		//WorldLocation.Normalize();

		// Trace parameters.
		FVector TraceStartLoc = WorldLoc;
		FVector TraceEndLoc = TraceStartLoc + (WorldDir * 2000);
		
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("End: (" + FString::SanitizeFloat(TraceEndLoc.X) + ", " + FString::SanitizeFloat(TraceEndLoc.Y) + ", " + FString::SanitizeFloat(TraceEndLoc.Z) + ")"));
		FCollisionQueryParams TraceParams(FName(TEXT("MouseTrace")));

		// Perform raycast.
		UWorld* World = GetWorld();
		FHitResult HitResult(ForceInit);
		if (World)
		{
			World->LineTraceSingleByChannel(HitResult, TraceStartLoc, TraceEndLoc, ECC_WorldStatic, TraceParams);
			DrawDebugLine(World, TraceStartLoc, TraceEndLoc, FColor::Yellow, true);
			if (HitResult.GetActor() != NULL && HitResult.GetActor()->FindComponentByClass<UBoxComponent>()->ComponentHasTag("DashWall"))
			{
				// Have the character face the mouse position.
				FRotator CharacterRotation = GetActorRotation();
				int NewYaw = -90;
				if ((WorldLoc.Y - GetActorLocation().Y) > 0)
				{
					NewYaw = 90;
				}
				FRotator NewRotation = FRotator(CharacterRotation.Pitch, NewYaw, CharacterRotation.Roll);
				SetActorRotation(NewRotation);

				SetActorLocation(HitResult.ImpactPoint);
			}
			else if (HitResult.GetActor() == NULL) 
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("NO HIT"));
			}
		}

		// Set character end location and dash state.
		/*FVector DashVector = GetActorLocation() + WorldLocation;
		DashVector.Normalize();
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("DashVector: (" + FString::SanitizeFloat(DashVector.X) + ", " + FString::SanitizeFloat(DashVector.Y) + ", " + FString::SanitizeFloat(DashVector.Z) + ")"));
		DashVector += FVector(100);
		EndDashLocation = FVector(GetActorLocation().X, DashVector.Y, GetActorLocation().Z);
		SetActorLocation(GetActorLocation() + EndDashLocation);*/
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("EndDashLocation: (" + FString::SanitizeFloat(EndDashLocation.X) + ", " + FString::SanitizeFloat(EndDashLocation.Y) + ", " + FString::SanitizeFloat(EndDashLocation.Z) + ")"));
		//bIsDashing = true;

		// Get dash direction.
		/*FVector DashVector = GetActorLocation() + WorldLocation;
		DashVector.Normalize();
		GetCharacterMovement()->Add(FVector(0, DashVector.Y, DashVector.Z) * 1000.0f);*/
		///*GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Vector: (" + FString::SanitizeFloat(WorldLoc.X) + ", " + FString::SanitizeFloat(WorldLoc.Y) + ", " + FString::SanitizeFloat(WorldLoc.Z) + ")"));
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Vector: (" + FString::SanitizeFloat(GetActorLocation().X) + ", " + FString::SanitizeFloat(GetActorLocation().Y) + ", " + FString::SanitizeFloat(GetActorLocation().Z) + ")"));*/
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Vector: (" + FString::FromInt(0) + ", " + FString::SanitizeFloat(DashVector.Y) + ", " + FString::SanitizeFloat(-(DashVector.Z + 75)) + ")"));
	}
}