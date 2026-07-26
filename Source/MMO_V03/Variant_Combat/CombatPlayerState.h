#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CombatStarterWeaponTypes.h"
#include "CombatPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCombatStarterWeaponChangedSignature);

/**
 * Minimal replicated starter-weapon state used by the combat hub vertical slice.
 */
UCLASS()
class ACombatPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACombatPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category="Starter Weapon")
	FCombatStarterWeaponChangedSignature OnStarterWeaponChanged;

	UFUNCTION(BlueprintPure, Category="Starter Weapon")
	bool HasClaimedInitialStarterWeapon() const { return bHasClaimedInitialStarterWeapon; }

	UFUNCTION(BlueprintPure, Category="Starter Weapon")
	FString GetClaimedStarterWeaponDefinitionId() const { return ClaimedStarterWeaponDefinitionId; }

	UFUNCTION(BlueprintPure, Category="Starter Weapon")
	ECombatStarterWeaponType GetEquippedStarterWeapon() const { return EquippedStarterWeapon; }

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Starter Weapon")
	bool ClaimInitialStarterWeapon(const FString& DefinitionId, FString& OutSuccessMessage, FString& OutFailureReason);

protected:
	UPROPERTY(ReplicatedUsing=OnRep_StarterWeaponState, VisibleAnywhere, Category="Starter Weapon")
	bool bHasClaimedInitialStarterWeapon = false;

	UPROPERTY(ReplicatedUsing=OnRep_StarterWeaponState, VisibleAnywhere, Category="Starter Weapon")
	FString ClaimedStarterWeaponDefinitionId;

	UPROPERTY(ReplicatedUsing=OnRep_StarterWeaponState, VisibleAnywhere, Category="Starter Weapon")
	ECombatStarterWeaponType EquippedStarterWeapon = ECombatStarterWeaponType::Unarmed;

	UFUNCTION()
	void OnRep_StarterWeaponState();

	void BroadcastStarterWeaponChanged();
};
