#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "gfc_text.h"
#include "gfc_primitives.h"

#include "gf3d_mesh.h"

typedef struct Entity_S
{
	Uint8 _inuse;
	GFC_TextLine name;
	Mesh *mesh;
	Texture *texture;
	GFC_Color color;
	GFC_Matrix4 matrix;
	GFC_Vector3D position;
	GFC_Vector3D rotation;
	GFC_Vector3D scale;
	GFC_Box bounds;
	void (*draw)(Entity_S);
	void (*think)(Entity_S);
	void (*update)(Entity_S);
	void* data;
}Entity;

/**
 * @brief 
 * @param
 * @param 
 * @returns 
 */
Entity* entity_new();

void entity_free(Entity* self);

void entity_system_init(Uint8 max_entities);

void entity_system_close();

void entity_draw(Entity* ent, GFC_Vector3D lightPos, GFC_Color colorMod);
#endif
