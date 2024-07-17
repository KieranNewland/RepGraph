// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TutorialCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, APlayerState*, NewPlayerState);

UCLASS()
class REPGRAPH_API ATutorialCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStateChanged OnPlayerStateChanged;

	virtual void BeginPlay() override;
	virtual void PostNetReceive() override;
	virtual void OnRep_PlayerState() override;
};
