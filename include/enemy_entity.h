#ifndef __ENEMY_ENTITY_H__
#define __ENEMY_ENTITY_H__

#include "gf3d_entity.h"
#include "weapon_system.h"

typedef enum
{
    ENEMY_LIGHT_TURRET,   // Low HP, fast fire, stationary
    ENEMY_HEAVY_TURRET,   // High HP, slow heavy fire, stationary
    ENEMY_FIGHTER,        // Medium HP, medium speed, machine gun + missiles
    ENEMY_BOMBER,         // High HP, slow, heavy missiles
    ENEMY_INTERCEPTOR,    // Low HP, very fast, homing missiles
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct
{
    EnemyType type;
    int health;
    int maxHealth;
    float speed;
    float fireRate;
    float fireTimer;

    GFC_Vector3D forward;     // Direction enemy is facing
    Entity* target;           // What enemy is shooting at
    WeaponLoadout loadout;    // Enemy's weapons

    const char* modelPath;
    GFC_Color color;
    GFC_Vector3D scale;
} EnemyData;

/**
 * @brief Spawns an enemy
 * @param pos Starting position
 * @param type Enemy type
 * @param target Entity to attack (usually the player)
 * @return Pointer to enemy entity
 */
Entity* enemy_spawn(GFC_Vector3D pos, EnemyType type, Entity* target);

/**
 * @brief Enemy AI logic - handles targeting and firing
 */
void enemy_think(Entity* self);

/**
 * @brief Updates enemy movement
 */
void enemy_update(Entity* self);

/**
 * @brief Fires enemy weapon
 * @param weaponIndex Which weapon to fire (0-2)
 */
void enemy_fire_weapon(Entity* self, int weaponIndex);

/**
 * @brief Damages an enemy
 * @param damage Amount of damage to deal
 */
void enemy_take_damage(Entity* self, int damage);

/**
 * @brief Frees enemy data
 */
void enemy_free(Entity* self);

#endif