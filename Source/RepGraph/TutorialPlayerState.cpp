#include "TutorialPlayerState.h"

#include "TutorialRepGraph.h"
#include "Net/UnrealNetwork.h"


void ATutorialPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Team = FMath::RandBool() ? 0 : 1;
		
		if (const UWorld* World = GetWorld())
		{
			if (const UNetDriver* NetworkDriver = World->GetNetDriver())
			{
				if (UTutorialRepGraph* RepGraph = NetworkDriver->GetReplicationDriver<UTutorialRepGraph>())
				{
					RepGraph->SetTeamForPlayerController(GetPlayerController(), Team);
				}
			}
		}
	}
}

void ATutorialPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Team);
}
