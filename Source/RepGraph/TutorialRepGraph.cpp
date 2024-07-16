#include "TutorialRepGraph.h"

UTutorialRepGraph::UTutorialRepGraph()
{
	ReplicationConnectionManagerClass = UTutorialConnectionGraph::StaticClass();
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

void UTutorialRepGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
	FGlobalActorReplicationInfo& GlobalInfo)
{
	if (const UTutorialConnectionGraph* ConnectionGraph = GetTutorialConnectionGraphFromActor(ActorInfo.GetActor()))
	{
		ConnectionGraph->TeamConnectionNode->NotifyAddNetworkActor(ActorInfo);
	}
}

void UTutorialRepGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	if (const UTutorialConnectionGraph* ConnectionGraph = GetTutorialConnectionGraphFromActor(ActorInfo.GetActor()))
	{
		ConnectionGraph->TeamConnectionNode->NotifyRemoveNetworkActor(ActorInfo);
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
