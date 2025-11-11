#include "weapon_system.h"
#include "simple_logger.h"
#include "plane_entity.h"
#include "bullet_entity.h"

WeaponStats weapon_get_stats(WeaponType type)
{
    WeaponStats w = { 0 };
    w.type = type;

    switch (type)
    {
    case WEAPON_MACHINE_GUN:
        w.speed = 350;
        w.damage = 10;
        w.lifetime = 1.2f;
        w.fireRate = 0.15f;
        w.homing = 0;
        w.turnRate = 0;
        w.modelPath = "models/bullets/bullet.obj";
        w.color = GFC_COLOR_WHITE;
        break;
    case WEAPON_MISSILE:
        w.speed = 250;
        w.damage = 40;
        w.lifetime = 2.5f;
        w.fireRate = 1.0f;
        w.homing = 0;
        w.turnRate = 0;
        w.modelPath = "models/bullets/missile.obj";
        w.color = GFC_COLOR_WHITE;
        break;
    case WEAPON_HOMING:
        w.speed = 220;
        w.damage = 30;
        w.lifetime = 5.0f;
        w.fireRate = 1.5f;
        w.homing = 1;
        w.turnRate = 0.1f;  // Increased turn rate for better homing
        w.modelPath = "models/bullets/missile.obj";
        w.color = GFC_COLOR_RED;
        break;
    default:
        break;
    }
    return w;
}

void weapon_loadout_init(WeaponLoadout* loadout, WeaponType w1, WeaponType w2, WeaponType w3)
{
    if (!loadout) return;
    loadout->weaponCount = 0;

    WeaponType chosen[3] = { w1, w2, w3 };
    for (int i = 0; i < 3; i++)
    {
        if (chosen[i] < WEAPON_COUNT)
        {
            loadout->weapons[i] = weapon_get_stats(chosen[i]);
            loadout->cooldownTimers[i] = 0;
            loadout->weaponCount++;
        }
    }
}

void weapon_update_cooldowns(WeaponLoadout* loadout, float dt)
{
    if (!loadout) return;
    for (int i = 0; i < loadout->weaponCount; i++)
    {
        if (loadout->cooldownTimers[i] > 0)
            loadout->cooldownTimers[i] -= dt;
    }
}

void weapon_fire(Entity* shooter, int weaponIndex)
{
    if (!shooter || !shooter->data) return;

    PlaneData* data = (PlaneData*)shooter->data;
    WeaponLoadout* loadout = &data->loadout;
    if (weaponIndex >= loadout->weaponCount) return;

    WeaponStats* w = &loadout->weapons[weaponIndex];
    if (loadout->cooldownTimers[weaponIndex] > 0) return;

    loadout->cooldownTimers[weaponIndex] = w->fireRate;

    // Use plane's forward direction (where it's pointing)
    GFC_Vector3D fireDir = data->forward;
    gfc_vector3d_normalize(&fireDir);

    // Spawn bullet ahead of plane
    GFC_Vector3D spawnPos;
    gfc_vector3d_scale(spawnPos, fireDir, 10.0f);  // 10 units ahead
    gfc_vector3d_add(spawnPos, spawnPos, shooter->position);

    // Create bullet
    Entity* bullet = bullet_entity_spawn(spawnPos, fireDir, shooter, *w);

    // For homing missiles, find nearest enemy as target
    if (bullet && w->homing)
    {
        BulletData* b = (BulletData*)bullet->data;

        // Find nearest enemy
        Entity* nearestEnemy = NULL;
        float nearestDist = 999999.0f;

        Uint32 maxEntities = entity_get_max_count();
        for (int i = 0; i < maxEntities; i++)
        {
            Entity* e = entity_get_by_index(i);
            if (!e) continue;

            // Check if it's an enemy
            int isEnemy = (strcmp(e->name, "Light Turret") == 0 ||
                strcmp(e->name, "Heavy Turret") == 0 ||
                strcmp(e->name, "Fighter") == 0 ||
                strcmp(e->name, "Bomber") == 0 ||
                strcmp(e->name, "Interceptor") == 0);

            if (isEnemy) {
                float dist = gfc_vector3d_magnitude_between(shooter->position, e->position);
                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearestEnemy = e;
                }
            }
        }

        if (nearestEnemy) {
            b->target = nearestEnemy;
            slog("Homing missile locked on to %s at dist %.1f", nearestEnemy->name, nearestDist);
        }
    }

    slog("Player fired weapon %d", weaponIndex);
}