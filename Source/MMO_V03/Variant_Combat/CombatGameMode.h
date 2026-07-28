// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatGameMode.generated.h"

class ACombatStarterWeaponChoice;
class AActor;
class ACombatActivationVolume;
class ACombatEnemy;
class ACombatEnemySpawner;
class ACombatEncounterSignal;

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

	/** Spawns a minimal micro-encounter after the starter sandbox */
	void SpawnStarterMicroEncounter();

protected:

	/** Determines how many local players should be spawned on game start */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 1;

	/** Optional override for the starter-weapon choice actor */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub")
	TSubclassOf<ACombatStarterWeaponChoice> StarterWeaponChoiceClass;

	/** If true, spawn the starter-weapon choice actors near the player start */
	UPROPERTY(EditDefaultsOnly, Category="Starter Weapon Hub")
	bool bSpawnStarterWeaponChoices = false;

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
	bool bSpawnGameplayValidationSandbox = false;

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

	/** If true, spawn a minimal encounter loop after the starter sandbox */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter")
	bool bSpawnStarterMicroEncounter = false;

	/** Optional override for the first micro-encounter enemy spawner */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter")
	TSubclassOf<ACombatEnemySpawner> StarterMicroEncounterSpawnerClass;

	/** Optional override for the first micro-encounter activation volume */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter")
	TSubclassOf<ACombatActivationVolume> StarterMicroEncounterActivationVolumeClass;

	/** Optional override for the checkpoint volume placed before the encounter */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter")
	TSubclassOf<AActor> StarterMicroEncounterCheckpointClass;

	/** Optional override for the first encounter enemy */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter")
	TSubclassOf<ACombatEnemy> StarterMicroEncounterEnemyClass;

	/** Distance in front of the spawn where the encounter checkpoint is placed */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter", meta=(ClampMin=0, Units="cm"))
	float StarterMicroEncounterForwardOffset = 2200.0f;

	/** Additional distance to place the activation volume after the checkpoint */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter", meta=(ClampMin=0, Units="cm"))
	float StarterMicroEncounterActivationOffset = 260.0f;

	/** Additional distance to place the enemy spawner after the activation volume */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter", meta=(ClampMin=0, Units="cm"))
	float StarterMicroEncounterEnemyOffset = 650.0f;

	/** Lateral distance used to place the completion beacon beside the encounter */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter", meta=(ClampMin=0, Units="cm"))
	float StarterMicroEncounterSignalSideOffset = 340.0f;

	/** Small lift applied to encounter actors to avoid floor clipping on spawn */
	UPROPERTY(EditDefaultsOnly, Category="Micro Encounter", meta=(Units="cm"))
	float StarterMicroEncounterHeightOffset = 10.0f;

	/** Used to assign players to different PlayerStarts in the level */
	int32 CurrentPlayerStartAssignment = 0;

	/** Prevents duplicate starter-hub spawns */
	bool bStarterChoicesSpawned = false;

	/** Prevents duplicate validation sandbox spawns */
	bool bGameplayValidationSandboxSpawned = false;

	/** Prevents duplicate micro-encounter spawns */
	bool bStarterMicroEncounterSpawned = false;

	/** Keeps track of the spawned encounter signal for this map session */
	UPROPERTY(Transient)
	TObjectPtr<ACombatEncounterSignal> SpawnedStarterMicroEncounterSignal;
};
