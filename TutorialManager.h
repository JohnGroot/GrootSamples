// Copyright 2018 Phosphor Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TutorialManager.generated.h"

class APlayerController;
class UTutorialDialogueWidget;
class UTutorialTemplate;
class UTutorialItem;
class UUserWidget;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldTutorialIndicatorDisplayed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldTutorialIndicatorHidden);

UCLASS(HideCategories = (ComponentTick, Tags, ComponentReplication, Activation, Variable, Collision, Cooking))
class GAME_API UTutorialManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UTutorialManager();

	void Init(APlayerController* InPlayerController, UWidgetComponent* InWidgetComponent);
	void SetupDefaultTutorial();

	void ScheduleTutorialAdvancement();

	void SetActiveTutorial(UTutorialItem* InTutorialItem);

	bool IsActive() const;

	void TryStartTutorial(UTutorialTemplate* InTemplate) const;

	void TryStartDynamicTutorial(const FGameplayTag& TutorialTag);

	void AddTutorialItem(UTutorialItem* InTutorialItem);

	void ForceTutorialEnd();

protected:
	UFUNCTION()
	void OnTutorialIndicatorClicked(class UPhoButton* InButton);

	UFUNCTION()
	void OnWorldIndicatorPressed(class UPhoButton* InButton);

	UFUNCTION()
	void OnTutorialDialoguePressed(class UPhoButton* InButton);

	bool IsTutorialStarted(const UTutorialTemplate* InTemplate) const;

	void AdvanceTutorial();

	void DisplayTutorialStep();
	void DisplayDialogue();
	void DisplayIndicator();
	void DisplayWorldIndicator();
	void PositionIndicatorOverWidget(const class UWidget* InWidget);
	void PositionIndicatorOverWorldPosition(const FVector& InPosition);

	AActor* GetFocusedWorldActor(const struct FTutorialWorldIndicatorData& WorldIndicatorData) const;

	bool CanAdvanceTutorial() const;
	void EndTutorial();

	UTutorialTemplate* GetLastTutorialTemplate() const;

	APlayerController* PlayerController;

	UTutorialItem* ActiveTutorial;

	UPROPERTY(EditDefaultsOnly)
	UTutorialTemplate* DefaultTutorial;

	UPROPERTY(EditDefaultsOnly)
	TArray<UTutorialTemplate*> DynamicTutorials;

	UPROPERTY()
	TArray<UTutorialItem*> ActiveDynamicTutorials;

	UTutorialItem* GetActiveDynamicTutorial(const FGameplayTag& InTutorialTag);
	UTutorialTemplate* GetDynamicTutorialTemplate(const FGameplayTag& InTutorialTag) const;

	UPROPERTY()
	UTutorialDialogueWidget* TutorialDialogueWidget;

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	TSubclassOf<UTutorialDialogueWidget> TutorialDialogueWidgetClass;

	UPROPERTY()
	UUserWidget* TutorialWidget;

	class UCanvasPanelSlot* TutorialWidgetSlot;

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	TSubclassOf<UUserWidget> TutorialIndicatorWidget;

	// Determines how many pixels the Tutorial UI Indicator is offset from a target widget's edges
	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	FVector2D TutorialIndicatorEdgeOffset;

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	TSubclassOf<UUserWidget> WorldIndicatorWidgetClass;

	UWidgetComponent* TutorialWidgetComponent;

	// Determines how many pixels the Tutorial UI Indicator is offset from a target widget's edges
	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	FVector2D WorldIndicatorSize;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> InterstitialWidgetClass;

	UPROPERTY()
	UUserWidget* InterstitialWidget = nullptr;

	UPROPERTY(BlueprintAssignable)
	FOnWorldTutorialIndicatorDisplayed OnWorldIndicatorDisplayed;

	UPROPERTY(BlueprintAssignable)
	FOnWorldTutorialIndicatorHidden OnWorldIndicatorHidden;

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	FName TutorialIndicatorButtonName = TEXT("TutorialIndicatorButton");

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	FName TutorialDialogueButtonName = TEXT("HiddenButton");

	UPROPERTY(EditDefaultsOnly, Category = TutorialSettings)
	FName TutorialWorldButtonName = TEXT("TutorialIndicatorButton");

	bool bAdvancementScheduled = false;
	int32 TutorialWidgetZOrder = 99;
};
