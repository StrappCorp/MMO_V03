// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatEncounterSignal.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"

ACombatEncounterSignal::ACombatEncounterSignal()
{
	PrimaryActorTick.bCanEverTick = false;
	InactiveLabel = FText::FromString(TEXT("READY"));
	ActiveLabel = FText::FromString(TEXT("CLEAR"));
	InactiveColor = FLinearColor(0.8f, 0.15f, 0.1f, 1.0f);
	ActiveColor = FLinearColor(0.1f, 0.9f, 0.2f, 1.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	VisualMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 1.4f));
	VisualMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));

	SignalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SignalLight"));
	SignalLight->SetupAttachment(SceneRoot);
	SignalLight->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	SignalLight->SetIntensity(12000.0f);
	SignalLight->SetAttenuationRadius(550.0f);
	SignalLight->SetCastShadows(false);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 240.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Label->SetWorldSize(52.0f);
	Label->SetCanEverAffectNavigation(false);

	RefreshPresentation();
}

void ACombatEncounterSignal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void ACombatEncounterSignal::SetInactiveLabel(const FText& InText)
{
	InactiveLabel = InText;
	RefreshPresentation();
}

void ACombatEncounterSignal::SetActiveLabel(const FText& InText)
{
	ActiveLabel = InText;
	RefreshPresentation();
}

void ACombatEncounterSignal::ToggleInteraction(AActor* ActivationInstigator)
{
	bIsActive = !bIsActive;
	RefreshPresentation();
}

void ACombatEncounterSignal::ActivateInteraction(AActor* ActivationInstigator)
{
	bIsActive = true;
	RefreshPresentation();
}

void ACombatEncounterSignal::DeactivateInteraction(AActor* ActivationInstigator)
{
	bIsActive = false;
	RefreshPresentation();
}

void ACombatEncounterSignal::RefreshPresentation()
{
	if (Label)
	{
		Label->SetText(bIsActive ? ActiveLabel : InactiveLabel);
		Label->SetTextRenderColor((bIsActive ? ActiveColor : InactiveColor).ToFColor(true));
	}

	if (SignalLight)
	{
		SignalLight->SetLightColor((bIsActive ? ActiveColor : InactiveColor).ToFColor(true));
		SignalLight->SetIntensity(bIsActive ? 18000.0f : 9000.0f);
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(bIsActive ? FVector(0.42f, 0.42f, 1.6f) : FVector(0.35f, 0.35f, 1.4f));
	}
}
