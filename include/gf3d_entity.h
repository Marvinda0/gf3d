#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "gfc_text.h"
#include "gfc_primitives.h"

#include "gf3d_mesh.h"

typedef struct Entity_S
{
	Uint8			_inuse;
	GFC_TextLine	name;
	Mesh			*mesh;
	Texture			*texture;
	GFC_Color		color;
	GFC_Matrix4		matrix;
	GFC_Vector3D	position;
	GFC_Vector3D	rotation;
	GFC_Vector3D	scale;
	GFC_Vector3D	speed;

	GFC_Box bounds;
	void (*draw)(struct Entity_S* self);
	void (*think)(struct Entity_S* self);
	void (*update)(struct Entity_S *self);
	void (*free)(struct Entity_S* self);
	Uint8 doGenericUpdate;
	void* data;
}Entity;

/**
 * @brief this function creates a new entity
 * @returns the newly created entity, or NULL on failure
 */
Entity* entity_new();

/*
 * @brief this function frees the given entity
 * @param self: the entity to free
 * @returns void
 */
void entity_free(Entity* self);

/**
 * @brief this function initializes the entity system
 * @param max_entities the maximum number of entities to support
 * @returns void
 */
void entity_system_init(Uint8 max_entities);

/**
 * @brief this function closes the entity system and frees all entities
 * @returns void
 */
void entity_system_close();

/**
 * @brief this function draws the given entity
 * @param ent the entity to draw
 * @param lightPos the position of the light source
 * @param colorMod a color modifier to apply to the entity
 * @returns void
 */
void entity_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod);

/**
 * @brief this function draws all entities in the entity system
 * @returns void
 */
void entity_draw_all();

/**
 * @brief this function calls the think function of the given entity
 * @param ent the entity to think
 * @returns void
 */

void entity_think(Entity* self);

/**
 * @brief this function calls the think function of all entities in the entity system
 * @returns void
 */
void entity_think_all();

/**
 * @brief this function updates the given entity
 * @param ent the entity to update
 * @returns void
 */
void entity_update(Entity* self);

/**
 * @brief this function updates all entities in the entity system
 * @returns void
 */
void entity_update_all();

#endif


