// Copyright 2018 Phosphor Studios. All Rights Reserved.

#include "TutorialTemplate.h"
#include "TutorialItem.h"

UTutorialTemplate::UTutorialTemplate()
	: UItemTemplate(false, UTutorialItem::StaticClass())
{
}

const TArray<FName> UTutorialTemplate::GetStepNames() const
{
	TArray<FName> OutStepNames;
	for (const FTutorialSequenceStep& Step : TutorialSequence.SequenceSteps)
	{
		OutStepNames.Add(Step.SequenceStepName);
	}
	return OutStepNames;
}

#if WITH_EDITOR
void UTutorialTemplate::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnTutorialTemplateUpdated.ExecuteIfBound(*this);
}

void UTutorialTemplate::CheckObjectForErrors()
{
	CHECK_REFERENCE_ALLOW_NULL(UTutorialTemplate, CatalogCustomData.NextTutorial);
}
#endif
