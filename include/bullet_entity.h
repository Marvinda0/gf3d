#ifndef __BULLET_ENTITY_H__
#define __BULLET_ENTITY_H__

#include "gf3d_entity.h"
#include "weapon_system.h"

typedef struct
{
    WeaponStats stats;
    float age;
    Entity* owner;    // Who fired this bullet
    Entity* target;   // For homing missiles
} BulletData;

/**
 * @brief Spawns a bullet
 * @param pos Starting position
 * @param forward Direction to fire
 * @param owner Who fired it (for collision exclusion)
 * @param stats Weapon stats (speed, damage, etc)
 * @return Pointer to bullet entity
 */
Entity* bullet_entity_spawn(GFC_Vector3D pos, GFC_Vector3D forward, Entity* owner, WeaponStats stats);

/**
 * @brief Bullet AI - handles homing and expiration
 */
void bullet_entity_think(Entity* self);

/**
 * @brief Updates bullet position
 */
void bullet_entity_update(Entity* self);

/**
 * @brief Checks if bullet hit anything
 */
void bullet_check_collisions(Entity* self);

/**
 * @brief Frees bullet data
 */
void bullet_entity_free(Entity* self);

#endif