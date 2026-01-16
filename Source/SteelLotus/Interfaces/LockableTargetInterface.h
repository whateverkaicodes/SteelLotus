#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LockableTargetInterface.generated.h"

UINTERFACE(BlueprintType)
class STEELLOTUS_API ULockableTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class STEELLOTUS_API ILockableTargetInterface
{
	GENERATED_BODY()

public:
	/** Where lock-on should aim (usually chest/head). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="LockOn")
	FVector GetLockOnLocation() const;

	/** Optional: allow target to deny lock-on (dead, hidden, etc.) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="LockOn")
	bool IsLockable() const;
};
