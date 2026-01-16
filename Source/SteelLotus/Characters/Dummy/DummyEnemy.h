#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SteelLotus/Interfaces/LockableTargetInterface.h"
#include "DummyEnemy.generated.h"

class USceneComponent;

UCLASS()
class STEELLOTUS_API ADummyEnemy : public ACharacter, public ILockableTargetInterface
{
	GENERATED_BODY()

public:
	ADummyEnemy();

public:
	virtual FVector GetLockOnLocation_Implementation() const override;
	
	virtual bool IsLockable_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LockOn")
	USceneComponent* LockOnPoint;
};
