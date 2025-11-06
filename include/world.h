#ifndef __WORLDD_H_
#define __WORLDD_H_

#include "gf3d_mesh.h"

typedef struct
{
	Mesh*			mesh;
	Texture			*Texture;
	GFC_Color		lightcolor;
	GFC_Vector3D	lightpos;
	GFC_List		entityList;
}World;

World* world_new();

World *world_load(const char *filename);

void world_free(World* world);

void world_draw(World* world);

World* get_the_world();

Uint8 world_edge_test(World* world, GFC_Vector3D start, GFC_Vector3D end, GFC_Vector3D* contact);
#endif
