#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UTargetingComponent;

UCLASS()
class STEELLOTUS_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    /** Handles movement input (WASD / left stick). */
	void Move(const FInputActionValue& Value);

	/** Handles look input (mouse / right stick). */
	void Look(const FInputActionValue& Value);
	
	/** Adds the default input mapping context to the local player. */
    void AddInputMappingContext();

	void LockOnPressed();

	/** Toggle weapon draw/sheath */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void ToggleWeapon();

	/** Helper: attach weapon to correct socket */
	void AttachWeaponToSocket(const FName SocketName);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeaponToHand();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeaponToSheath();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	/** Camera boom positioning the camera relative to the character's head. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;
	
	/** Follow camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	/** Weapon mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	UStaticMeshComponent* KatanaMesh;
	
    /** Default input mapping context for this character. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	/** Handles lock-on target selection / tracking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UTargetingComponent* TargetingComponent;
    
	/** Move input action. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;
    
    /** Look input action. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* LookAction;

	/** Toggle weapon input action */
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* ToggleWeaponAction;

	/** Lock-on input action. */
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* LockOnAction;

	UPROPERTY(BlueprintReadOnly)
	bool bWeaponDrawn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	bool bIsEquipping = false;

	/** Socket names (set to match your skeleton sockets) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName KatanaHandSocket = TEXT("Katana_Hand_R");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName KatanaSheathSocket = TEXT("Katana_Sheath");

	UPROPERTY(EditDefaultsOnly, Category="Weapon|Anim")
	UAnimMontage* DrawMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Weapon|Anim")
	UAnimMontage* SheathMontage = nullptr;
};
