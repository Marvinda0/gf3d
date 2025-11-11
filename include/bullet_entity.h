#ifndef __BULLET_ENTITY_H__
#define __BULLET_ENTITY_H__

#include "gf3d_entity.h"
#include "weapon_system.h"

typedef struct {
    WeaponStats stats;
    float age;
    Entity* owner;
    Entity* target; // for homing


} BulletData;

Entity* bullet_entity_spawn(GFC_Vector3D pos, GFC_Vector3D forward, Entity* owner, WeaponStats stats);
void bullet_entity_think(Entity* self);
void bullet_entity_update(Entity* self);
void bullet_entity_free(Entity* self);

#endif
