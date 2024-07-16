// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialPlayerController.h"


void ATutorialPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	OnPlayerStateChanged.Broadcast(PlayerState);
}
