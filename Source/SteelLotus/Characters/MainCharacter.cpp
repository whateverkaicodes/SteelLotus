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
	if (TargetingComponent)
	{
		TargetingComponent->ToggleLockOn();
	}
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

