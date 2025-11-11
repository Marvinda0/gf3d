#include "weapon_system.h"
#include "simple_logger.h"
#include "camera_entity.h"  
#include "plane_entity.h"

WeaponStats weapon_get_stats(WeaponType type)
{
    WeaponStats w = { 0 };
    w.type = type;

    switch (type)
    {
    case WEAPON_MACHINE_GUN:
        w.speed = 120;
        w.damage = 10;
        w.lifetime = 2;
        w.fireRate = 0.12f;
        w.homing = 0;
        w.turnRate = 0;
        w.modelPath = "models/cube.obj";
        w.color = GFC_COLOR_WHITE;
        break;
    case WEAPON_MISSILE:
        w.speed = 70;
        w.damage = 40;
        w.lifetime = 5;
        w.fireRate = 1.0f;
        w.homing = 0;
        w.turnRate = 0;
        w.modelPath = "models/bullets/missile.obj";
        w.color = GFC_COLOR_WHITE;
        break;
    case WEAPON_HOMING:
        w.speed = 60;
        w.damage = 30;
        w.lifetime = 8;
        w.fireRate = 1.2f;
        w.homing = 1;
        w.turnRate = 0.05f;
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
    slog("WEAPON FIRE");
    if (!shooter || !shooter->data) return;

    PlaneData* data = (PlaneData*)shooter->data;
    WeaponLoadout* loadout = &data->loadout;
    if (weaponIndex >= loadout->weaponCount) return;

    WeaponStats* w = &loadout->weapons[weaponIndex];
    if (loadout->cooldownTimers[weaponIndex] > 0) return;

    loadout->cooldownTimers[weaponIndex] = w->fireRate;

    CameraEntityData* camData = NULL;
    if (data->camera && data->camera->data) {
        camData = (CameraEntityData*)data->camera->data;
    }

    GFC_Vector3D fireDir = data->forward;
    if (camData) {
        fireDir = camData->forward; // use camera's look direction
    }

    GFC_Vector3D spawnPos;
    gfc_vector3d_scale(spawnPos, fireDir, 6.0f);
    gfc_vector3d_add(spawnPos, spawnPos, shooter->position);

    bullet_entity_spawn(spawnPos, fireDir, shooter, *w);
}  
