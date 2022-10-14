/*
	TiX Engine v2.0 Copyright (C) 2018~2021
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once
#include "Cluster.h"

class TNaniteMesh;
void Encode(
	TNaniteMesh& Mesh,
	TVector<FClusterGroup>& Groups, 
	TVector<FCluster>& ClusterSources,
	TVector< FClusterInstance>& ClusterInstances
);