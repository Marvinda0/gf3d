#include "enemy_entity.h"
#include "simple_logger.h"
#include "bullet_entity.h"
#include "gfc_vector.h"
#include "weapon_system.h"
#include "data_definitions.h"
#include <math.h>

Entity* enemy_spawn(GFC_Vector3D pos, EnemyType type, Entity* target)
{
    Entity* self = entity_new();
    if (!self) return NULL;

    EnemyData* e = gfc_allocate_array(sizeof(EnemyData), 1);
    self->data = e;

    e->type = type;
    e->target = target;
    e->fireTimer = 0;
    e->forward = gfc_vector3d(0, 1, 0);

    // Load definition from JSON
    EnemyDefinition* def = data_get_enemy_def(type);
    if (!def) {
        slog("ERROR: Failed to get enemy definition for type %d", type);
        entity_free(self);
        return NULL;
    }

    // Copy from definition
    gfc_line_cpy(self->name, def->name);
    e->health = def->health;
    e->maxHealth = def->health;
    e->speed = def->speed;
    e->fireRate = def->fireRate;
    e->modelPath = def->modelPath;
    e->color = def->color;
    e->scale = def->scale;

    // Convert weapon name to WeaponType enum
    WeaponType weaponType = WEAPON_MACHINE_GUN; // default
    if (strcmp(def->weaponType, "machine_gun") == 0) {
        weaponType = WEAPON_MACHINE_GUN;
    }
    else if (strcmp(def->weaponType, "missile") == 0) {
        weaponType = WEAPON_MISSILE;
    }
    else if (strcmp(def->weaponType, "homing") == 0) {
        weaponType = WEAPON_HOMING;
    }
    weapon_loadout_init(&e->loadout, weaponType, 0, 0);

    // Load mesh and set entity properties
    self->mesh = gf3d_mesh_load(e->modelPath);

    self->color = e->color;
    self->scale = e->scale;
    self->position = pos;
    self->rotation = gfc_vector3d(0, 0, 0);
    slog("Enemy %s: mesh=%p, color=(%.2f,%.2f,%.2f,%.2f), scale=(%.2f,%.2f,%.2f)",
        self->name, self->mesh,
        e->color.r, e->color.g, e->color.b, e->color.a,
        e->scale.x, e->scale.y, e->scale.z);
    self->color = e->color;

    // Ground turrets to terrain
    if (e->type == ENEMY_LIGHT_TURRET || e->type == ENEMY_HEAVY_TURRET)
    {
        GFC_Vector3D contact;
        if (entity_get_floor_position(self, get_the_world(), &contact))
        {
            self->position.z = contact.z;
        }
    }

    // Set up bounds
    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    if (self->bounds) {
        self->bounds->x = pos.x;
        self->bounds->y = pos.y;
        self->bounds->z = pos.z;
        self->bounds->w = 10.0f * e->scale.x;
        self->bounds->h = 10.0f * e->scale.y;
        self->bounds->d = 10.0f * e->scale.z;
    }
    if (def->speed == 0.0f) {  // Stationary turrets
        // Make collision box TALL so bullets can hit from above
        self->bounds->z = self->position.z + 10.0f;  // Center of box higher
        self->bounds->d = 30.0f;  // Make it tall
    }

    self->think = enemy_think;
    self->update = enemy_update;
    self->free = enemy_free;

    return self;
}

void enemy_think(Entity* self)
{
    if (!self || !self->data) return;
    EnemyData* e = (EnemyData*)self->data;
    if (!e->target || !e->target->_inuse) return;

    float dt = 1.0f / 60.0f;
    weapon_update_cooldowns(&e->loadout, dt);
    e->fireTimer -= dt;

    // Direction to player
    GFC_Vector3D toPlayer;
    gfc_vector3d_sub(toPlayer, e->target->position, self->position);
    gfc_vector3d_normalize(&toPlayer);

    float turnRate = 2.0f * dt;
    GFC_Vector3D diff;
    gfc_vector3d_sub(diff, toPlayer, e->forward);
    gfc_vector3d_scale(diff, diff, turnRate);
    gfc_vector3d_add(e->forward, e->forward, diff);
    gfc_vector3d_normalize(&e->forward);

    float dot = e->forward.x * toPlayer.x + e->forward.y * toPlayer.y + e->forward.z * toPlayer.z;
    if (dot > 0.75f && e->fireTimer <= 0)
    {
        enemy_fire_weapon(self, 0);
        e->fireTimer = e->fireRate;
    }
}

void enemy_update(Entity* self)
{
    if (!self || !self->data) return;
    EnemyData* e = (EnemyData*)self->data;
    if (e->speed <= 0) return;

    float dt = 1.0f / 60.0f;

    // --- Always move forward ---
    GFC_Vector3D move;
    gfc_vector3d_scale(move, e->forward, e->speed * dt);
    gfc_vector3d_add(self->position, self->position, move);

    // Simple horizontal rotation (visual turn)
    self->rotation.z = atan2f(e->forward.x, e->forward.y);

    // Update bounds
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
}

void enemy_fire_weapon(Entity* self, int weaponIndex)
{
    if (!self || !self->data) return;

    EnemyData* e = (EnemyData*)self->data;
    if (weaponIndex >= e->loadout.weaponCount) return;
    if (!e->target || !e->target->_inuse) return;

    WeaponStats* w = &e->loadout.weapons[weaponIndex];
    if (e->loadout.cooldownTimers[weaponIndex] > 0) return;

    e->loadout.cooldownTimers[weaponIndex] = w->fireRate;

    GFC_Vector3D dirToPlayer;
    gfc_vector3d_sub(dirToPlayer, e->target->position, self->position);
    gfc_vector3d_normalize(&dirToPlayer);

    GFC_Vector3D spawnPos;
    gfc_vector3d_scale(spawnPos, dirToPlayer, 12.0f);
    gfc_vector3d_add(spawnPos, spawnPos, self->position);

    Entity* bullet = bullet_entity_spawn(spawnPos, dirToPlayer, self, *w);

    if (bullet && w->homing && e->target)
    {
        BulletData* b = (BulletData*)bullet->data;
        b->target = e->target;
    }
}

void enemy_take_damage(Entity* self, int damage)
{
    if (!self || !self->data) return;
    EnemyData* e = (EnemyData*)self->data;
    e->health -= damage;

    slog("%s took %d damage! HP: %d/%d", self->name, damage, e->health, e->maxHealth);

    if (e->health <= 0)
    {
        slog("%s destroyed!", self->name);
        entity_free(self);
    }
}


void enemy_free(Entity* self)
{
    if (!self) return;

    if (self->data)
    {
        free(self->data);
        self->data = NULL;
    }

    self->_inuse = 0;
}