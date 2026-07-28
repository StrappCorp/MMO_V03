#include "CombatStarterWeaponChoice.h"

#include "CombatPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

ACombatStarterWeaponChoice::ACombatStarterWeaponChoice()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	OverlapBox->SetupAttachment(SceneRoot);
	OverlapBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	OverlapBox->SetBoxExtent(FVector(90.0f, 90.0f, 120.0f));
	OverlapBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapBox->SetGenerateOverlapEvents(true);
	OverlapBox->SetCanEverAffectNavigation(false);
	OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &ACombatStarterWeaponChoice::OnOverlap);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Label->SetWorldSize(42.0f);
	Label->SetTextRenderColor(FColor::White);
	Label->SetCanEverAffectNavigation(false);

	RefreshChoicePresentation();
}

void ACombatStarterWeaponChoice::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatStarterWeaponChoice, StarterWeaponType);
}

void ACombatStarterWeaponChoice::SetStarterWeaponType(ECombatStarterWeaponType InStarterWeaponType)
{
	StarterWeaponType = InStarterWeaponType;
	RefreshChoicePresentation();
	ForceNetUpdate();
}

void ACombatStarterWeaponChoice::BeginPlay()
{
	Super::BeginPlay();
	RefreshChoicePresentation();
}

void ACombatStarterWeaponChoice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshChoicePresentation();
}

void ACombatStarterWeaponChoice::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn || !OverlappingPawn->IsPlayerControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OverlappingPawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	ACombatPlayerState* CombatPlayerState = PlayerController->GetPlayerState<ACombatPlayerState>();
	if (!CombatPlayerState)
	{
		NotifyPlayerController(PlayerController, TEXT("Échec - progression indisponible pour choisir l’arme initiale."));
		return;
	}

	const FString StarterWeaponDefinitionId = CombatStarterWeapon::WeaponTypeToDefinitionId(StarterWeaponType);
	if (StarterWeaponDefinitionId.IsEmpty())
	{
		NotifyPlayerController(PlayerController, TEXT("Échec - cette borne n’a pas d’arme initiale configurée."));
		return;
	}

	FString SuccessMessage;
	FString FailureReason;
	if (CombatPlayerState->ClaimInitialStarterWeapon(StarterWeaponDefinitionId, SuccessMessage, FailureReason))
	{
		NotifyPlayerController(PlayerController, FString::Printf(TEXT("Succès - %s"), *SuccessMessage));
	}
	else
	{
		const FString ResolvedFailureReason = FailureReason.IsEmpty()
			? TEXT("Choix d’arme initiale indisponible.")
			: FailureReason;
		NotifyPlayerController(PlayerController, FString::Printf(TEXT("Échec - %s"), *ResolvedFailureReason));
	}
}

void ACombatStarterWeaponChoice::OnRep_StarterWeaponType()
{
	RefreshChoicePresentation();
}

void ACombatStarterWeaponChoice::RefreshChoicePresentation()
{
	if (Label)
	{
		Label->SetText(CombatStarterWeapon::GetDisplayText(StarterWeaponType));
	}

	if (!VisualMesh)
	{
		return;
	}

	VisualMesh->SetStaticMesh(ResolveChoiceMesh());

	constexpr float ImportedSwordDisplayScale = 0.033333f;
	constexpr float ImportedSecondaryWeaponSwordRatio = 0.5f;
	const auto GetWeaponMaxExtent = [](const UStaticMesh* StaticMesh) -> float
	{
		if (!StaticMesh)
		{
			return 0.0f;
		}

		const FVector BoxExtent = StaticMesh->GetBounds().BoxExtent;
		return static_cast<float>(FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	};

	const auto ComputeScaleFromSwordRatio = [&](const UStaticMesh* WeaponMesh, float SwordScale, float SwordRatio)
	{
		if (!WeaponMesh)
		{
			return SwordScale * SwordRatio;
		}

		const float WeaponMaxExtent = GetWeaponMaxExtent(WeaponMesh);
		if (WeaponMaxExtent <= KINDA_SMALL_NUMBER)
		{
			return SwordScale * SwordRatio;
		}

		if (const UStaticMesh* SwordReferenceMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Arthurians/Weapons/One_Handed_Sword/T0/one_handed_sword_T0.one_handed_sword_T0")))
		{
			const float SwordMaxExtent = GetWeaponMaxExtent(SwordReferenceMesh);
			if (SwordMaxExtent > KINDA_SMALL_NUMBER)
			{
				const float TargetDisplayedMaxExtent = SwordMaxExtent * SwordScale * SwordRatio;
				return TargetDisplayedMaxExtent / WeaponMaxExtent;
			}
		}

		return SwordScale * SwordRatio;
	};

	const bool bUsingFallbackChoiceMesh = VisualMesh->GetStaticMesh()
		&& VisualMesh->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/"));

	switch (StarterWeaponType)
	{
	case ECombatStarterWeaponType::Sword:
		if (!bUsingFallbackChoiceMesh)
		{
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
			VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
			VisualMesh->SetRelativeScale3D(FVector(ImportedSwordDisplayScale, ImportedSwordDisplayScale, ImportedSwordDisplayScale));
		}
		else
		{
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
			VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
			VisualMesh->SetRelativeScale3D(FVector(0.35f, 0.12f, 1.25f));
		}
		break;
	case ECombatStarterWeaponType::Dagger:
		if (!bUsingFallbackChoiceMesh)
		{
			const float ImportedDaggerScale = ComputeScaleFromSwordRatio(VisualMesh->GetStaticMesh(), ImportedSwordDisplayScale, ImportedSecondaryWeaponSwordRatio);
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
			VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
			VisualMesh->SetRelativeScale3D(FVector(ImportedDaggerScale, ImportedDaggerScale, ImportedDaggerScale));
		}
		else
		{
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
			VisualMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
			VisualMesh->SetRelativeScale3D(FVector(0.20f, 0.20f, 0.80f));
		}
		break;
	case ECombatStarterWeaponType::ChannelingOrb:
		if (!bUsingFallbackChoiceMesh)
		{
			const float ImportedWandScale = ComputeScaleFromSwordRatio(VisualMesh->GetStaticMesh(), ImportedSwordDisplayScale, ImportedSecondaryWeaponSwordRatio);
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
			VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
			VisualMesh->SetRelativeScale3D(FVector(ImportedWandScale, ImportedWandScale, ImportedWandScale));
		}
		else
		{
			VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
			VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
			VisualMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.45f));
		}
		break;
	default:
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
		VisualMesh->SetRelativeScale3D(FVector(0.20f, 0.20f, 0.20f));
		break;
	}
}

void ACombatStarterWeaponChoice::NotifyPlayerController(APlayerController* PlayerController, const FString& Message) const
{
	if (PlayerController && !Message.IsEmpty())
	{
		PlayerController->ClientMessage(FString::Printf(TEXT("[Starter] %s"), *Message));
	}
}

UStaticMesh* ACombatStarterWeaponChoice::ResolveChoiceMesh() const
{
	switch (StarterWeaponType)
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
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	default:
		return nullptr;
	}
}
