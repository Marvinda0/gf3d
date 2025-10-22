#ifndef __MONSTER_H__
#define __MONSTER_H__

#include "gf3d_entity.h"

/**
 * @brief this function spawns a monster entity at the given position
 * @param position the position to spawn the monster at
 * @returns a pointer to the spawned monster entity, or NULL on failure
 */
Entity* monster_spawn(GFC_Vector3D position, GFC_Color color);

void monster_think(Entity* self);

void monster_update(Entity* self);


#endif

