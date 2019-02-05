// Copyright 2018 Phosphor Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ItemTemplate.h"
#include "GameplayTagContainer.h"
#include "TutorialTemplate.generated.h"

USTRUCT(BlueprintType)
struct FTutorialWidgetData
{
	GENERATED_BODY()

	// Determines whether or not this step will wait for a menu to open or an animation to finish
	// "Auto" advancement assumes that the next display position of the indicator is already at its final position on screen when the indicator is pressed
	UPROPERTY(EditDefaultsOnly, Category = StepData)
	bool bMenuUnchangedOnClick = false;

	UPROPERTY(EditDefaultsOnly, Category = StepData, meta=(EditCondition="!bMenuUnChangedOnClick"))
	TSubclassOf<class UWidget> NextStepWidgetOverride;

	UPROPERTY(EditDefaultsOnly, Category = StepData)
	TArray<FName> TargetWidgetPath;
};

USTRUCT(BlueprintType)
struct FTutorialWorldIndicatorData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bOpensMenu = true;

	UPROPERTY(EditDefaultsOnly)
	bool bMapIndicator = false;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle = "!bMapIndicator"))
	bool bSelectRegion = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bMapIndicator"))
	int32 RegionSlot = 0;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bMapIndicator"))
	int32 BuildingSlot = 0;

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle = "bMapIndicator"))
	FIntPoint TileCoordinate;
};

USTRUCT(BlueprintType)
struct FTutorialDialogueData
{
	GENERATED_BODY()

	/** 
	* Determines which side of the screen the speaker will appear on 
	* If the speaker image is already displayed this will be ignored
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ESpeakerDisplayPosition DisplayPosition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText SpeakerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UPaperSprite* SpeakerSprite;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	FText DialogueText;
};

USTRUCT(BlueprintType)
struct FTutorialIndicatorData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bWorldIndicator = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bWorldIndicator", ShowOnlyInnerProperties))
	FTutorialWidgetData WidgetData;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bWorldIndicator", ShowOnlyInnerProperties))
	FTutorialWorldIndicatorData WorldIndicatorData;

};

USTRUCT(BlueprintType)
struct FTutorialSequenceStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName SequenceStepName;

	UPROPERTY(EditDefaultsOnly)
	FPermanentStatModCollection StepStatEffect;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag StepTag;

	UPROPERTY(EditDefaultsOnly)
	bool bDialogueDisplayed = false;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bDialogueDisplayed", ShowOnlyInnerProperties))
	FTutorialIndicatorData IndicatorData;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bDialogueDisplayed"))
	FTutorialDialogueData DialogueData;

};

USTRUCT(BlueprintType)
struct FTutorialSequence
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName SequenceName;

	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "SequenceStepName"))
	TArray<FTutorialSequenceStep> SequenceSteps;
};

USTRUCT(BlueprintType)
struct FTutorialBuildingSetting
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
	bool bChangeBuilding = true;

	// If null building is changed back to its default type
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bChangeBuilding"))
	class UBuildingTemplateBase* BuildingTemplate;
};

USTRUCT(BlueprintType)
struct FTutorialRegionSetting
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bRegionActive = true;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bRegionActive))
	TArray<FTutorialBuildingSetting> BuildingSettings;
};

USTRUCT(BlueprintType)
struct FTutorialTemplateCustomData : public FCatalogCustomData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category=TutorialCompletionData)
	FCatalogReference NextTutorial;

	UPROPERTY(EditDefaultsOnly, Category = TutorialInitData)
	ETutorialType Type = ETutorialType::Initial;

	UPROPERTY(EditDefaultsOnly, Category = TutorialCompletionData)
	bool bCloseMenuOnCompletion = true;
};

DECLARE_DELEGATE_OneParam(FTutorialTemplateUpdatedEvent, class UTutorialTemplate&);

UCLASS(BlueprintType, HideCategories = Item)
class GAME_API UTutorialTemplate : public UItemTemplate
{
	GENERATED_BODY()

public:
	UTutorialTemplate();

	UPROPERTY(EditDefaultsOnly, Category = TutorialInitData)
	TMap<UItemTemplate*, int32> TutorialItemsGranted;

	UPROPERTY(EditDefaultsOnly, Category = TutorialTownData, meta = (InlineEditConditionToggle))
	bool bCustomBaseSetup = false;

	UPROPERTY(EditDefaultsOnly, Category = TutorialTownData, meta = (EditCondition="bCustomBaseSetup"))
	TArray<FTutorialRegionSetting> RegionSettings;

	UPROPERTY(EditDefaultsOnly, Category = TutorialData, meta = (TitleProperty = "SequenceName"))
	FTutorialSequence TutorialSequence;

	UPROPERTY(VisibleDefaultsOnly, Category = TutorialInitData)
	FGameplayTag TutorialTag;

	UPROPERTY(VisibleDefaultsOnly, Category = TutorialCompletionData)
	FGameplayTag TutorialCompletionTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = PlayFabCustomData, meta = (ShowOnlyInnerProperties))
	FTutorialTemplateCustomData CatalogCustomData;

	const TArray<FName> GetStepNames() const;
	ETutorialType GetTutorialType() const { return CatalogCustomData.Type; }

#if WITH_EDITOR
	FTutorialTemplateUpdatedEvent OnTutorialTemplateUpdated;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void CheckObjectForErrors() override;
#endif

	CATALOG_CUSTOM_DATA_FUNCTIONS();
};
