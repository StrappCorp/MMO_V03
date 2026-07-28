// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "CombatCharacter.h"
#include "CombatPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "MMO_V03.h"
#include "UI/CombatDebugTextPanelWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	constexpr int32 CombatDebugWindowZOrder = 32;
	const FVector2D CombatInventoryWindowPosition(24.0f, 24.0f);
	const FVector2D CombatSkillsWindowPosition(520.0f, 24.0f);

	FString GetStarterWeaponLabel(const ACombatPlayerState* CombatPlayerState)
	{
		if (!CombatPlayerState || !CombatPlayerState->HasClaimedInitialStarterWeapon())
		{
			return TEXT("(vide)");
		}

		return CombatStarterWeapon::GetDisplayText(CombatPlayerState->GetEquippedStarterWeapon()).ToString();
	}

	int32 GetDisplayedSkillLevel(const ACombatPlayerState* CombatPlayerState, ECombatStarterWeaponType WeaponType)
	{
		return CombatPlayerState
			&& CombatPlayerState->HasClaimedInitialStarterWeapon()
			&& CombatPlayerState->GetEquippedStarterWeapon() == WeaponType
			? 1
			: 0;
	}
}

void ACombatPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ACombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ACombatPlayerController::HandleToggleCombatInventoryInput);
			InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ACombatPlayerController::HandleToggleCombatSkillsInput);
			InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACombatPlayerController::HandleCloseCombatInterfaceInput);
		}
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogMMO_V03, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		// subscribe to the pawn's OnDestroyed delegate
		InPawn->OnDestroyed.AddDynamic(this, &ACombatPlayerController::OnPawnDestroyed);
	}

	if (bIsCombatInventoryOpen)
	{
		RefreshCombatInventoryWidget();
	}

	if (bIsCombatSkillsOpen)
	{
		RefreshCombatSkillsWidget();
	}
}

void ACombatPlayerController::SetRespawnTransform(const FTransform& NewRespawn)
{
	// save the new respawn transform
	RespawnTransform = NewRespawn;
}

void ACombatPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// spawn a new character at the respawn transform
	if (ACombatCharacter* RespawnedCharacter = GetWorld()->SpawnActor<ACombatCharacter>(CharacterClass, RespawnTransform))
	{
		// possess the character
		Possess(RespawnedCharacter);
	}
}

bool ACombatPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ACombatPlayerController::HandleToggleCombatInventoryInput()
{
	ToggleCombatInventoryWidget();
}

void ACombatPlayerController::HandleToggleCombatSkillsInput()
{
	ToggleCombatSkillsWidget();
}

void ACombatPlayerController::HandleCloseCombatInterfaceInput()
{
	if (!bIsCombatInventoryOpen && !bIsCombatSkillsOpen)
	{
		return;
	}

	CloseCombatInventoryWidget();
	CloseCombatSkillsWidget();
}

void ACombatPlayerController::EnsureCombatInventoryWidget()
{
	if (!IsLocalPlayerController() || CombatInventoryWidget)
	{
		return;
	}

	CombatInventoryWidget = CreateWidget<UCombatDebugTextPanelWidget>(this, UCombatDebugTextPanelWidget::StaticClass());
	if (CombatInventoryWidget)
	{
		CombatInventoryWidget->SetPanelPosition(CombatInventoryWindowPosition);
		CombatInventoryWidget->AddToPlayerScreen(CombatDebugWindowZOrder);
		CombatInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ACombatPlayerController::EnsureCombatSkillsWidget()
{
	if (!IsLocalPlayerController() || CombatSkillsWidget)
	{
		return;
	}

	CombatSkillsWidget = CreateWidget<UCombatDebugTextPanelWidget>(this, UCombatDebugTextPanelWidget::StaticClass());
	if (CombatSkillsWidget)
	{
		CombatSkillsWidget->SetPanelPosition(CombatSkillsWindowPosition);
		CombatSkillsWidget->AddToPlayerScreen(CombatDebugWindowZOrder);
		CombatSkillsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ACombatPlayerController::RefreshCombatInventoryWidget()
{
	EnsureCombatInventoryWidget();
	if (!CombatInventoryWidget)
	{
		return;
	}

	CombatInventoryWidget->SetPanelText(
		TEXT("INVENTAIRE + ÉQUIPEMENT"),
		BuildCombatInventoryBodyText(),
		TEXT("I ouvrir/fermer · Echap fermer"));
}

void ACombatPlayerController::RefreshCombatSkillsWidget()
{
	EnsureCombatSkillsWidget();
	if (!CombatSkillsWidget)
	{
		return;
	}

	CombatSkillsWidget->SetPanelText(
		TEXT("COMPÉTENCES"),
		BuildCombatSkillsBodyText(),
		TEXT("K ouvrir/fermer · Echap fermer"));
}

void ACombatPlayerController::OpenCombatInventoryWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	CloseCombatSkillsWidget();
	EnsureCombatInventoryWidget();
	if (!CombatInventoryWidget)
	{
		return;
	}

	RefreshCombatInventoryWidget();
	CombatInventoryWidget->SetVisibility(ESlateVisibility::Visible);
	bIsCombatInventoryOpen = true;
	UpdateDebugWidgetInputMode();
}

void ACombatPlayerController::CloseCombatInventoryWidget()
{
	if (!bIsCombatInventoryOpen)
	{
		return;
	}

	bIsCombatInventoryOpen = false;
	if (CombatInventoryWidget)
	{
		CombatInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateDebugWidgetInputMode();
}

void ACombatPlayerController::ToggleCombatInventoryWidget()
{
	if (bIsCombatInventoryOpen)
	{
		CloseCombatInventoryWidget();
		return;
	}

	OpenCombatInventoryWidget();
}

void ACombatPlayerController::OpenCombatSkillsWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	CloseCombatInventoryWidget();
	EnsureCombatSkillsWidget();
	if (!CombatSkillsWidget)
	{
		return;
	}

	RefreshCombatSkillsWidget();
	CombatSkillsWidget->SetVisibility(ESlateVisibility::Visible);
	bIsCombatSkillsOpen = true;
	UpdateDebugWidgetInputMode();
}

void ACombatPlayerController::CloseCombatSkillsWidget()
{
	if (!bIsCombatSkillsOpen)
	{
		return;
	}

	bIsCombatSkillsOpen = false;
	if (CombatSkillsWidget)
	{
		CombatSkillsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateDebugWidgetInputMode();
}

void ACombatPlayerController::ToggleCombatSkillsWidget()
{
	if (bIsCombatSkillsOpen)
	{
		CloseCombatSkillsWidget();
		return;
	}

	OpenCombatSkillsWidget();
}

void ACombatPlayerController::UpdateDebugWidgetInputMode()
{
	const bool bAnyDebugWidgetOpen = bIsCombatInventoryOpen || bIsCombatSkillsOpen;

	SetIgnoreMoveInput(bAnyDebugWidgetOpen);
	SetIgnoreLookInput(bAnyDebugWidgetOpen);

	if (bAnyDebugWidgetOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

ACombatCharacter* ACombatPlayerController::GetCombatCharacter() const
{
	return Cast<ACombatCharacter>(GetPawn());
}

const ACombatPlayerState* ACombatPlayerController::GetCombatPlayerState() const
{
	return GetPlayerState<ACombatPlayerState>();
}

FString ACombatPlayerController::BuildCombatInventoryBodyText() const
{
	const ACombatPlayerState* CombatPlayerState = GetCombatPlayerState();
	const ACombatCharacter* CombatCharacter = GetCombatCharacter();
	const FString EquippedWeaponLabel = GetStarterWeaponLabel(CombatPlayerState);

	TArray<FString> Lines;
	Lines.Reserve(16);
	Lines.Add(TEXT("Équipement"));
	Lines.Add(FString::Printf(TEXT("- Main droite: %s"), *EquippedWeaponLabel));
	Lines.Add(TEXT("- Main gauche: (vide)"));
	Lines.Add(TEXT("- Armure: (vide)"));
	Lines.Add(TEXT("- Bouclier: (vide)"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("Inventaire"));

	if (CombatPlayerState && CombatPlayerState->HasClaimedInitialStarterWeapon())
	{
		Lines.Add(TEXT("- 16 slots standards: vides"));
		Lines.Add(TEXT("- 4 slots sécurisés: vides"));
		Lines.Add(FString::Printf(TEXT("- Arme starter active: %s"), *EquippedWeaponLabel));
	}
	else
	{
		Lines.Add(TEXT("- Aucun objet branché pour l’instant."));
	}

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("État MMO_V03"));
	Lines.Add(TEXT("- Fenêtre native remise en place."));
	Lines.Add(TEXT("- Backend inventaire/équipement détaillé de MMOV02 encore à reconnecter."));
	if (!CombatCharacter)
	{
		Lines.Add(TEXT("- Aucun personnage de combat actuellement possédé."));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString ACombatPlayerController::BuildCombatSkillsBodyText() const
{
	const ACombatPlayerState* CombatPlayerState = GetCombatPlayerState();
	const bool bHasClaimedStarterWeapon = CombatPlayerState && CombatPlayerState->HasClaimedInitialStarterWeapon();
	const ECombatStarterWeaponType EquippedWeapon = bHasClaimedStarterWeapon
		? CombatPlayerState->GetEquippedStarterWeapon()
		: ECombatStarterWeaponType::Unarmed;

	TArray<FString> Lines;
	Lines.Reserve(12);
	Lines.Add(TEXT("Familles affichées"));
	Lines.Add(FString::Printf(
		TEXT("- Épée: niveau %d%s"),
		GetDisplayedSkillLevel(CombatPlayerState, ECombatStarterWeaponType::Sword),
		EquippedWeapon == ECombatStarterWeaponType::Sword ? TEXT(" · active") : TEXT("")));
	Lines.Add(FString::Printf(
		TEXT("- Dague: niveau %d%s"),
		GetDisplayedSkillLevel(CombatPlayerState, ECombatStarterWeaponType::Dagger),
		EquippedWeapon == ECombatStarterWeaponType::Dagger ? TEXT(" · active") : TEXT("")));
	Lines.Add(FString::Printf(
		TEXT("- Baguette magique: niveau %d%s"),
		GetDisplayedSkillLevel(CombatPlayerState, ECombatStarterWeaponType::ChannelingOrb),
		EquippedWeapon == ECombatStarterWeaponType::ChannelingOrb ? TEXT(" · active") : TEXT("")));
	Lines.Add(TEXT(""));

	if (bHasClaimedStarterWeapon)
	{
		Lines.Add(FString::Printf(TEXT("- Arme starter active: %s"), *CombatStarterWeapon::GetDisplayText(EquippedWeapon).ToString()));
	}
	else
	{
		Lines.Add(TEXT("- Aucune arme starter revendiquée pour l’instant."));
	}

	Lines.Add(TEXT("- Affichage minimal remis en place dans MMO_V03."));
	Lines.Add(TEXT("- Progression détaillée MMOV02 encore à rebrancher."));

	return FString::Join(Lines, TEXT("\n"));
}
