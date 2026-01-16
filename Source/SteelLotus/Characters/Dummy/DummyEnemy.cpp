#include "DummyEnemy.h"
#include "Components/SceneComponent.h"

ADummyEnemy::ADummyEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	LockOnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetRootComponent());
	
	LockOnPoint->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
}

FVector ADummyEnemy::GetLockOnLocation_Implementation() const
{
	return LockOnPoint ? LockOnPoint->GetComponentLocation() : GetActorLocation();
}

bool ADummyEnemy::IsLockable_Implementation() const
{
	return true;
}
