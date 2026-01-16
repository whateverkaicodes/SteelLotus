#include "MainCharacter.h"

#include "Components/InputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SteelLotus/Components/TargetingComponent.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);          // IMPORTANT
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;          // camera follows controller

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Optional (common 3rd person setup)
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Katana mesh component (attach to character mesh)
	KatanaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KatanaMesh"));
	KatanaMesh->SetupAttachment(GetMesh()); // we will reattach to sockets at runtime
	KatanaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	AddInputMappingContext();

	// Start sheathed by default
	bWeaponDrawn = false;
	AttachWeaponToSocket(KatanaSheathSocket);
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller && !MovementVector.IsNearlyZero())
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		// Forward / backward
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// Right / left
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDir, MovementVector.Y);
		AddMovementInput(RightDir, MovementVector.X);
	}
}

void AMainCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	//UE_LOG(LogTemp, Warning, TEXT("Look axis: %s"), *LookAxis.ToString());

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(-LookAxis.Y);
}

void AMainCharacter::AddInputMappingContext()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}
}

void AMainCharacter::LockOnPressed()
{
	if (!TargetingComponent) return;

	TargetingComponent->ToggleLockOn();
	ApplyLockOnMode(TargetingComponent->IsLockedOn());

	// Snap instantly on the same frame
	UpdateLockOn(0.f);
}


void AMainCharacter::ApplyLockOnMode(bool bEnable)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (bEnable)
	{
		// store previous settings
		bPrevUseControllerRotationYaw = bUseControllerRotationYaw;
		bPrevOrientRotationToMovement = MoveComp->bOrientRotationToMovement;

		// Souls-like strafe mode
		bUseControllerRotationYaw = true;
		MoveComp->bOrientRotationToMovement = false;

		// HARD LOCK: stop player look input
		if (PC)
		{
			PC->SetIgnoreLookInput(true);
		}
	}
	else
	{
		// revert
		bUseControllerRotationYaw = bPrevUseControllerRotationYaw;
		MoveComp->bOrientRotationToMovement = bPrevOrientRotationToMovement;

		// restore look input
		if (PC)
		{
			PC->SetIgnoreLookInput(false);
		}
	}
}


void AMainCharacter::UpdateLockOn(float DeltaTime)
{
	// Not locked -> nothing to do
	if (!TargetingComponent || !TargetingComponent->IsLockedOn())
	{
		return;
	}

	AActor* Target = TargetingComponent->GetCurrentTarget();
	if (!IsValid(Target))
	{
		TargetingComponent->ClearLockOn();
		ApplyLockOnMode(false);
		return;
	}

	const FVector TargetLoc = TargetingComponent->GetCurrentTargetLocation();
	const FVector MyLoc = GetActorLocation();

	// Auto-unlock by distance
	const float MaxDistSq = FMath::Square(MaxLockDistance);
	if (FVector::DistSquared(MyLoc, TargetLoc) > MaxDistSq)
	{
		TargetingComponent->ClearLockOn();
		ApplyLockOnMode(false);
		return;
	}

	// 1) Face target (Yaw only) - Souls-like
	FVector ToTarget = (TargetLoc - MyLoc);
	ToTarget.Z = 0.f;

	if (!ToTarget.IsNearlyZero())
	{
		const FRotator DesiredYawRot(0.f, ToTarget.Rotation().Yaw, 0.f);
		const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), DesiredYawRot, DeltaTime, LockFaceInterpSpeed);
		SetActorRotation(NewRot);
	}

	// 2) Smooth Souls-like camera: snap if far, otherwise smooth
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		const FVector CamToTarget = (TargetLoc - CamLoc);
		if (!CamToTarget.IsNearlyZero())
		{
			FRotator Desired = CamToTarget.Rotation();
			Desired.Pitch = FMath::ClampAngle(Desired.Pitch, MinLockPitch, MaxLockPitch);
			Desired.Roll = 0.f;

			const FRotator Current = PC->GetControlRotation();

			// Optional: one-time snap if it’s way off (feels better than slow spin)
			const float AngleDelta = FMath::Abs(FRotator::NormalizeAxis(Desired.Yaw - Current.Yaw))
								   + FMath::Abs(FRotator::NormalizeAxis(Desired.Pitch - Current.Pitch));

			FRotator NewRot;
			if (AngleDelta > LockOnSnapAngleThreshold)
			{
				NewRot = Desired; // snap
			}
			else
			{
				NewRot = FMath::RInterpTo(Current, Desired, DeltaTime, LockOnCameraInterpSpeed); // smooth
			}

			PC->SetControlRotation(NewRot);
		}
	}

	// Optional debug (remove later)
	// DrawDebugLine(GetWorld(), MyLoc + FVector(0,0,50), TargetLoc, FColor::Green, false, -1.f, 0, 1.5f);
}

void AMainCharacter::ToggleWeapon()
{
	if (bIsEquipping) return;

	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	UAnimMontage* MontageToPlay = bWeaponDrawn ? SheathMontage : DrawMontage;
	if (!MontageToPlay) return;

	bIsEquipping = true;

	// Bind end callback once (safe to rebind: remove then add)
	AnimInst->OnMontageEnded.RemoveDynamic(this, &AMainCharacter::OnEquipMontageEnded);
	AnimInst->OnMontageEnded.AddDynamic(this, &AMainCharacter::OnEquipMontageEnded);

	AnimInst->Montage_Play(MontageToPlay, 1.0f);
}


void AMainCharacter::AttachWeaponToSocket(const FName SocketName)
{
	if (!KatanaMesh || !GetMesh()) return;

	KatanaMesh->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName
	);
}

void AMainCharacter::AttachWeaponToHand()
{
	AttachWeaponToSocket(KatanaHandSocket);
	bWeaponDrawn = true;
}

void AMainCharacter::AttachWeaponToSheath()
{
	AttachWeaponToSocket(KatanaSheathSocket);
	bWeaponDrawn = false;
}

void AMainCharacter::OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsEquipping = false;
}

void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLockOn(DeltaTime);
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look);
		}

		if (ToggleWeaponAction)
		{
			EnhancedInput->BindAction(ToggleWeaponAction, ETriggerEvent::Started, this, &AMainCharacter::ToggleWeapon);
		}

		if (LockOnAction)
		{
			EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &AMainCharacter::LockOnPressed);
		}
	}
}

