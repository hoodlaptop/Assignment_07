#include "Drone.h"

#include "DroneController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"

ADrone::ADrone()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComp");
	SetRootComponent(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);

	SkeletonComp = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletonComp");
	SkeletonComp->SetupAttachment(CapsuleComp);

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComp");
	StaticMeshComp->SetupAttachment(CapsuleComp);

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(CapsuleComp);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
}

void ADrone::BeginPlay()
{
	Super::BeginPlay();
}

void ADrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FHitResult GroundHit;
	FVector Start = GetActorLocation();
	float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	FVector End = Start + FVector(0.f, 0.f, -(HalfHeight + 20.f));
	bIsGrounded = GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_Visibility);

	float AccelScale = bIsGrounded ? 1.0f : AirAccelerationMultiplier;

	FVector HorizontalInput = GetActorForwardVector() * MoveInputVec.X
		                    + GetActorRightVector()   * MoveInputVec.Y;

	Velocity += HorizontalInput * Acceleration * AccelScale * DeltaTime;
	Velocity.Z += MoveInputVec.Z * Acceleration * DeltaTime;
	Velocity.Z += GravityAccel * DeltaTime;
	Velocity *= FMath::Pow(Damping, DeltaTime);

	FHitResult HorizontalHit;
	AddActorWorldOffset(FVector(Velocity.X, Velocity.Y, 0.f) * DeltaTime, true, &HorizontalHit);
	if (HorizontalHit.IsValidBlockingHit())
	{
		Velocity.X = 0.f;
		Velocity.Y = 0.f;
	}

	FHitResult VerticalHit;
	AddActorWorldOffset(FVector(0.f, 0.f, Velocity.Z) * DeltaTime, true, &VerticalHit);
	if (VerticalHit.IsValidBlockingHit() && Velocity.Z < 0.f)
		Velocity.Z = 0.f;
}

void ADrone::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ADroneController* DroneController = Cast<ADroneController>(GetController()))
		{
			if (DroneController->MoveAction)
			{
				EnhancedInput->BindAction(DroneController->MoveAction, ETriggerEvent::Triggered, this, &ADrone::Move);
				EnhancedInput->BindAction(DroneController->MoveAction, ETriggerEvent::Completed, this, &ADrone::MoveEnd);
			}

			if (DroneController->LookAction)
			{
				EnhancedInput->BindAction(DroneController->LookAction, ETriggerEvent::Triggered, this, &ADrone::Look);
			}

			if (DroneController->RoolAction)
			{
				EnhancedInput->BindAction(DroneController->RoolAction, ETriggerEvent::Triggered, this, &ADrone::Rool);
			}
		}
	}
}

void ADrone::Move(const FInputActionValue& Value)
{
	if (!Controller) return;
	MoveInputVec = Value.Get<FVector3d>();
}

void ADrone::MoveEnd(const FInputActionValue& Value)
{
	MoveInputVec = FVector::ZeroVector;
}

void ADrone::Look(const FInputActionValue& Value)
{
	FVector2D LookInput = Value.Get<FVector2D>();
	AddActorLocalRotation(FRotator(0.0f, LookInput.X, 0.0f));
	AddActorLocalRotation(FRotator(LookInput.Y, 0.0f, 0.0f));
}

void ADrone::Rool(const FInputActionValue& Value)
{
	float RollInput = Value.Get<float>();
	AddActorLocalRotation(FRotator(0.0f, 0.0f, RollInput));
}
