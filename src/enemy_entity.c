#include "enemy_entity.h"
#include "simple_logger.h"
#include "gfc_vector.h"
#include "math.h"

Entity* enemy_spawn(GFC_Vector3D pos, EnemyType type, Entity* target)
{
    Entity* self = entity_new();
    if (!self) return NULL;

    EnemyData* e = gfc_allocate_array(sizeof(EnemyData), 1);
    self->data = e;

    e->type = type;
    e->target = target;
    e->fireTimer = 0;

    switch (type)
    {
    case ENEMY_TURRET:
        gfc_line_cpy(self->name, "ENEMY_TURRET");
        e->health = 100;
        e->speed = 0;
        weapon_loadout_init(&e->loadout, WEAPON_MACHINE_GUN, 0, 0);
        e->modelPath = "models/enemies/turret.obj";
        e->color = GFC_COLOR_WHITE;
        e->scale = gfc_vector3d(1.5f, 1.5f, 1.5f);
        break;

    case ENEMY_FIGHTER:
        gfc_line_cpy(self->name, "ENEMY_FIGHTER");
        e->health = 60;
        e->speed = 6;
        weapon_loadout_init(&e->loadout, WEAPON_MACHINE_GUN, WEAPON_MISSILE, 0);
        e->modelPath = "models/enemies/plane.obj";
        e->color = GFC_COLOR_WHITE;
        e->scale = gfc_vector3d(1.0f, 1.0f, 1.0f);
        break;

    case ENEMY_BOMBER:
        gfc_line_cpy(self->name, "ENEMY_BOMBER");
        e->health = 120;
        e->speed = 3;
        weapon_loadout_init(&e->loadout, WEAPON_MISSILE, 0, 0);
        e->modelPath = "models/enemies/plane.obj";
        e->color = GFC_COLOR_RED;
        e->scale = gfc_vector3d(2.0f, 2.0f, 2.0f);
        break;

    case ENEMY_INTERCEPTOR:
        gfc_line_cpy(self->name, "ENEMY_INTERCEPTOR");
        e->health = 40;
        e->speed = 8;
        weapon_loadout_init(&e->loadout, WEAPON_MACHINE_GUN, WEAPON_HOMING, 0);
        e->modelPath = "models/enemies/plane.obj";
        e->color = GFC_COLOR_BLUE;
        e->scale = gfc_vector3d(0.8f, 0.8f, 0.8f);
        break;
    }

    // Apply visual settings
    self->mesh = gf3d_mesh_load(e->modelPath);
    self->color = e->color;
    self->scale = e->scale;
    self->position = pos;
    self->rotation = gfc_vector3d(0, 0, 0);

    self->think = enemy_think;
    self->update = enemy_update;
    self->free = entity_free;

    return self;
}


void enemy_think(Entity* self)
{
    if (!self || !self->data) return;
    EnemyData* e = (EnemyData*)self->data;
    float dt = 1.0f / 60.0f;

    e->fireTimer -= dt;
    weapon_update_cooldowns(&e->loadout, dt);

    if (e->fireTimer <= 0 && e->target)
    {
        weapon_fire(self, 0);
        e->fireTimer = e->loadout.weapons[0].fireRate * 2; // pause between bursts
    }
}

void enemy_update(Entity* self)
{
    if (!self || !self->data) return;
    EnemyData* e = (EnemyData*)self->data;

    if (e->speed > 0 && e->target)
    {
        GFC_Vector3D toPlayer;
        gfc_vector3d_sub(toPlayer, e->target->position, self->position);
        gfc_vector3d_normalize(&toPlayer);

        GFC_Vector3D move;
        gfc_vector3d_scale(move, toPlayer, e->speed * (1.0f / 60.0f));
        gfc_vector3d_add(self->position, self->position, move);
    }
}
