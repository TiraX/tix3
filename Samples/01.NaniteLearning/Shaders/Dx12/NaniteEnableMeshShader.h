
#pragma once

#define NANITE_TREE_NO_TESS	(0)
#define NANITE_GRID_WITH_TESS (1)

#define NANITE_DEMO NANITE_TREE_NO_TESS

#if NANITE_DEMO == NANITE_TREE_NO_TESS
#define NANITE_MESH_SHADER 0
#elif NANITE_DEMO == NANITE_GRID_WITH_TESS
#define NANITE_MESH_SHADER 1
#else
#pragma error
#endif

#define USE_AS_SHADER 1		// This is a AS test, can be removed now.
#define DO_TESS (NANITE_MESH_SHADER && (1))
