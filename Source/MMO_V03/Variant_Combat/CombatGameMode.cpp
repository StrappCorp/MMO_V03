// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatGameMode.h"
#include "CombatPlayerState.h"
#include "Gameplay/CombatStarterWeaponChoice.h"
#include "CombatStarterWeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

ACombatGameMode::ACombatGameMode()
{
	PlayerStateClass = ACombatPlayerState::StaticClass();
}

void ACombatGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnStarterWeaponChoices();

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

	const TSubclassOf<ACombatStarterWeaponChoice> ChoiceClassToSpawn = StarterWeaponChoiceClass
		? StarterWeaponChoiceClass
		: ACombatStarterWeaponChoice::StaticClass();

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
