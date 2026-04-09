#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Drone.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;

struct FInputActionValue;

UCLASS()
class ASSIGNMENT_07_API ADrone : public APawn
{
	GENERATED_BODY()

public:
	ADrone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Components")
	USkeletalMeshComponent* SkeletonComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Components")
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Components")
	UCameraComponent* CameraComp;

	UPROPERTY(EditAnywhere, Category="Drone|Physics")
	float GravityAccel = -980.f;

	UPROPERTY(EditAnywhere, Category="Drone|Physics")
	float Acceleration = 1500.f;

	UPROPERTY(EditAnywhere, Category="Drone|Physics")
	float Damping = 0.5f;

	UPROPERTY(EditAnywhere, Category="Drone|Physics")
	float AirAccelerationMultiplier = 0.4f;

	bool bIsGrounded = false;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UFUNCTION()
	void Move(const FInputActionValue& Value);

	UFUNCTION()
	void MoveEnd(const FInputActionValue& Value);

	UFUNCTION()
	void Look(const FInputActionValue& Value);

	UFUNCTION()
	void Rool(const FInputActionValue& Value);

private:
	FVector Velocity = FVector::ZeroVector;
	FVector MoveInputVec = FVector::ZeroVector;
};
