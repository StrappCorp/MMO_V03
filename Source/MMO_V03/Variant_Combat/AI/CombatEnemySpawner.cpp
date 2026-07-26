// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatEnemySpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "CombatEnemy.h"
#include "CombatActivatable.h"

ACombatEnemySpawner::ACombatEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the reference spawn capsule
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

void ACombatEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	RemainingSpawnCount = FMath::Max(SpawnCount, 0);
	
	// should we spawn an enemy right away?
	if (bShouldSpawnEnemiesImmediately)
	{
		// schedule the first enemy spawn
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACombatEnemySpawner::SpawnEnemy, InitialSpawnDelay);
	}

}

void ACombatEnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the spawn timer
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}

void ACombatEnemySpawner::SetEnemyClass(TSubclassOf<ACombatEnemy> InEnemyClass)
{
	EnemyClass = InEnemyClass;
}

void ACombatEnemySpawner::SetShouldSpawnEnemiesImmediately(bool bInShouldSpawnEnemiesImmediately)
{
	bShouldSpawnEnemiesImmediately = bInShouldSpawnEnemiesImmediately;
}

void ACombatEnemySpawner::SetInitialSpawnDelay(float InInitialSpawnDelay)
{
	InitialSpawnDelay = FMath::Max(0.0f, InInitialSpawnDelay);
}

void ACombatEnemySpawner::SetSpawnCount(int32 InSpawnCount)
{
	SpawnCount = FMath::Max(0, InSpawnCount);
	RemainingSpawnCount = SpawnCount;
}

void ACombatEnemySpawner::SetRespawnDelay(float InRespawnDelay)
{
	RespawnDelay = FMath::Max(0.0f, InRespawnDelay);
}

void ACombatEnemySpawner::SetActivationDelay(float InActivationDelay)
{
	ActivationDelay = FMath::Max(0.0f, InActivationDelay);
}

void ACombatEnemySpawner::SetAllowReactivationAfterDepletion(bool bInAllowReactivationAfterDepletion)
{
	bAllowReactivationAfterDepletion = bInAllowReactivationAfterDepletion;
}

void ACombatEnemySpawner::SetActorsToActivateWhenDepleted(const TArray<AActor*>& InActors)
{
	ActorsToActivateWhenDepleted = InActors;
}

void ACombatEnemySpawner::AddActorToActivateWhenDepleted(AActor* InActor)
{
	if (InActor)
	{
		ActorsToActivateWhenDepleted.AddUnique(InActor);
	}
}

void ACombatEnemySpawner::SetActorsToDeactivateWhenActivated(const TArray<AActor*>& InActors)
{
	ActorsToDeactivateWhenActivated = InActors;
}

void ACombatEnemySpawner::AddActorToDeactivateWhenActivated(AActor* InActor)
{
	if (InActor)
	{
		ActorsToDeactivateWhenActivated.AddUnique(InActor);
	}
}

void ACombatEnemySpawner::SpawnEnemy()
{
	if (RemainingSpawnCount <= 0)
	{
		return;
	}

	if (IsValid(ActiveEnemy))
	{
		return;
	}

	// ensure the enemy class is valid
	if (IsValid(EnemyClass))
	{
		// spawn the enemy at the reference capsule's transform
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ACombatEnemy* SpawnedEnemy = GetWorld()->SpawnActor<ACombatEnemy>(EnemyClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

		// was the enemy successfully created?
		if (SpawnedEnemy)
		{
			ActiveEnemy = SpawnedEnemy;

			// subscribe to the death delegate
			SpawnedEnemy->OnEnemyDied.AddDynamic(this, &ACombatEnemySpawner::OnEnemyDied);
		}
	}
}

void ACombatEnemySpawner::OnEnemyDied()
{
	ActiveEnemy = nullptr;

	// decrease the spawn counter
	--RemainingSpawnCount;

	// is this the last enemy we should spawn?
	if (RemainingSpawnCount <= 0)
	{
		// schedule the activation on depleted message
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACombatEnemySpawner::SpawnerDepleted, ActivationDelay);
		return;
	}

	// schedule the next enemy spawn
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &ACombatEnemySpawner::SpawnEnemy, RespawnDelay);
}

void ACombatEnemySpawner::SpawnerDepleted()
{
	NotifyActors(ActorsToActivateWhenDepleted, true);
}

void ACombatEnemySpawner::NotifyActors(const TArray<AActor*>& Actors, bool bActivate)
{
	for (AActor* CurrentActor : Actors)
	{
		if (ICombatActivatable* CombatActivatable = Cast<ICombatActivatable>(CurrentActor))
		{
			if (bActivate)
			{
				CombatActivatable->ActivateInteraction(this);
			}
			else
			{
				CombatActivatable->DeactivateInteraction(this);
			}
		}
	}
}

void ACombatEnemySpawner::ToggleInteraction(AActor* ActivationInstigator)
{
	// stub
}

void ACombatEnemySpawner::ActivateInteraction(AActor* ActivationInstigator)
{
	// only manual/deferred spawners should react to activation requests
	if (bShouldSpawnEnemiesImmediately)
	{
		return;
	}

	if (IsValid(ActiveEnemy) || GetWorld()->GetTimerManager().IsTimerActive(SpawnTimer))
	{
		return;
	}

	if (RemainingSpawnCount <= 0)
	{
		if (!bAllowReactivationAfterDepletion)
		{
			return;
		}

		RemainingSpawnCount = FMath::Max(SpawnCount, 0);
	}

	if (RemainingSpawnCount <= 0)
	{
		return;
	}

	// raise the activation flag
	bHasBeenActivated = true;

	NotifyActors(ActorsToDeactivateWhenActivated, false);

	// spawn the first enemy
	SpawnEnemy();
}

void ACombatEnemySpawner::DeactivateInteraction(AActor* ActivationInstigator)
{
	// stub
}
