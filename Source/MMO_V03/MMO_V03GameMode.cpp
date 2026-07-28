// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMO_V03GameMode.h"

#include "MMO_V03Character.h"
#include "MMO_V03PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr TCHAR ArthurianHubMapToken[] = TEXT("Arthurian_Hub");
	constexpr TCHAR CombatLevelMapToken[] = TEXT("Lvl_Combat");
	constexpr TCHAR CombatPlayerControllerClassPath[] = TEXT("/Game/Variant_Combat/Blueprints/BP_CombatPlayerController.BP_CombatPlayerController_C");
	constexpr TCHAR CombatCharacterClassPath[] = TEXT("/Game/Variant_Combat/Blueprints/BP_CombatCharacter.BP_CombatCharacter_C");

	bool IsCombatMapName(const FString& MapName)
	{
		return MapName.Contains(ArthurianHubMapToken) || MapName.Contains(CombatLevelMapToken);
	}
}

AMMO_V03GameMode::AMMO_V03GameMode()
{
	PlayerControllerClass = AMMO_V03PlayerController::StaticClass();
	DefaultPawnClass = AMMO_V03Character::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonCharacterBP(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonCharacterBP.Class)
	{
		DefaultPawnClass = ThirdPersonCharacterBP.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> ThirdPersonPlayerControllerBP(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (ThirdPersonPlayerControllerBP.Class)
	{
		PlayerControllerClass = ThirdPersonPlayerControllerBP.Class;
	}
}

void AMMO_V03GameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	if (IsCombatMapName(MapName))
	{
		if (UClass* CombatPlayerControllerClass = LoadClass<APlayerController>(nullptr, CombatPlayerControllerClassPath))
		{
			PlayerControllerClass = CombatPlayerControllerClass;
		}

		if (UClass* CombatPawnClass = LoadClass<APawn>(nullptr, CombatCharacterClassPath))
		{
			DefaultPawnClass = CombatPawnClass;
		}
	}

	Super::InitGame(MapName, Options, ErrorMessage);
}
