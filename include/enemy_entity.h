#ifndef __ENEMY_ENTITY_H__
#define __ENEMY_ENTITY_H__

#include "gf3d_entity.h"
#include "weapon_system.h"

typedef enum {
    ENEMY_TURRET = 0,
    ENEMY_FIGHTER,
    ENEMY_BOMBER,
    ENEMY_INTERCEPTOR
} EnemyType;

typedef struct {
    GFC_TextLine name;
    EnemyType type;
    float health;
    float speed;
    float fireCooldown;
    float fireTimer;
    WeaponLoadout loadout;
    Entity* target;

    const char* modelPath;
    GFC_Color color;
    GFC_Vector3D scale;
} EnemyData;

Entity* enemy_spawn(GFC_Vector3D pos, EnemyType type, Entity* target);
void enemy_think(Entity* self);
void enemy_update(Entity* self);

#endif
