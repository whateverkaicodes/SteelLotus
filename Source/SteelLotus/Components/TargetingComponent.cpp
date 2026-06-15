#include "TargetingComponent.h"

#include "TargetingComponent.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "SteelLotus/Interfaces/LockableTargetInterface.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // lock behavior can live in character tick later
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTargetingComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
		return;
	}

	AcquireBestTarget();
}

void UTargetingComponent::ClearLockOn()
{
	CurrentTargetActor = nullptr;
}

FVector UTargetingComponent::GetCurrentTargetLocation() const
{
	AActor* Target = CurrentTargetActor.Get();
	if (!Target) return FVector::ZeroVector;

	if (Target->GetClass()->ImplementsInterface(ULockableTargetInterface::StaticClass()))
	{
		return ILockableTargetInterface::Execute_GetLockOnLocation(Target);
	}
	return Target->GetActorLocation();
}

bool UTargetingComponent::SwitchTarget(bool bToRight)
{
	if (!IsLockedOn())
		return false;

	AActor* NewTarget = FindBestSwitchTarget(bToRight);
	if (!NewTarget)
		return false;

	CurrentTargetActor = NewTarget;

	// Debug (optional)
	DrawDebugSphere(GetWorld(), GetCurrentTargetLocation(), 12.f, 12, FColor::Yellow, false, 1.0f);

	return true;
}


bool UTargetingComponent::AcquireBestTarget()
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;

	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(Owner) ? Cast<APawn>(Owner)->GetController() : nullptr);
	if (!PC) return false;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamForward = CamRot.Vector();

	const float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxAngleDegrees));

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnOverlap), false, Owner);

	// Search pawns/characters around the player
	const FVector Center = Owner->GetActorLocation();
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);

	bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		Sphere,
		Params
	);

	if (!bHit) return false;

	float BestScore = -FLT_MAX;
	AActor* BestActor = nullptr;

	for (const FOverlapResult& Res : Overlaps)
	{
		AActor* Candidate = Res.GetActor();
		if (!Candidate || Candidate == Owner) continue;

		// Must implement interface
		if (!Candidate->GetClass()->ImplementsInterface(ULockableTargetInterface::StaticClass()))
			continue;

		if (!ILockableTargetInterface::Execute_IsLockable(Candidate))
			continue;

		const FVector TargetLoc = ILockableTargetInterface::Execute_GetLockOnLocation(Candidate);
		const FVector ToTarget = (TargetLoc - CamLoc);
		const float Dist = ToTarget.Size();

		if (Dist > MaxLockDistance) continue;

		const FVector Dir = ToTarget.GetSafeNormal();
		const float Dot = FVector::DotProduct(CamForward, Dir);
		if (Dot < MinDot) continue;

		if (bRequireLineOfSight && !HasLineOfSightTo(Candidate, TargetLoc))
			continue;

		const float Score = ComputeScore(CamLoc, CamForward, TargetLoc);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	if (BestActor)
	{
		CurrentTargetActor = BestActor;

		// Debug (temporary, no widget yet)
		DrawDebugLine(GetWorld(), Owner->GetActorLocation(), GetCurrentTargetLocation(), FColor::Green, false, 1.5f, 0, 2.f);
		DrawDebugSphere(GetWorld(), GetCurrentTargetLocation(), 12.f, 12, FColor::Green, false, 1.5f);

		return true;
	}

	return false;
}

bool UTargetingComponent::HasLineOfSightTo(const AActor* Target, const FVector& TargetLoc) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Target) return false;

	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(Owner) ? Cast<APawn>(Owner)->GetController() : nullptr);
	if (!PC) return false;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnLOS), true, Owner);
	Params.AddIgnoredActor(Target);

	// Trace against visibility
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TargetLoc, ECC_Visibility, Params);

	// If it hit something else, line of sight is blocked
	return !bBlocked;
}

float UTargetingComponent::ComputeScore(const FVector& CamLoc, const FVector& CamForward, const FVector& TargetLoc) const
{
	const FVector ToTarget = TargetLoc - CamLoc;
	const float Dist = ToTarget.Size();
	const float Dot = FVector::DotProduct(CamForward, ToTarget.GetSafeNormal());

	// Higher dot = more centered, lower dist = better
	const float DistNorm = FMath::Clamp(Dist / SearchRadius, 0.f, 1.f);

	return (AngleWeight * Dot) - (DistanceWeight * DistNorm);
}

AActor* UTargetingComponent::FindBestSwitchTarget(bool bToRight) const
{
	AActor* Owner = GetOwner();
	AActor* Current = CurrentTargetActor.Get();
	if (!Owner || !Current) return nullptr;

	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(Owner) ? Cast<APawn>(Owner)->GetController() : nullptr);
	if (!PC) return nullptr;

	// Get camera for LOS/angle scoring and screen projection
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamForward = CamRot.Vector();

	// Screen projection of current target (anchor)
	FVector2D CurrentScreen;
	const FVector CurrentLoc = GetCurrentTargetLocation();
	if (!PC->ProjectWorldLocationToScreen(CurrentLoc, CurrentScreen))
		return nullptr;

	// Overlap candidates like AcquireBestTarget
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnSwitchOverlap), false, Owner);

	const FVector Center = Owner->GetActorLocation();
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		Sphere,
		Params
	);

	if (!bHit) return nullptr;

	AActor* BestActor = nullptr;
	float BestScore = FLT_MAX; // lower is better (closest on that side)

	for (const FOverlapResult& Res : Overlaps)
	{
		AActor* Candidate = Res.GetActor();
		if (!Candidate || Candidate == Owner || Candidate == Current) continue;

		if (!Candidate->GetClass()->ImplementsInterface(ULockableTargetInterface::StaticClass()))
			continue;

		if (!ILockableTargetInterface::Execute_IsLockable(Candidate))
			continue;

		const FVector TargetLoc = ILockableTargetInterface::Execute_GetLockOnLocation(Candidate);

		// Distance / cone / LOS filters (same philosophy as AcquireBestTarget)
		const float Dist = FVector::Dist(CamLoc, TargetLoc);
		if (Dist > MaxLockDistance) continue;

		const float Dot = FVector::DotProduct(CamForward, (TargetLoc - CamLoc).GetSafeNormal());
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxAngleDegrees));
		if (Dot < MinDot) continue;

		if (bRequireLineOfSight && !HasLineOfSightTo(Candidate, TargetLoc))
			continue;

		// Screen-space side check
		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(TargetLoc, ScreenPos))
			continue;

		const float DeltaX = ScreenPos.X - CurrentScreen.X;

		// Right means positive delta X, left means negative delta X
		if (bToRight && DeltaX <= 0.f) continue;
		if (!bToRight && DeltaX >= 0.f) continue;

		// Score: pick the closest candidate on that side (prefer small delta X),
		// and keep it near same vertical level to avoid weird jumps.
		const float DeltaY = FMath::Abs(ScreenPos.Y - CurrentScreen.Y);

		const float Score = FMath::Abs(DeltaX) + (0.25f * DeltaY);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	return BestActor;
}

