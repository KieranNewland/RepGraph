#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TutorialPlayerState.generated.h"

UCLASS()
class REPGRAPH_API ATutorialPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 Team = -1;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
