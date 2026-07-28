#pragma once

#include "CoreMinimal.h"
#include "CombatStarterWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatStarterWeaponType : uint8
{
	Unarmed UMETA(DisplayName="Mains nues"),
	Sword UMETA(DisplayName="Épée"),
	Dagger UMETA(DisplayName="Dague"),
	ChannelingOrb UMETA(DisplayName="Baguette magique")
};

namespace CombatStarterWeapon
{
	inline FString NormalizeDefinitionId(const FString& DefinitionId)
	{
		return DefinitionId.TrimStartAndEnd();
	}

	inline bool IsValidSelectableDefinitionId(const FString& DefinitionId)
	{
		const FString NormalizedDefinitionId = NormalizeDefinitionId(DefinitionId);
		return NormalizedDefinitionId.Equals(TEXT("tier0-sword"), ESearchCase::IgnoreCase)
			|| NormalizedDefinitionId.Equals(TEXT("tier0-dagger"), ESearchCase::IgnoreCase)
			|| NormalizedDefinitionId.Equals(TEXT("starter-channeling-orb"), ESearchCase::IgnoreCase);
	}

	inline ECombatStarterWeaponType DefinitionIdToWeaponType(const FString& DefinitionId)
	{
		const FString NormalizedDefinitionId = NormalizeDefinitionId(DefinitionId);
		if (NormalizedDefinitionId.Equals(TEXT("tier0-sword"), ESearchCase::IgnoreCase))
		{
			return ECombatStarterWeaponType::Sword;
		}

		if (NormalizedDefinitionId.Equals(TEXT("tier0-dagger"), ESearchCase::IgnoreCase))
		{
			return ECombatStarterWeaponType::Dagger;
		}

		if (NormalizedDefinitionId.Equals(TEXT("starter-channeling-orb"), ESearchCase::IgnoreCase))
		{
			return ECombatStarterWeaponType::ChannelingOrb;
		}

		return ECombatStarterWeaponType::Unarmed;
	}

	inline FString WeaponTypeToDefinitionId(ECombatStarterWeaponType WeaponType)
	{
		switch (WeaponType)
		{
		case ECombatStarterWeaponType::Sword:
			return TEXT("tier0-sword");
		case ECombatStarterWeaponType::Dagger:
			return TEXT("tier0-dagger");
		case ECombatStarterWeaponType::ChannelingOrb:
			return TEXT("starter-channeling-orb");
		default:
			return FString();
		}
	}

	inline FText GetDisplayText(ECombatStarterWeaponType WeaponType)
	{
		switch (WeaponType)
		{
		case ECombatStarterWeaponType::Sword:
			return FText::FromString(TEXT("Épée"));
		case ECombatStarterWeaponType::Dagger:
			return FText::FromString(TEXT("Dague"));
		case ECombatStarterWeaponType::ChannelingOrb:
			return FText::FromString(TEXT("Baguette magique"));
		default:
			return FText::FromString(TEXT("Mains nues"));
		}
	}
}
