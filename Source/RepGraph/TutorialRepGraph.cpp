#include "TutorialRepGraph.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

UReplicationGraphNode_AlwaysRelevant_WithPending::UReplicationGraphNode_AlwaysRelevant_WithPending()
{
	// Call PrepareForReplication before replication once per frame
	bRequiresPrepareForReplicationCall = true;
}

void UReplicationGraphNode_AlwaysRelevant_WithPending::PrepareForReplication()
{
	UTutorialRepGraph* ReplicationGraph = Cast<UTutorialRepGraph>(GetOuter());
	ReplicationGraph->HandlePendingActorsAndTeamRequests();
}

void UReplicationGraphNode_AlwaysRelevant_ForTeam::GatherActorListsForConnection(
	const FConnectionGatherActorListParameters& Params)
{
	// Get all other team members with the same team ID from ReplicationGraph->TeamConnectionListMap
	UTutorialRepGraph* ReplicationGraph = Cast<UTutorialRepGraph>(GetOuter());
	const UTutorialConnectionManager* ConnectionManager = Cast<UTutorialConnectionManager>(&Params.ConnectionManager);
	if (ReplicationGraph && ConnectionManager && ConnectionManager->Team != -1)
	{
		if (TArray<UTutorialConnectionManager*>* TeamConnections = ReplicationGraph->TeamConnectionListMap.GetConnectionArrayForTeam(ConnectionManager->Team))
		{
			for (const UTutorialConnectionManager* TeamMember : *TeamConnections)
			{
				TeamMember->TeamConnectionNode->GatherActorListsForConnectionDefault(Params);
			}
		}

		// Add all visible non-team actors to the list
		const TArray<UTutorialConnectionManager*>& NonTeamConnections = ReplicationGraph->TeamConnectionListMap.GetVisibleConnectionArrayForNonTeam(ConnectionManager->Pawn.Get(), ConnectionManager->Team);

		for (const UTutorialConnectionManager* NonTeamMember : NonTeamConnections)
		{
			NonTeamMember->TeamConnectionNode->GatherActorListsForConnectionDefault(Params);
		}
	}
	else
	{
		Super::GatherActorListsForConnection(Params);
	}
}

void UReplicationGraphNode_AlwaysRelevant_ForTeam::GatherActorListsForConnectionDefault(
	const FConnectionGatherActorListParameters& Params)
{
	Super::GatherActorListsForConnection(Params);
}

TArray<UTutorialConnectionManager*>* FTeamConnectionListMap::GetConnectionArrayForTeam(int32 Team)
{
	return Find(Team);
}

TArray<UTutorialConnectionManager*> FTeamConnectionListMap::GetVisibleConnectionArrayForNonTeam(const APawn* Pawn, int32 Team)
{	
	TArray<UTutorialConnectionManager*> NonTeamConnections;

	if (!IsValid(Pawn))
	{
		return NonTeamConnections;
	}

	// Setup query params and ignore all team members
	TArray<UTutorialConnectionManager*>* TeamMembers = GetConnectionArrayForTeam(Team);
		
	FCollisionQueryParams TraceParams;
	if (TeamMembers)
	{
		for (const UTutorialConnectionManager* ConnectionManager : *TeamMembers)
		{
			TraceParams.AddIgnoredActor(ConnectionManager->Pawn.Get());
		}
	}
	else
	{
		TraceParams.AddIgnoredActor(Pawn);
	}

	// Iterate over all teams that do not match the input team
	TArray<int32> Teams;
	GetKeys(Teams);

	const UWorld* World = Pawn->GetWorld();
	const FVector TraceOffset = FVector(0.0f, 0.0f, 180.0f);
	const FVector TraceStart = Pawn->GetActorLocation() + TraceOffset;
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		const int32 TeamID = Teams[i];
		if (TeamID != Team)
		{
			const TArray<UTutorialConnectionManager*>* OtherTeamMembers = GetConnectionArrayForTeam(TeamID);

			if (OtherTeamMembers)
			{
				for (UTutorialConnectionManager* ConnectionManager : *OtherTeamMembers)
				{
					if (!ConnectionManager->Pawn.IsValid())
					{
						continue;
					}
					
					// Raycast between our pawn and the other. If we hit anything then we do not have line of sight
					FHitResult OutHit;
					const FVector TraceEnd = ConnectionManager->Pawn.Get()->GetActorLocation() + TraceOffset;
					if (!World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_GameTraceChannel1, TraceParams))
					{
						NonTeamConnections.Add(ConnectionManager);		
					}
				}
			}
		}
	}

	return NonTeamConnections;
}

void FTeamConnectionListMap::AddConnectionToTeam(int32 Team, UTutorialConnectionManager* ConnManager)
{
	TArray<UTutorialConnectionManager*>& TeamList = FindOrAdd(Team);
	TeamList.Add(ConnManager);
}

void FTeamConnectionListMap::RemoveConnectionFromTeam(int32 Team, UTutorialConnectionManager* ConnManager)
{
	if (TArray<UTutorialConnectionManager*>* TeamList = Find(Team))
	{
		TeamList->RemoveSwap(ConnManager);

		// Remove the team from the map if there are no more connections
		if (TeamList->Num() == 0)
		{
			Remove(Team);
		}
	}
}

UTutorialRepGraph::UTutorialRepGraph()
{
	// Specify the connection graph class to use
	ReplicationConnectionManagerClass = UTutorialConnectionManager::StaticClass();
}

void UTutorialRepGraph::InitGlobalGraphNodes()
{
	Super::InitGlobalGraphNodes();

	// Create the always relevant node
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_WithPending>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UTutorialRepGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager)
{
	Super::InitConnectionGraphNodes(ConnectionManager);

	// Create the connection graph for the incoming connection
	UTutorialConnectionManager* TutorialRepGraph = Cast<UTutorialConnectionManager>(ConnectionManager);

	if (ensure(TutorialRepGraph))
	{
		TutorialRepGraph->AlwaysRelevantForConnectionNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
		AddConnectionGraphNode(TutorialRepGraph->AlwaysRelevantForConnectionNode, ConnectionManager);
		
		TutorialRepGraph->TeamConnectionNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForTeam>();
		AddConnectionGraphNode(TutorialRepGraph->TeamConnectionNode, ConnectionManager);
	}
}

void UTutorialRepGraph::RemoveClientConnection(UNetConnection* NetConnection)
{
	int32 ConnectionId = 0;
	bool bFound = false;
	
	auto UpdateList = [&](TArray<TObjectPtr<UNetReplicationGraphConnection>>& List)
	{
		for (int32 idx = 0; idx < List.Num(); ++idx)
		{
			UTutorialConnectionManager* ConnectionManager = Cast<UTutorialConnectionManager>(Connections[idx]);
			repCheck(ConnectionManager);

			if (ConnectionManager->NetConnection == NetConnection)
			{
				ensure(!bFound);

				// Remove the connection from the team node if the team is valid
				if (ConnectionManager->Team != -1)
				{
					TeamConnectionListMap.RemoveConnectionFromTeam(ConnectionManager->Team, ConnectionManager);
				}

				// Also remove it from the input list
				List.RemoveAtSwap(idx, 1, EAllowShrinking::No);
				bFound = true;
			}
			else
			{
				ConnectionManager->ConnectionOrderNum = ConnectionId++;
			}
		}
	};

	UpdateList(Connections);
	UpdateList(PendingConnections);
}

void UTutorialRepGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();

	PendingConnectionActors.Reset();
	PendingTeamRequests.Reset();

	auto EmptyConnectionNode = [](TArray<TObjectPtr<UNetReplicationGraphConnection>>& GraphConnections)
	{
		for (UNetReplicationGraphConnection* GraphConnection : GraphConnections)
		{
			if (const UTutorialConnectionManager* TutorialConnectionManager = Cast<UTutorialConnectionManager>(GraphConnection))
			{
				// Clear out all always relevant actors
				// Seamless travel means that the team connections will still be relevant due to the controllers not being destroyed
				TutorialConnectionManager->AlwaysRelevantForConnectionNode->NotifyResetAllNetworkActors();
			}
		}
	};

	EmptyConnectionNode(PendingConnections);
	EmptyConnectionNode(Connections);
}

void UTutorialRepGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
                                                    FGlobalActorReplicationInfo& GlobalInfo)
{
	// All clients must receive game states and player states
	if (ActorInfo.Class->IsChildOf(AGameStateBase::StaticClass()) || ActorInfo.Class->IsChildOf(APlayerState::StaticClass()))
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
	}
	// If not we see if it belongs to a connection
	else if (UTutorialConnectionManager* ConnectionManager = GetTutorialConnectionManagerFromActor(ActorInfo.GetActor()))
	{
		if (ActorInfo.Actor->bOnlyRelevantToOwner)
		{
			ConnectionManager->AlwaysRelevantForConnectionNode->NotifyAddNetworkActor(ActorInfo);
		}
		else
		{
			ConnectionManager->TeamConnectionNode->NotifyAddNetworkActor(ActorInfo);

			if (APawn* Pawn = Cast<APawn>(ActorInfo.GetActor()))
			{
				ConnectionManager->Pawn = Pawn;
			}
		}
	}
	else if(ActorInfo.Actor->GetNetOwner())
	{
		// Add to PendingConnectionActors if the net connection is not ready yet
		PendingConnectionActors.Add(ActorInfo.GetActor());
	}
}

void UTutorialRepGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	if (ActorInfo.Class->IsChildOf(AGameStateBase::StaticClass()) || ActorInfo.Class->IsChildOf(APlayerState::StaticClass()))
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
	}
	else if (const UTutorialConnectionManager* ConnectionManager = GetTutorialConnectionManagerFromActor(ActorInfo.GetActor()))
	{
		if (ActorInfo.Actor->bOnlyRelevantToOwner)
		{
			ConnectionManager->AlwaysRelevantForConnectionNode->NotifyRemoveNetworkActor(ActorInfo);
		}
		else
		{
			ConnectionManager->TeamConnectionNode->NotifyRemoveNetworkActor(ActorInfo);
		}
	}
	else if (ActorInfo.Actor->GetNetOwner())
	{
		PendingConnectionActors.Remove(ActorInfo.GetActor());
	}
}

void UTutorialRepGraph::SetTeamForPlayerController(APlayerController* PlayerController, int32 Team)
{
	if (PlayerController)
	{
		if (UTutorialConnectionManager* ConnectionManager = GetTutorialConnectionManagerFromActor(PlayerController))
		{
			const int32 CurrentTeam = ConnectionManager->Team;
			if (CurrentTeam != Team)
			{
				// Remove the connection to the old team list
				if (CurrentTeam != -1)
				{
					TeamConnectionListMap.RemoveConnectionFromTeam(CurrentTeam, ConnectionManager);
				}

				// Add the graph to the new team list
				if (Team != -1)
				{
					TeamConnectionListMap.AddConnectionToTeam(Team, ConnectionManager);
				}
				ConnectionManager->Team = Team;
			}
		}
		else
		{
			// Add to PendingTeamRequests if the net connection is not ready yet
			PendingTeamRequests.Emplace(Team, PlayerController);
		}
	}
}

void UTutorialRepGraph::HandlePendingActorsAndTeamRequests()
{
	// Setup all pending team requests
	if(PendingTeamRequests.Num() > 0)
	{
		TArray<TTuple<int32, APlayerController*>> TempRequests = MoveTemp(PendingTeamRequests);

		for (const TTuple<int32, APlayerController*>& Request : TempRequests)
		{
			if (IsValid(Request.Value))
			{
				SetTeamForPlayerController(Request.Value, Request.Key);
			}
		}
	}

	// Set up all pending connections
	if (PendingConnectionActors.Num() > 0)
	{
		TArray<AActor*> PendingActors = MoveTemp(PendingConnectionActors);

		for (AActor* Actor : PendingActors)
		{
			if (IsValid(Actor))
			{
				FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(Actor);
				RouteAddNetworkActorToNodes(FNewReplicatedActorInfo(Actor), GlobalInfo);
			}
		}
	}
}

UTutorialConnectionManager* UTutorialRepGraph::GetTutorialConnectionManagerFromActor(const AActor* Actor)
{
	if (Actor)
	{
		if (UNetConnection* NetConnection = Actor->GetNetConnection())
		{
			if (UTutorialConnectionManager* ConnectionManager = Cast<UTutorialConnectionManager>(FindOrAddConnectionManager(NetConnection)))
			{
				return ConnectionManager;
			}
		}
	}
	
	return nullptr;
}
