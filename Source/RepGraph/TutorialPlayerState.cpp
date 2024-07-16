#include "TutorialPlayerState.h"

#include "Net/UnrealNetwork.h"


void ATutorialPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Team = FMath::RandBool() ? 0 : 1;
	}
}

void ATutorialPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Team);
}
