// Copyright 2018 Phosphor Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "TutorialItem.generated.h"

class UTutorialManager;
class UTutorialTemplate;
class UAnalyticsManager;

struct FTutorialSequenceStep;
struct FTutorialWorldIndicatorData;
struct FTutorialDialogueData;
struct FGameplayTagContainer;
struct FTutorialSequence;

USTRUCT(BlueprintType)
struct FTutorialInstanceCustomData : public FItemInstanceCustomData
{
	GENERATED_BODY()

};

/**
* Item class created from a Tutorial Template & used to store the track the progress of the active tutorial
*/
UCLASS()
class GAME_API UTutorialItem : public UItem
{
	GENERATED_BODY()

public:
	virtual void PostCreateInitialize() override;
	virtual void PostLoadInitialize() override;

	ETutorialType GetTutorialType() const;

	const FTutorialSequenceStep& GetCurrentSequenceStep() const;
	class UWidget* GetCurrentTargetWidget() const;
	const FTutorialWorldIndicatorData& GetCurrentWorldIndicatorData() const;
	const FTutorialDialogueData& GetCurrentDialogueData() const;
	TSubclassOf<class UWidget> GetNextWidgetStepOverride() const;

	const FGameplayTag& GetTutorialCompletionTag() const;
	const FCatalogReference& GetNextTutorial() const;
	int32 GetStepIndex() const { return StepIndex; }
	void ApplyStepEffects();

	bool IsMenuUnchanged() const;
	bool DoesWorldIndicatorOpenMenu() const;
	bool HandleTutorialAdvanced();

	void EndTutorial();

	void LogInvalidGraphicsStep() const;
	void LogInvalidWorldIndicatorStep() const;

protected:
	void OnDataInitialized();

	const FTutorialSequence& GetCurrentSequence() const;
	int32 GetStepCount() const;
	UTutorialTemplate* GetTutorialTemplate() const;

	UTutorialManager* TutorialManager;

	int32 StepIndex = 0;

	FTutorialInstanceCustomData InstanceCustomData;

	INSTANCE_CUSTOM_DATA_FUNCTIONS();

private:
	UAnalyticsManager* GetAnalytics() const;
};
