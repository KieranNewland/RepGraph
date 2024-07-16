#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "TutorialRepGraph.generated.h"

UCLASS()
class UReplicationGraphNode_AlwaysRelevant_ForTeam : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()
};

UCLASS()
class UTutorialConnectionGraph : public UNetReplicationGraphConnection
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_ForTeam* TeamConnectionNode;

	FName TeamName = NAME_None;
};

UCLASS(Blueprintable)
class REPGRAPH_API UTutorialRepGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:
	UTutorialRepGraph();
	
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

private:
	UTutorialConnectionGraph* GetTutorialConnectionGraphFromActor(const AActor* Actor);
};
