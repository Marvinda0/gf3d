#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "gfc_text.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"
#include "world.h"
#include "gf3d_mesh.h"
#include "gf3d_texture.h"

typedef struct Entity_S
{
	Uint8			_inuse;
	GFC_TextLine	name; 
	Mesh			*mesh; // mesh to draw for the entity
	Texture			*texture; // texture to apply to the mesh
	GFC_Color		color; // color tint to apply to the entity
	GFC_Matrix4		matrix; // cached matrix for rendering
	GFC_Vector3D	position; // where the entity is located
	GFC_Vector3D	rotation; // in radians
	GFC_Vector3D	scale; // how big the entity is
	GFC_Vector3D	velocity; // movement speed
	float speed;

	GFC_Box *bounds; // collision bounds
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
void entity_system_init(Uint32 max_entities);

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

/*
* @brief this function gets the floor position under the entity
* @param self: the entity to check
* @param world: the world to check against
* @param contact: the position of the floor under the entity
*/
Uint8 entity_get_floor_position(Entity* self, World *world, GFC_Vector3D* contact);
#endif


// 	return world_get_floor_position(world, self->position, contact);


/*
* @brief this function checks for sphere-sphere collision between two entities
 * @param a: the first entity
 * @param b: the second entity
 * @param radiusA: the radius of the first entity's sphere
 * @param radiusB: the radius of the second entity's sphere
 * @returns 1 if the entities are colliding, 0 otherwise
*/
Uint8 entity_sphere_collision(Entity* a, Entity* b, float radiusA, float radiusB);