#include "TutorialRepGraph.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

UReplicationGraphNode_AlwaysRelevant_WithPending::UReplicationGraphNode_AlwaysRelevant_WithPending()
{
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
	const UTutorialConnectionGraph* ConnectionGraph = Cast<UTutorialConnectionGraph>(&Params.ConnectionManager);
	if (ReplicationGraph && ConnectionGraph && ConnectionGraph->Team != -1)
	{
		if (TArray<UTutorialConnectionGraph*>* TeamConnections = ReplicationGraph->TeamConnectionListMap.GetConnectionArrayForTeam(ConnectionGraph->Team))
		{
			for (const UTutorialConnectionGraph* TeamMember : *TeamConnections)
			{
				TeamMember->TeamConnectionNode->GatherActorListsForConnectionDefault(Params);
			}
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

TArray<UTutorialConnectionGraph*>* FTeamConnectionListMap::GetConnectionArrayForTeam(int32 Team)
{
	return Find(Team);
}

void FTeamConnectionListMap::AddConnectionToTeam(int32 Team, UTutorialConnectionGraph* ConnManager)
{
	TArray<UTutorialConnectionGraph*>& TeamList = FindOrAdd(Team);
	TeamList.Add(ConnManager);
}

void FTeamConnectionListMap::RemoveConnectionFromTeam(int32 Team, UTutorialConnectionGraph* ConnManager)
{
	if (TArray<UTutorialConnectionGraph*>* TeamList = Find(Team))
	{
		TeamList->RemoveSwap(ConnManager);
		
		if (TeamList->Num() == 0)
		{
			Remove(Team);
		}
	}
}

UTutorialRepGraph::UTutorialRepGraph()
{
	ReplicationConnectionManagerClass = UTutorialConnectionGraph::StaticClass();
}

void UTutorialRepGraph::InitGlobalGraphNodes()
{
	Super::InitGlobalGraphNodes();

	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_WithPending>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UTutorialRepGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager)
{
	Super::InitConnectionGraphNodes(ConnectionManager);
	
	UTutorialConnectionGraph* TutorialRepGraph = Cast<UTutorialConnectionGraph>(ConnectionManager);

	if (ensure(TutorialRepGraph))
	{
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
			UTutorialConnectionGraph* ConnectionManager = Cast<UTutorialConnectionGraph>(Connections[idx]);
			repCheck(ConnectionManager);

			if (ConnectionManager->NetConnection == NetConnection)
			{
				ensure(!bFound);

				if (ConnectionManager->Team != -1)
				{
					TeamConnectionListMap.RemoveConnectionFromTeam(ConnectionManager->Team, ConnectionManager);
				}

				List.RemoveAtSwap(idx, 1, false);
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
}

void UTutorialRepGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
                                                    FGlobalActorReplicationInfo& GlobalInfo)
{
	// All clients must receive game states and player states
	if (ActorInfo.Class->IsChildOf(AGameStateBase::StaticClass()) || ActorInfo.Class->IsChildOf(APlayerState::StaticClass()))
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
	}
	else if (const UTutorialConnectionGraph* ConnectionGraph = GetTutorialConnectionGraphFromActor(ActorInfo.GetActor()))
	{
		ConnectionGraph->TeamConnectionNode->NotifyAddNetworkActor(ActorInfo);
	}
	else if(ActorInfo.Actor->GetNetOwner())
	{
		PendingConnectionActors.Add(ActorInfo.GetActor());
	}
}

void UTutorialRepGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	if (ActorInfo.Class->IsChildOf(AGameStateBase::StaticClass()) || ActorInfo.Class->IsChildOf(APlayerState::StaticClass()))
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
	}
	else if (const UTutorialConnectionGraph* ConnectionGraph = GetTutorialConnectionGraphFromActor(ActorInfo.GetActor()))
	{
		ConnectionGraph->TeamConnectionNode->NotifyRemoveNetworkActor(ActorInfo);
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
		if (UTutorialConnectionGraph* ConnectionGraph = GetTutorialConnectionGraphFromActor(PlayerController))
		{
			const int32 CurrentTeam = ConnectionGraph->Team;
			if (CurrentTeam != Team)
			{
				if (CurrentTeam != -1)
				{
					TeamConnectionListMap.RemoveConnectionFromTeam(CurrentTeam, ConnectionGraph);
				}

				if (Team != -1)
				{
					TeamConnectionListMap.AddConnectionToTeam(Team, ConnectionGraph);
				}
				ConnectionGraph->Team = Team;
			}
		}
		else
		{
			PendingTeamRequests.Emplace(Team, PlayerController);
		}
	}
}

void UTutorialRepGraph::HandlePendingActorsAndTeamRequests()
{
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

UTutorialConnectionGraph* UTutorialRepGraph::GetTutorialConnectionGraphFromActor(const AActor* Actor)
{
	if (Actor)
	{
		if (UNetConnection* NetConnection = Actor->GetNetConnection())
		{
			if (UTutorialConnectionGraph* ConnectionGraph = Cast<UTutorialConnectionGraph>(FindOrAddConnectionManager(NetConnection)))
			{
				return ConnectionGraph;
			}
		}
	}
	
	return nullptr;
}
