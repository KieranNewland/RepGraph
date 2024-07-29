#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "TutorialRepGraph.generated.h"

UCLASS()
class UReplicationGraphNode_AlwaysRelevant_WithPending : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:
	UReplicationGraphNode_AlwaysRelevant_WithPending();
	virtual void PrepareForReplication() override;
};

UCLASS()
class UReplicationGraphNode_AlwaysRelevant_ForTeam : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;
	virtual void GatherActorListsForConnectionDefault(const FConnectionGatherActorListParameters& Params);
};

UCLASS()
class UTutorialConnectionGraph : public UNetReplicationGraphConnection
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantForConnectionNode;
	
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_ForTeam* TeamConnectionNode;

	int32 Team = -1;

	UPROPERTY()
	TWeakObjectPtr<APawn> Pawn = nullptr;
};

struct FTeamConnectionListMap : TMap<int32, TArray<UTutorialConnectionGraph*>>
{
	TArray<UTutorialConnectionGraph*>* GetConnectionArrayForTeam(int32 Team);
	TArray<UTutorialConnectionGraph*> GetVisibleConnectionArrayForNonTeam(const APawn* Pawn, int32 Team);
	
	void AddConnectionToTeam(int32 Team, UTutorialConnectionGraph* ConnManager);
	void RemoveConnectionFromTeam(int32 Team, UTutorialConnectionGraph* ConnManager);
};

UCLASS(Blueprintable)
class REPGRAPH_API UTutorialRepGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:
	UTutorialRepGraph();

	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager) override;

	virtual void RemoveClientConnection(UNetConnection* NetConnection) override;
	virtual void ResetGameWorldState() override;
	
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

	void SetTeamForPlayerController(APlayerController* PlayerController, int32 Team);
	void HandlePendingActorsAndTeamRequests();

private:
	UPROPERTY()
	UReplicationGraphNode_AlwaysRelevant_WithPending* AlwaysRelevantNode;
	
	UPROPERTY()
	TArray<AActor*> PendingConnectionActors;
	TArray<TTuple<int32, APlayerController*>> PendingTeamRequests;

	friend UReplicationGraphNode_AlwaysRelevant_ForTeam;
	FTeamConnectionListMap TeamConnectionListMap;
	
	UTutorialConnectionGraph* GetTutorialConnectionGraphFromActor(const AActor* Actor);
};
