// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatGameMode.generated.h"

class ACombatStarterWeaponChoice;

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

	/** Used to assign players to different PlayerStarts in the level */
	int32 CurrentPlayerStartAssignment = 0;

	/** Prevents duplicate starter-hub spawns */
	bool bStarterChoicesSpawned = false;
};
