// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatGameMode.h"
#include "AI/CombatEnemy.h"
#include "AI/CombatEnemySpawner.h"
#include "CombatPlayerState.h"
#include "Gameplay/CombatActivationVolume.h"
#include "Gameplay/CombatEncounterSignal.h"
#include "Gameplay/CombatStarterWeaponChoice.h"
#include "CombatStarterWeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace
{
	TSubclassOf<AActor> ResolveValidationActorClass(TSubclassOf<AActor> OverrideClass, const TCHAR* FallbackPath)
	{
		if (OverrideClass)
		{
			return OverrideClass;
		}

		return LoadClass<AActor>(nullptr, FallbackPath);
	}

	template <typename TClass>
	TSubclassOf<TClass> ResolveValidationSubclass(TSubclassOf<TClass> OverrideClass, const TCHAR* FallbackPath)
	{
		if (OverrideClass)
		{
			return OverrideClass;
		}

		return LoadClass<TClass>(nullptr, FallbackPath);
	}
}

ACombatGameMode::ACombatGameMode()
{
	PlayerStateClass = ACombatPlayerState::StaticClass();
}

void ACombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnStarterWeaponChoices();
	SpawnGameplayValidationSandbox();
	SpawnStarterMicroEncounter();

	// create each additional local player.
	// Player 0 will be created automatically as part of regular game init
	for (int32 i = 2; i <= NumberOfLocalPlayers; ++i)
	{
		UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
	}
}

AActor* ACombatGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// build the current player tag
	FName PlayerTag = FName(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));

	// find all player starts with the matching player tag
	TArray<AActor*> PlayerStarts;

	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	// increment the player start assignment index
	++CurrentPlayerStartAssignment;

	// if no PlayerStarts were found, default to all PlayerStarts instead
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	// have we found at least one PlayerStart?
	if (!PlayerStarts.IsEmpty())
	{
		return PlayerStarts[ FMath::RandRange(0, PlayerStarts.Num() - 1) ];
	}

	// no PlayerStarts in the level
	return nullptr;
}

void ACombatGameMode::SpawnStarterWeaponChoices()
{
	if (bStarterChoicesSpawned || !GetWorld())
	{
		return;
	}

	bStarterChoicesSpawned = true;

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName(TEXT("Player0")), PlayerStarts);
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	const AActor* AnchorActor = PlayerStarts.IsEmpty() ? nullptr : PlayerStarts[0];
	const FVector AnchorLocation = AnchorActor ? AnchorActor->GetActorLocation() : FVector::ZeroVector;
	const FRotator AnchorRotation = AnchorActor ? AnchorActor->GetActorRotation() : FRotator::ZeroRotator;
	const FVector ForwardVector = AnchorRotation.Vector();
	const FVector RightVector = FRotationMatrix(AnchorRotation).GetUnitAxis(EAxis::Y);
	const FVector SpawnBaseLocation = AnchorLocation + (ForwardVector * StarterChoiceForwardOffset) + FVector(0.0f, 0.0f, StarterChoiceHeightOffset);
	const FRotator SpawnRotation = AnchorRotation + FRotator(0.0f, 180.0f, 0.0f);

	TSubclassOf<ACombatStarterWeaponChoice> ChoiceClassToSpawn = StarterWeaponChoiceClass;
	if (!ChoiceClassToSpawn)
	{
		ChoiceClassToSpawn = ACombatStarterWeaponChoice::StaticClass();
	}

	const ECombatStarterWeaponType StarterChoices[] = {
		ECombatStarterWeaponType::Sword,
		ECombatStarterWeaponType::Dagger,
		ECombatStarterWeaponType::ChannelingOrb
	};

	for (int32 ChoiceIndex = 0; ChoiceIndex < UE_ARRAY_COUNT(StarterChoices); ++ChoiceIndex)
	{
		const float LateralOffset = (static_cast<float>(ChoiceIndex) - 1.0f) * StarterChoiceSideSpacing;
		const FVector SpawnLocation = SpawnBaseLocation + (RightVector * LateralOffset);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		if (ACombatStarterWeaponChoice* ChoiceActor = GetWorld()->SpawnActor<ACombatStarterWeaponChoice>(ChoiceClassToSpawn, SpawnLocation, SpawnRotation, SpawnParameters))
		{
			ChoiceActor->SetStarterWeaponType(StarterChoices[ChoiceIndex]);
		}
	}
}

void ACombatGameMode::SpawnStarterMicroEncounter()
{
	if (bStarterMicroEncounterSpawned || !bSpawnStarterMicroEncounter || !GetWorld())
	{
		return;
	}

	bStarterMicroEncounterSpawned = true;

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName(TEXT("Player0")), PlayerStarts);
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	const AActor* AnchorActor = PlayerStarts.IsEmpty() ? nullptr : PlayerStarts[0];
	const FVector AnchorLocation = AnchorActor ? AnchorActor->GetActorLocation() : FVector::ZeroVector;
	const FRotator AnchorRotation = AnchorActor ? AnchorActor->GetActorRotation() : FRotator::ZeroRotator;
	const FVector ForwardVector = AnchorRotation.Vector();
	const FVector RightVector = FRotationMatrix(AnchorRotation).GetUnitAxis(EAxis::Y);
	const FVector EncounterBaseLocation = AnchorLocation
		+ (ForwardVector * StarterMicroEncounterForwardOffset)
		+ FVector(0.0f, 0.0f, StarterMicroEncounterHeightOffset);

	const TSubclassOf<ACombatEnemySpawner> SpawnerClass = ResolveValidationSubclass<ACombatEnemySpawner>(
		StarterMicroEncounterSpawnerClass,
		TEXT("/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemySpawner.BP_CombatEnemySpawner_C"));
	const TSubclassOf<ACombatActivationVolume> ActivationVolumeClass = ResolveValidationSubclass<ACombatActivationVolume>(
		StarterMicroEncounterActivationVolumeClass,
		TEXT("/Game/Variant_Combat/Blueprints/Interactables/BP_CombatActivationVolume.BP_CombatActivationVolume_C"));
	const TSubclassOf<AActor> CheckpointClass = ResolveValidationActorClass(
		StarterMicroEncounterCheckpointClass,
		TEXT("/Game/Variant_Combat/Blueprints/Interactables/BP_CombatCheckpointVolume.BP_CombatCheckpointVolume_C"));
	const TSubclassOf<ACombatEnemy> EnemyClass = ResolveValidationSubclass<ACombatEnemy>(
		StarterMicroEncounterEnemyClass,
		TEXT("/Game/Variant_Combat/Blueprints/AI/BP_CombatEnemy.BP_CombatEnemy_C"));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (CheckpointClass)
	{
		if (AActor* Checkpoint = GetWorld()->SpawnActor<AActor>(CheckpointClass, EncounterBaseLocation, AnchorRotation, SpawnParameters))
		{
			Checkpoint->Tags.AddUnique(FName(TEXT("CombatMicroEncounter")));
		}
	}

	SpawnedStarterMicroEncounterSignal = GetWorld()->SpawnActorDeferred<ACombatEncounterSignal>(
		ACombatEncounterSignal::StaticClass(),
		FTransform(AnchorRotation, EncounterBaseLocation + (RightVector * StarterMicroEncounterSignalSideOffset)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (SpawnedStarterMicroEncounterSignal)
	{
		SpawnedStarterMicroEncounterSignal->SetInactiveLabel(FText::FromString(TEXT("FIGHT")));
		SpawnedStarterMicroEncounterSignal->SetActiveLabel(FText::FromString(TEXT("CLEAR")));
		UGameplayStatics::FinishSpawningActor(SpawnedStarterMicroEncounterSignal, FTransform(AnchorRotation, EncounterBaseLocation + (RightVector * StarterMicroEncounterSignalSideOffset)));
		SpawnedStarterMicroEncounterSignal->Tags.AddUnique(FName(TEXT("CombatMicroEncounter")));
	}

	ACombatEnemySpawner* EncounterSpawner = nullptr;
	if (SpawnerClass)
	{
		const FTransform SpawnerTransform(
			AnchorRotation,
			EncounterBaseLocation + (ForwardVector * (StarterMicroEncounterActivationOffset + StarterMicroEncounterEnemyOffset)));

		EncounterSpawner = GetWorld()->SpawnActorDeferred<ACombatEnemySpawner>(
			SpawnerClass,
			SpawnerTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (EncounterSpawner)
		{
			EncounterSpawner->SetShouldSpawnEnemiesImmediately(false);
			EncounterSpawner->SetInitialSpawnDelay(0.0f);
			EncounterSpawner->SetRespawnDelay(1.0f);
			EncounterSpawner->SetActivationDelay(0.2f);
			EncounterSpawner->SetSpawnCount(1);
			EncounterSpawner->SetAllowReactivationAfterDepletion(true);
			EncounterSpawner->SetEnemyClass(EnemyClass);

			if (SpawnedStarterMicroEncounterSignal)
			{
				EncounterSpawner->AddActorToDeactivateWhenActivated(SpawnedStarterMicroEncounterSignal);
				EncounterSpawner->AddActorToActivateWhenDepleted(SpawnedStarterMicroEncounterSignal);
			}

			UGameplayStatics::FinishSpawningActor(EncounterSpawner, SpawnerTransform);
			EncounterSpawner->Tags.AddUnique(FName(TEXT("CombatMicroEncounter")));
		}
	}

	if (ActivationVolumeClass)
	{
		const FTransform ActivationTransform(
			AnchorRotation,
			EncounterBaseLocation + (ForwardVector * StarterMicroEncounterActivationOffset));

		ACombatActivationVolume* SpawnedActivationActor = GetWorld()->SpawnActorDeferred<ACombatActivationVolume>(
			ActivationVolumeClass,
			ActivationTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (SpawnedActivationActor)
		{
			if (EncounterSpawner)
			{
				SpawnedActivationActor->AddActorToActivate(EncounterSpawner);
			}

			UGameplayStatics::FinishSpawningActor(SpawnedActivationActor, ActivationTransform);
			SpawnedActivationActor->Tags.AddUnique(FName(TEXT("CombatMicroEncounter")));
		}
	}
}

void ACombatGameMode::SpawnGameplayValidationSandbox()
{
	if (bGameplayValidationSandboxSpawned || !bSpawnGameplayValidationSandbox || !GetWorld())
	{
		return;
	}

	bGameplayValidationSandboxSpawned = true;

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName(TEXT("Player0")), PlayerStarts);
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	const AActor* AnchorActor = PlayerStarts.IsEmpty() ? nullptr : PlayerStarts[0];
	const FVector AnchorLocation = AnchorActor ? AnchorActor->GetActorLocation() : FVector::ZeroVector;
	const FRotator AnchorRotation = AnchorActor ? AnchorActor->GetActorRotation() : FRotator::ZeroRotator;
	const FVector ForwardVector = AnchorRotation.Vector();
	const FVector RightVector = FRotationMatrix(AnchorRotation).GetUnitAxis(EAxis::Y);
	const FVector SpawnBaseLocation = AnchorLocation
		+ (ForwardVector * ValidationSandboxForwardOffset)
		+ FVector(0.0f, 0.0f, ValidationSandboxHeightOffset);

	const TSubclassOf<AActor> BreakableClass = ResolveValidationActorClass(
		ValidationBreakableClass,
		TEXT("/Game/Variant_Combat/Blueprints/Interactables/BP_CombatDamageableBox.BP_CombatDamageableBox_C"));
	const TSubclassOf<AActor> DummyClass = ResolveValidationActorClass(
		ValidationDummyClass,
		TEXT("/Game/Variant_Combat/Blueprints/Interactables/BP_CombatDummy.BP_CombatDummy_C"));
	const TSubclassOf<AActor> HazardClass = ResolveValidationActorClass(
		ValidationHazardClass,
		TEXT("/Game/Variant_Combat/Blueprints/Interactables/BP_CombatLavaFloor.BP_CombatLavaFloor_C"));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (BreakableClass)
	{
		if (AActor* Breakable = GetWorld()->SpawnActor<AActor>(
			BreakableClass,
			SpawnBaseLocation - (RightVector * ValidationSandboxSideOffset),
			AnchorRotation,
			SpawnParameters))
		{
			Breakable->Tags.AddUnique(FName(TEXT("CombatValidationSandbox")));
		}
	}

	if (DummyClass)
	{
		if (AActor* Dummy = GetWorld()->SpawnActor<AActor>(
			DummyClass,
			SpawnBaseLocation + (RightVector * ValidationSandboxSideOffset),
			AnchorRotation,
			SpawnParameters))
		{
			Dummy->Tags.AddUnique(FName(TEXT("CombatValidationSandbox")));
		}
	}

	if (HazardClass)
	{
		const FVector HazardLocation = SpawnBaseLocation + (ForwardVector * ValidationHazardForwardOffset);
		if (AActor* Hazard = GetWorld()->SpawnActor<AActor>(HazardClass, HazardLocation, AnchorRotation, SpawnParameters))
		{
			Hazard->Tags.AddUnique(FName(TEXT("CombatValidationSandbox")));
		}
	}
}
