#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatStarterWeaponTypes.h"
#include "CombatStarterWeaponChoice.generated.h"

class APlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Simple visible overlap choice used by the minimal combat starter hub.
 */
UCLASS()
class ACombatStarterWeaponChoice : public AActor
{
	GENERATED_BODY()

public:
	ACombatStarterWeaponChoice();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetStarterWeaponType(ECombatStarterWeaponType InStarterWeaponType);
	ECombatStarterWeaponType GetStarterWeaponType() const { return StarterWeaponType; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> OverlapBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(ReplicatedUsing=OnRep_StarterWeaponType, EditAnywhere, BlueprintReadOnly, Category="Starter Weapon")
	ECombatStarterWeaponType StarterWeaponType = ECombatStarterWeaponType::Sword;

private:
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_StarterWeaponType();

	void RefreshChoicePresentation();
	void NotifyPlayerController(APlayerController* PlayerController, const FString& Message) const;
	UStaticMesh* ResolveChoiceMesh() const;
};
