// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TutorialPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, APlayerState*, PlayerState);

/**
 * 
 */
UCLASS()
class REPGRAPH_API ATutorialPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStateChanged OnPlayerStateChanged;
	
	virtual void OnRep_PlayerState() override;
	
};
