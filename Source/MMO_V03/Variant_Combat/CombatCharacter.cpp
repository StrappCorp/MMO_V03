// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatCharacter.h"
#include "CombatPlayerController.h"
#include "CombatPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "CombatLifeBar.h"
#include "Engine/DamageEvents.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

ACombatCharacter::ACombatCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// bind the attack montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatCharacter::AttackMontageEnded);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	DesiredCombatCameraDistance = ClampCombatCameraDistance(DefaultCameraDistance);
	CameraBoom->TargetArmLength = DesiredCombatCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// create the life bar widget component
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	// create the minimal starter weapon visual
	EquippedStarterWeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedStarterWeaponVisual"));
	EquippedStarterWeaponVisual->SetupAttachment(GetMesh());
	EquippedStarterWeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedStarterWeaponVisual->SetCanEverAffectNavigation(false);
	EquippedStarterWeaponVisual->SetCastShadow(false);
	EquippedStarterWeaponVisual->SetVisibility(false);

	// set the player tag
	Tags.Add(FName("Player"));
}

void ACombatCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACombatCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACombatCharacter::ComboAttackPressed()
{
	// route the input
	DoComboAttackStart();
}

void ACombatCharacter::ChargedAttackPressed()
{
	// route the input
	DoChargedAttackStart();
}

void ACombatCharacter::ChargedAttackReleased()
{
	// route the input
	DoChargedAttackEnd();
}

void ACombatCharacter::ToggleCamera()
{
	// call the BP hook
	BP_ToggleCamera();
}

void ACombatCharacter::HandleWalkRunToggleReleased()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsMoveInputIgnored())
		{
			return;
		}
	}

	SetWalkModeEnabled(!bIsWalkModeEnabled);
}

void ACombatCharacter::HandleSprintPressed()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsMoveInputIgnored())
		{
			return;
		}
	}

	SetSprinting(true);
}

void ACombatCharacter::HandleSprintReleased()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsMoveInputIgnored())
		{
			return;
		}
	}

	SetSprinting(false);
}

void ACombatCharacter::SetWalkModeEnabled(bool bNewWalkModeEnabled)
{
	if (bIsWalkModeEnabled == bNewWalkModeEnabled)
	{
		return;
	}

	bIsWalkModeEnabled = bNewWalkModeEnabled;
	RefreshMovementSpeed();

	if (!HasAuthority())
	{
		ServerSetWalkModeEnabled(bNewWalkModeEnabled);
	}
}

void ACombatCharacter::SetSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;
	RefreshMovementSpeed();

	if (!HasAuthority())
	{
		ServerSetSprinting(bNewSprinting);
	}
}

void ACombatCharacter::RefreshMovementSpeed()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = GetDesiredMovementSpeed();
	}
}

float ACombatCharacter::GetDesiredMovementSpeed() const
{
	const float BaseSpeed = bIsWalkModeEnabled ? WalkSpeed : RunSpeed;
	const float BoostedSpeed = bIsWalkModeEnabled ? FastWalkSpeed : SprintSpeed;
	return bIsSprinting ? BoostedSpeed : BaseSpeed;
}

void ACombatCharacter::ServerSetWalkModeEnabled_Implementation(bool bNewWalkModeEnabled)
{
	bIsWalkModeEnabled = bNewWalkModeEnabled;
	RefreshMovementSpeed();
}

void ACombatCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	bIsSprinting = bNewSprinting;
	RefreshMovementSpeed();
}

void ACombatCharacter::ZoomCameraIn()
{
	DoCameraZoom(-CombatCameraZoomStep);
}

void ACombatCharacter::ZoomCameraOut()
{
	DoCameraZoom(CombatCameraZoomStep);
}

float ACombatCharacter::ClampCombatCameraDistance(float DesiredDistance) const
{
	const float MinDistance = FMath::Min(CombatCameraMinDistance, CombatCameraMaxDistance);
	const float MaxDistance = FMath::Max(CombatCameraMinDistance, CombatCameraMaxDistance);
	return FMath::Clamp(DesiredDistance, MinDistance, MaxDistance);
}

void ACombatCharacter::HandleStarterWeaponChanged()
{
	RefreshStarterWeaponFromPlayerState();
}

void ACombatCharacter::RefreshObservedCombatPlayerState()
{
	ACombatPlayerState* NextCombatPlayerState = GetPlayerState<ACombatPlayerState>();
	if (CachedCombatPlayerState == NextCombatPlayerState)
	{
		return;
	}

	if (CachedCombatPlayerState)
	{
		CachedCombatPlayerState->OnStarterWeaponChanged.RemoveDynamic(this, &ACombatCharacter::HandleStarterWeaponChanged);
	}

	CachedCombatPlayerState = NextCombatPlayerState;
	if (CachedCombatPlayerState)
	{
		CachedCombatPlayerState->OnStarterWeaponChanged.AddDynamic(this, &ACombatCharacter::HandleStarterWeaponChanged);
	}
}

void ACombatCharacter::RefreshStarterWeaponFromPlayerState()
{
	const ACombatPlayerState* CombatPlayerState = CachedCombatPlayerState
		? CachedCombatPlayerState.Get()
		: GetPlayerState<ACombatPlayerState>();

	if (CombatPlayerState)
	{
		ApplyStarterWeaponState(
			CombatPlayerState->GetEquippedStarterWeapon(),
			CombatPlayerState->HasClaimedInitialStarterWeapon());
		return;
	}

	ApplyStarterWeaponState(ECombatStarterWeaponType::Unarmed, false);
}

void ACombatCharacter::CacheStarterWeaponBaseStats()
{
	if (bStarterWeaponBaseStatsCached)
	{
		return;
	}

	bStarterWeaponBaseStatsCached = true;
	BaseMeleeDamage = MeleeDamage;
	BaseMeleeTraceDistance = MeleeTraceDistance;
	BaseMeleeTraceRadius = MeleeTraceRadius;
}

void ACombatCharacter::ApplyStarterWeaponState(ECombatStarterWeaponType StarterWeapon, bool bStarterWeaponClaimed)
{
	CacheStarterWeaponBaseStats();

	CurrentStarterWeapon = StarterWeapon;
	bHasClaimedStarterWeapon = bStarterWeaponClaimed;

	MeleeDamage = BaseMeleeDamage;
	MeleeTraceDistance = BaseMeleeTraceDistance;
	MeleeTraceRadius = BaseMeleeTraceRadius;

	switch (StarterWeapon)
	{
	case ECombatStarterWeaponType::Sword:
		MeleeDamage = BaseMeleeDamage + 0.5f;
		MeleeTraceDistance = BaseMeleeTraceDistance + 35.0f;
		MeleeTraceRadius = BaseMeleeTraceRadius + 10.0f;
		break;
	case ECombatStarterWeaponType::Dagger:
		MeleeDamage = BaseMeleeDamage + 0.25f;
		MeleeTraceDistance = BaseMeleeTraceDistance + 10.0f;
		MeleeTraceRadius = FMath::Max(20.0f, BaseMeleeTraceRadius - 10.0f);
		break;
	case ECombatStarterWeaponType::ChannelingOrb:
		MeleeDamage = BaseMeleeDamage;
		MeleeTraceDistance = BaseMeleeTraceDistance + 45.0f;
		MeleeTraceRadius = BaseMeleeTraceRadius + 15.0f;
		break;
	default:
		break;
	}

	RefreshStarterWeaponVisual();
}

void ACombatCharacter::RefreshStarterWeaponVisual()
{
	if (!EquippedStarterWeaponVisual)
	{
		return;
	}

	if (!bHasClaimedStarterWeapon || CurrentStarterWeapon == ECombatStarterWeaponType::Unarmed)
	{
		EquippedStarterWeaponVisual->SetStaticMesh(nullptr);
		EquippedStarterWeaponVisual->SetVisibility(false, true);
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		EquippedStarterWeaponVisual->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			StarterWeaponAttachBoneName);
	}

	EquippedStarterWeaponVisual->SetStaticMesh(LoadStarterWeaponVisualMesh(CurrentStarterWeapon));
	EquippedStarterWeaponVisual->SetVisibility(EquippedStarterWeaponVisual->GetStaticMesh() != nullptr, true);

	constexpr float ImportedSwordScale = 0.0175f;
	constexpr float ImportedSecondaryWeaponSwordRatio = 0.5f;
	const auto GetStarterWeaponMaxExtent = [](const UStaticMesh* StaticMesh) -> float
	{
		if (!StaticMesh)
		{
			return 0.0f;
		}

		const FVector BoxExtent = StaticMesh->GetBounds().BoxExtent;
		return static_cast<float>(FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	};

	const auto ComputeStarterWeaponScaleFromSwordRatio = [&](const UStaticMesh* WeaponMesh, float SwordScale, float SwordRatio)
	{
		if (!WeaponMesh)
		{
			return SwordScale * SwordRatio;
		}

		const float WeaponMaxExtent = GetStarterWeaponMaxExtent(WeaponMesh);
		if (WeaponMaxExtent <= KINDA_SMALL_NUMBER)
		{
			return SwordScale * SwordRatio;
		}

		if (const UStaticMesh* SwordReferenceMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Arthurians/Weapons/One_Handed_Sword/T0/one_handed_sword_T0.one_handed_sword_T0")))
		{
			const float SwordMaxExtent = GetStarterWeaponMaxExtent(SwordReferenceMesh);
			if (SwordMaxExtent > KINDA_SMALL_NUMBER)
			{
				const float TargetDisplayedMaxExtent = SwordMaxExtent * SwordScale * SwordRatio;
				return TargetDisplayedMaxExtent / WeaponMaxExtent;
			}
		}

		return SwordScale * SwordRatio;
	};

	const auto BuildStarterWeaponGripLocation = [&](const UStaticMesh* WeaponMesh, float WeaponScale, float GripRatio = 0.40f, float VerticalOffset = -10.0f)
	{
		FVector RelativeLocation(0.0f, 0.0f, VerticalOffset);

		if (!WeaponMesh)
		{
			return RelativeLocation;
		}

		const FVector BoxExtent = WeaponMesh->GetBounds().BoxExtent;
		const float GripOffset = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z) * WeaponScale * GripRatio;

		if (BoxExtent.X >= BoxExtent.Y && BoxExtent.X >= BoxExtent.Z)
		{
			RelativeLocation.X -= GripOffset;
		}
		else if (BoxExtent.Y >= BoxExtent.Z)
		{
			RelativeLocation.Y -= GripOffset;
		}
		else
		{
			RelativeLocation.Z -= GripOffset;
		}

		return RelativeLocation;
	};

	const bool bUsingFallbackStarterSwordMesh = EquippedStarterWeaponVisual->GetStaticMesh()
		&& EquippedStarterWeaponVisual->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Cube.Cube");

	switch (CurrentStarterWeapon)
	{
	case ECombatStarterWeaponType::Sword:
		if (!bUsingFallbackStarterSwordMesh)
		{
			const UStaticMesh* SwordMesh = EquippedStarterWeaponVisual->GetStaticMesh();
			EquippedStarterWeaponVisual->SetRelativeLocation(BuildStarterWeaponGripLocation(SwordMesh, ImportedSwordScale));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(ImportedSwordScale, ImportedSwordScale, ImportedSwordScale));
		}
		else
		{
			EquippedStarterWeaponVisual->SetRelativeLocation(FVector(6.0f, 0.0f, 0.0f));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(0.06f, 0.02f, 0.75f));
		}
		break;
	case ECombatStarterWeaponType::Dagger:
		if (const UStaticMesh* DaggerMesh = EquippedStarterWeaponVisual->GetStaticMesh())
		{
			const float ImportedDaggerScale = ComputeStarterWeaponScaleFromSwordRatio(DaggerMesh, ImportedSwordScale, ImportedSecondaryWeaponSwordRatio);
			EquippedStarterWeaponVisual->SetRelativeLocation(BuildStarterWeaponGripLocation(DaggerMesh, ImportedDaggerScale));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(ImportedDaggerScale, ImportedDaggerScale, ImportedDaggerScale));
		}
		else
		{
			EquippedStarterWeaponVisual->SetRelativeLocation(FVector(4.0f, 0.0f, 0.0f));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.45f));
		}
		break;
	case ECombatStarterWeaponType::ChannelingOrb:
		if (const UStaticMesh* WandMesh = EquippedStarterWeaponVisual->GetStaticMesh())
		{
			const float ImportedWandScale = ComputeStarterWeaponScaleFromSwordRatio(WandMesh, ImportedSwordScale, ImportedSecondaryWeaponSwordRatio);
			EquippedStarterWeaponVisual->SetRelativeLocation(BuildStarterWeaponGripLocation(WandMesh, ImportedWandScale));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(ImportedWandScale, ImportedWandScale, ImportedWandScale));
		}
		else
		{
			EquippedStarterWeaponVisual->SetRelativeLocation(FVector(4.0f, 0.0f, 0.0f));
			EquippedStarterWeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
			EquippedStarterWeaponVisual->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.45f));
		}
		break;
	default:
		EquippedStarterWeaponVisual->SetVisibility(false, true);
		break;
	}
}

UStaticMesh* ACombatCharacter::LoadStarterWeaponVisualMesh(ECombatStarterWeaponType StarterWeapon) const
{
	switch (StarterWeapon)
	{
	case ECombatStarterWeaponType::Sword:
		if (UStaticMesh* ImportedSwordMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Arthurians/Weapons/One_Handed_Sword/T0/one_handed_sword_T0.one_handed_sword_T0")))
		{
			return ImportedSwordMesh;
		}
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	case ECombatStarterWeaponType::Dagger:
		if (UStaticMesh* ImportedDaggerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Arthurians/Weapons/Dagger/T0/Dagger_T0.Dagger_T0")))
		{
			return ImportedDaggerMesh;
		}
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	case ECombatStarterWeaponType::ChannelingOrb:
		if (UStaticMesh* ImportedWandMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Arthurians/Weapons/Magic_Wound/T0/Magic_Staff_T0.Magic_Staff_T0")))
		{
			return ImportedWandMesh;
		}
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	default:
		return nullptr;
	}
}

void ACombatCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACombatCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACombatCharacter::DoCameraZoom(float DeltaArmLength)
{
	if (CurrentHP <= 0.0f || CameraBoom == nullptr || FMath::IsNearlyZero(DeltaArmLength))
	{
		return;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->IsLookInputIgnored())
		{
			return;
		}
	}

	DesiredCombatCameraDistance = ClampCombatCameraDistance(DesiredCombatCameraDistance + DeltaArmLength);
	if (CombatCameraZoomInterpSpeed <= 0.0f)
	{
		CameraBoom->TargetArmLength = DesiredCombatCameraDistance;
	}
}

void ACombatCharacter::DoComboAttackStart()
{
	// are we already playing an attack animation?
	if (bIsAttacking)
	{
		// cache the input time so we can check it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	// perform a combo attack
	ComboAttack();
}

void ACombatCharacter::DoComboAttackEnd()
{
	// stub
}

void ACombatCharacter::DoChargedAttackStart()
{
	// raise the charging attack flag
	bIsChargingAttack = true;

	if (bIsAttacking)
	{
		// do not attack if the charge animation hasn't looped at least once
		if (!bHasLoopedChargedAttack)
		{
			bHasReleasedChargedAttack = false;
		}

		// cache the input time so we can check it later
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();

		return;
	}

	ChargedAttack();
}

void ACombatCharacter::DoChargedAttackEnd()
{
	// lower the charging attack flag
	bIsChargingAttack = false;

	// have we done the charge loop at least once and haven't released the button yet?
	if (bHasLoopedChargedAttack && !bHasReleasedChargedAttack)
	{
		// release the charge and resolve the attack
		bHasReleasedChargedAttack = true;

		LoopOrResolveChargedAttack();
	}
}

void ACombatCharacter::ResetHP()
{
	// reset the current HP total
	CurrentHP = MaxHP;

	// update the life bar
	LifeBarWidget->SetLifePercentage(1.0f);
}

void ACombatCharacter::RefreshWorldLifeBarVisibility()
{
	if (LifeBar == nullptr)
	{
		return;
	}

	LifeBar->SetVisibility(bShowWorldLifeBar, true);
	LifeBar->SetHiddenInGame(!bShowWorldLifeBar);
}

void ACombatCharacter::ComboAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	// reset the combo count
	ComboCount = 0;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
		}
	}

}

void ACombatCharacter::ChargedAttack()
{
	// raise the attacking flag
	bIsAttacking = true;

	// reset the charge loop flag
	bHasLoopedChargedAttack = false;

	// reset the charge release flag
	bHasReleasedChargedAttack = false;

	// notify enemies they are about to be attacked
	NotifyEnemiesOfIncomingAttack();

	// play the charged attack montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// subscribe to montage completed and interrupted events
		if (MontageLength > 0.0f)
		{
			// set the end delegate for the montage
			AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
		}
	}
}

void ACombatCharacter::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// reset the attacking flag
	bIsAttacking = false;

	// check if we have a non-stale cached input
	if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
	{
		// are we holding the charged attack button?
		if (bIsChargingAttack)
		{
			// do a charged attack
			ChargedAttack();
		}
		else
		{
			// do a regular attack
			ComboAttack();
		}
	}
}

void ACombatCharacter::DoAttackTrace(FName DamageSourceBone)
{
	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;
	TSet<AActor*> DamagedActors;

	// start at the provided socket location, sweep forward
	const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// check for pawn and world dynamic collision object types
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			AActor* HitActor = CurrentHit.GetActor();
			if (!HitActor || DamagedActors.Contains(HitActor))
			{
				continue;
			}

			// check if we've hit a damageable actor
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor);

			if (Damageable)
			{
				DamagedActors.Add(HitActor);

				// knock upwards and away from the impact normal
				const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

				// pass the damage event to the actor
				Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

				// call the BP handler to play effects, etc.
				DealtDamage(MeleeDamage, CurrentHit.ImpactPoint);
			}
		}
	}
}

void ACombatCharacter::CheckCombo()
{
	// are we playing a non-charge attack animation?
	if (bIsAttacking && !bIsChargingAttack)
	{
		// is the last attack input not stale?
		if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputCacheTimeTolerance)
		{
			// consume the attack input so we don't accidentally trigger it twice
			CachedAttackInputTime = 0.0f;

			// increase the combo counter
			++ComboCount;

			// do we still have a combo section to play?
			if (ComboCount < ComboSectionNames.Num())
			{
				// notify enemies they are about to be attacked
				NotifyEnemiesOfIncomingAttack();

				// jump to the next combo section
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
				}
			}
		}
	}
}

void ACombatCharacter::CheckChargedAttack()
{
	// raise the looped charged attack flag
	bHasLoopedChargedAttack = true;

	// set the input release flag from the input. This will determine if we loop or resolve
	bHasReleasedChargedAttack = !bIsChargingAttack;

	// resolve the charge loop
	LoopOrResolveChargedAttack();
}

void ACombatCharacter::LoopOrResolveChargedAttack()
{
	// jump to either the loop or the attack section depending on whether we've released the charge
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(bHasReleasedChargedAttack ? ChargeAttackSection : ChargeLoopSection , ChargedAttackMontage);
	}
}

void ACombatCharacter::NotifyEnemiesOfIncomingAttack()
{
	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;
	TSet<AActor*> WarnedActors;

	// start at the actor location, sweep forward
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * DangerTraceDistance);

	// check for pawn object types only
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(DangerTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			AActor* HitActor = CurrentHit.GetActor();
			if (!HitActor || WarnedActors.Contains(HitActor))
			{
				continue;
			}

			// check if we've hit a damageable actor
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor);

			if (Damageable)
			{
				WarnedActors.Add(HitActor);

				// notify the enemy
				Damageable->NotifyDanger(GetActorLocation(), this);
			}
		}
	}
}

void ACombatCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	// pass the damage event to the actor
	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	// only process knockback and effects if we received nonzero damage
	if (ActualDamage > 0.0f)
	{
		// apply the knockback impulse
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);

		// is the character ragdolling?
		if (GetMesh()->IsSimulatingPhysics())
		{
			// apply an impulse to the ragdoll
			GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(), DamageLocation);
		}

		// pass control to BP to play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}

}

void ACombatCharacter::HandleDeath()
{
	// disable movement while we're dead
	GetCharacterMovement()->DisableMovement();

	// enable full ragdoll physics
	GetMesh()->SetSimulatePhysics(true);

	// hide the life bar
	LifeBar->SetHiddenInGame(true);

	// pull back the camera
	DesiredCombatCameraDistance = ClampCombatCameraDistance(DeathCameraDistance);
	GetCameraBoom()->TargetArmLength = DesiredCombatCameraDistance;

	// schedule respawning
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &ACombatCharacter::RespawnCharacter, RespawnTime, false);
}

void ACombatCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	// stub
}

void ACombatCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// stub
}

void ACombatCharacter::RespawnCharacter()
{
	// destroy the character and let it be respawned by the Player Controller
	Destroy();
}

float ACombatCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// only process damage if the character is still alive
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// reduce the current HP
	CurrentHP -= Damage;

	// have we run out of HP?
	if (CurrentHP <= 0.0f)
	{
		// die
		HandleDeath();
	}
	else
	{
		// update the life bar
		LifeBarWidget->SetLifePercentage(CurrentHP / MaxHP);

		// enable partial ragdoll physics, but keep the pelvis vertical
		GetMesh()->SetPhysicsBlendWeight(0.5f);
		GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
	}

	// return the received damage amount
	return Damage;
}

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is the character still alive?
	if (CurrentHP >= 0.0f)
	{
		// disable ragdoll physics
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}
}

void ACombatCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CameraBoom == nullptr)
	{
		return;
	}

	DesiredCombatCameraDistance = ClampCombatCameraDistance(DesiredCombatCameraDistance);

	if (CombatCameraZoomInterpSpeed <= 0.0f)
	{
		CameraBoom->TargetArmLength = DesiredCombatCameraDistance;
		return;
	}

	const float CurrentDistance = CameraBoom->TargetArmLength;
	if (FMath::IsNearlyEqual(CurrentDistance, DesiredCombatCameraDistance, CombatCameraZoomSnapTolerance))
	{
		CameraBoom->TargetArmLength = DesiredCombatCameraDistance;
		return;
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(CurrentDistance, DesiredCombatCameraDistance, DeltaSeconds, CombatCameraZoomInterpSpeed);
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	// get the life bar from the widget component
	LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
	check(LifeBarWidget);

	// initialize the camera
	DesiredCombatCameraDistance = ClampCombatCameraDistance(DefaultCameraDistance);
	GetCameraBoom()->TargetArmLength = DesiredCombatCameraDistance;
	RefreshMovementSpeed();

	// save the relative transform for the mesh so we can reset the ragdoll later
	MeshStartingTransform = GetMesh()->GetRelativeTransform();

	// set the life bar color
	LifeBarWidget->SetBarColor(LifeBarColor);
	RefreshWorldLifeBarVisibility();

	// cache the baseline combat values before the starter weapon mutates them
	CacheStarterWeaponBaseStats();
	RefreshObservedCombatPlayerState();
	RefreshStarterWeaponFromPlayerState();

	// reset HP to maximum
	ResetHP();
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedCombatPlayerState)
	{
		CachedCombatPlayerState->OnStarterWeaponChanged.RemoveDynamic(this, &ACombatCharacter::HandleStarterWeaponChanged);
		CachedCombatPlayerState = nullptr;
	}

	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatCharacter::Look);

		// Combo Attack
		EnhancedInputComponent->BindAction(ComboAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ComboAttackPressed);

		// Charged Attack
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Started, this, &ACombatCharacter::ChargedAttackPressed);
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Completed, this, &ACombatCharacter::ChargedAttackReleased);
		EnhancedInputComponent->BindAction(ChargedAttackAction, ETriggerEvent::Canceled, this, &ACombatCharacter::ChargedAttackReleased);

		// Camera Side Toggle
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACombatCharacter::ToggleCamera);
	}

	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ACombatCharacter::ZoomCameraIn);
		PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ACombatCharacter::ZoomCameraOut);
		PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &ACombatCharacter::HandleWalkRunToggleReleased);
		PlayerInputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &ACombatCharacter::HandleSprintPressed);
		PlayerInputComponent->BindKey(EKeys::RightShift, IE_Released, this, &ACombatCharacter::HandleSprintReleased);
	}
}

void ACombatCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	RefreshObservedCombatPlayerState();
	RefreshStarterWeaponFromPlayerState();

	// update the respawn transform on the Player Controller
	if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(GetController()))
	{
		PC->SetRespawnTransform(GetActorTransform());
	}
}

void ACombatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RefreshObservedCombatPlayerState();
	RefreshStarterWeaponFromPlayerState();
}

void ACombatCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RefreshObservedCombatPlayerState();
	RefreshStarterWeaponFromPlayerState();
}
