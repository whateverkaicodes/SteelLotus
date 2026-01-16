#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STEELLOTUS_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	/** Called by input (Tab) */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category="LockOn")
	void ClearLockOn();

	UFUNCTION(BlueprintCallable, Category="LockOn")
	bool IsLockedOn() const { return CurrentTargetActor.IsValid(); }

	UFUNCTION(BlueprintCallable, Category="LockOn")
	AActor* GetCurrentTarget() const { return CurrentTargetActor.Get(); }

	/** For debug/UI later */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	FVector GetCurrentTargetLocation() const;

protected:
	virtual void BeginPlay() override;

	/** Finds best target and locks onto it. Returns true if found. */
	bool AcquireBestTarget();

	/** Optional LOS check */
	bool HasLineOfSightTo(const AActor* Target, const FVector& TargetLoc) const;

	/** Scoring function (angle + distance) */
	float ComputeScore(const FVector& CamLoc, const FVector& CamForward, const FVector& TargetLoc) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	float SearchRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	float MaxLockDistance = 2500.f;

	/** Targets must be within this cone in front of camera (degrees). */
	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	float MaxAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	bool bRequireLineOfSight = true;

	/** Weights for score tuning */
	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	float AngleWeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="LockOn|Config")
	float DistanceWeight = 1.0f;

private:
	TWeakObjectPtr<AActor> CurrentTargetActor;
};
