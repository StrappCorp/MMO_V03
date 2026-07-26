#include "CombatPlayerState.h"

#include "Net/UnrealNetwork.h"

ACombatPlayerState::ACombatPlayerState()
{
	bReplicates = true;
}

void ACombatPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatPlayerState, bHasClaimedInitialStarterWeapon);
	DOREPLIFETIME(ACombatPlayerState, ClaimedStarterWeaponDefinitionId);
	DOREPLIFETIME(ACombatPlayerState, EquippedStarterWeapon);
}

bool ACombatPlayerState::ClaimInitialStarterWeapon(const FString& DefinitionId, FString& OutSuccessMessage, FString& OutFailureReason)
{
	OutSuccessMessage.Reset();
	OutFailureReason.Reset();

	if (!HasAuthority())
	{
		OutFailureReason = TEXT("Le choix d’arme initiale doit être validé côté serveur.");
		return false;
	}

	if (bHasClaimedInitialStarterWeapon)
	{
		OutFailureReason = TEXT("L’arme initiale a déjà été choisie pour ce personnage.");
		return false;
	}

	const FString NormalizedDefinitionId = CombatStarterWeapon::NormalizeDefinitionId(DefinitionId);
	if (!CombatStarterWeapon::IsValidSelectableDefinitionId(NormalizedDefinitionId))
	{
		OutFailureReason = TEXT("Cette option d’arme initiale n’est pas autorisée.");
		return false;
	}

	const ECombatStarterWeaponType ResolvedStarterWeapon = CombatStarterWeapon::DefinitionIdToWeaponType(NormalizedDefinitionId);
	if (ResolvedStarterWeapon == ECombatStarterWeaponType::Unarmed)
	{
		OutFailureReason = TEXT("Impossible de résoudre l’arme initiale demandée.");
		return false;
	}

	bHasClaimedInitialStarterWeapon = true;
	ClaimedStarterWeaponDefinitionId = NormalizedDefinitionId;
	EquippedStarterWeapon = ResolvedStarterWeapon;
	ForceNetUpdate();
	BroadcastStarterWeaponChanged();

	OutSuccessMessage = FString::Printf(
		TEXT("Arme initiale choisie : %s. Elle est maintenant équipée."),
		*CombatStarterWeapon::GetDisplayText(EquippedStarterWeapon).ToString());
	return true;
}

void ACombatPlayerState::OnRep_StarterWeaponState()
{
	BroadcastStarterWeaponChanged();
}

void ACombatPlayerState::BroadcastStarterWeaponChanged()
{
	OnStarterWeaponChanged.Broadcast();
}
