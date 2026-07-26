// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatGameMode.generated.h"

class ACombatStarterWeaponChoice;
class AActor;

/**
 *  Simple GameMode for a third person combat game
 */
UCLASS(abstract)
class ACombatGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	ACombatGameMode();

protected:

	/** Initialization */
	virtual void BeginPlay() override;

	/** Assigns a PlayerStart to a specific player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Spawns the minimal starter-weapon hub near the combat spawn */
	void SpawnStarterWeaponChoices();

	/** Spawns a tiny validation sandbox so the combat loop can be tested immediately in PIE */
	void SpawnGameplayValidationSandbox();

protected:

	/** Determines how many local players should be spawned on game start */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 1;

	/** Optional override for the starter-weapon choice actor */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub")
	TSubclassOf<ACombatStarterWeaponChoice> StarterWeaponChoiceClass;

	/** Distance in front of the spawn where starter choices will appear */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub", meta=(ClampMin=0, Units="cm"))
	float StarterChoiceForwardOffset = 450.0f;

	/** Horizontal spacing between starter choices */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub", meta=(ClampMin=0, Units="cm"))
	float StarterChoiceSideSpacing = 220.0f;

	/** Small lift applied to keep the spawned hub above the floor */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub", meta=(Units="cm"))
	float StarterChoiceHeightOffset = 15.0f;

	/** If true, spawn a lightweight combat sandbox near the player start for end-to-end gameplay validation */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox")
	bool bSpawnGameplayValidationSandbox = true;

	/** Optional override for the breakable actor used in the validation sandbox */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox")
	TSubclassOf<AActor> ValidationBreakableClass;

	/** Optional override for the training dummy used in the validation sandbox */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox")
	TSubclassOf<AActor> ValidationDummyClass;

	/** Optional override for the hazard used to validate player death / respawn */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox")
	TSubclassOf<AActor> ValidationHazardClass;

	/** Distance in front of the spawn where the validation sandbox begins */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox", meta=(ClampMin=0, Units="cm"))
	float ValidationSandboxForwardOffset = 900.0f;

	/** Horizontal spacing used between the spawned validation actors */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox", meta=(ClampMin=0, Units="cm"))
	float ValidationSandboxSideOffset = 220.0f;

	/** Distance between the breakable/dummy line and the hazard */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox", meta=(ClampMin=0, Units="cm"))
	float ValidationHazardForwardOffset = 300.0f;

	/** Small lift applied to keep the validation actors from clipping the ground on spawn */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Validation Sandbox", meta=(Units="cm"))
	float ValidationSandboxHeightOffset = 10.0f;

	/** Used to assign players to different PlayerStarts in the level */
	int32 CurrentPlayerStartAssignment = 0;

	/** Prevents duplicate starter-hub spawns */
	bool bStarterChoicesSpawned = false;

	/** Prevents duplicate validation sandbox spawns */
	bool bGameplayValidationSandboxSpawned = false;
};
