// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialCharacter.h"

void ATutorialCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ATutorialCharacter::PostNetReceive()
{
	Super::PostNetReceive();
}

void ATutorialCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	OnPlayerStateChanged.Broadcast(GetPlayerState());
}
