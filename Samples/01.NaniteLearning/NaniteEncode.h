/*
	TiX Engine v3.0 Copyright (C) 2022~2025
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