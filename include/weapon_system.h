#ifndef __WEAPON_SYSTEM_H__
#define __WEAPON_SYSTEM_H__

#include "gf3d_entity.h"

typedef enum {
    WEAPON_MACHINE_GUN = 0, 
    WEAPON_MISSILE,
    WEAPON_HOMING,
    WEAPON_COUNT
} WeaponType;

typedef struct WeaponStats {
    WeaponType type;
    float speed;
    float damage;
    float lifetime;
    float fireRate;
    int homing;
    float turnRate; 
    const char* modelPath;
    GFC_Color color;
} WeaponStats;

Entity* bullet_entity_spawn(GFC_Vector3D pos, GFC_Vector3D forward, Entity* owner, WeaponStats stats);

typedef struct {
    WeaponStats weapons[3];
    float cooldownTimers[3];
    int weaponCount;
} WeaponLoadout;

WeaponStats weapon_get_stats(WeaponType type);
void weapon_loadout_init(WeaponLoadout* loadout, WeaponType w1, WeaponType w2, WeaponType w3);
void weapon_update_cooldowns(WeaponLoadout* loadout, float dt);
void weapon_fire(Entity* shooter, int weaponIndex);

#endif
