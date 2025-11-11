#include "bullet_entity.h"
#include "enemy_entity.h"
#include "plane_entity.h"
#include "simple_logger.h"
#include "gfc_vector.h"
#include <math.h>

Entity* bullet_entity_spawn(GFC_Vector3D pos, GFC_Vector3D forward, Entity* owner, WeaponStats stats)
{
    Entity* self = entity_new();
    if (!self) return NULL;

    gfc_line_cpy(self->name, "Bullet");
    self->mesh = gf3d_mesh_load(stats.modelPath);
    self->color = stats.color;
    self->position = pos;
    self->scale = gfc_vector3d(1, 1, 1);
    self->rotation = gfc_vector3d(0, 0, 0);

    self->data = gfc_allocate_array(sizeof(BulletData), 1);
    if (!self->data) {
        entity_free(self);
        return NULL;
    }

    BulletData* b = (BulletData*)self->data;
    b->stats = stats;
    b->age = 0;
    b->owner = owner;
    b->target = NULL;

    self->think = bullet_entity_think;
    self->update = bullet_entity_update;
    self->free = bullet_entity_free;
    self->draw = entity_draw;

    // Normalize and set velocity - FORWARD!
    gfc_vector3d_normalize(&forward);
    self->velocity = forward;

    // Set up collision bounds - BIGGER so they actually hit things
    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    if (self->bounds) {
        self->bounds->x = pos.x;
        self->bounds->y = pos.y;
        self->bounds->z = pos.z;
        self->bounds->w = 6.0f;  // Made bigger
        self->bounds->h = 6.0f;
        self->bounds->d = 6.0f;
    }

    return self;
}

void bullet_entity_think(Entity* self)
{
    if (!self || !self->data) return;
    BulletData* b = (BulletData*)self->data;

    float dt = 1.0f / 60.0f;
    b->age += dt;

    // Check lifetime
    if (b->age > b->stats.lifetime)
    {
        entity_free(self);
        return;
    }

    // Homing logic
    if (b->stats.homing && b->target && b->target->_inuse)
    {
        GFC_Vector3D toTarget;
        gfc_vector3d_sub(toTarget, b->target->position, self->position);
        gfc_vector3d_normalize(&toTarget);

        // Smoothly adjust toward target regardless of distance
        GFC_Vector3D diff;
        gfc_vector3d_sub(diff, toTarget, self->velocity);
        gfc_vector3d_scale(diff, diff, b->stats.turnRate);
        gfc_vector3d_add(self->velocity, self->velocity, diff);
        gfc_vector3d_normalize(&self->velocity);
    }


    // Check collisions every frame
    bullet_check_collisions(self);
}

void bullet_entity_update(Entity* self)
{
    if (!self || !self->data) return;
    BulletData* b = (BulletData*)self->data;

    float dt = 1.0f / 60.0f;
    GFC_Vector3D move;
    gfc_vector3d_scale(move, self->velocity, b->stats.speed * dt);
    gfc_vector3d_add(self->position, self->position, move);

    // Update bounds
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
}

void bullet_check_collisions(Entity* self)
{
    if (!self || !self->data || !self->bounds) return;

    BulletData* b = (BulletData*)self->data;
    Entity* owner = b->owner;

    // Get max entities from system
    Uint32 maxEntities = entity_get_max_count();

    for (int i = 0; i < maxEntities; i++)
    {
        Entity* other = entity_get_by_index(i);
        if (!other) continue;
        if (other == self) continue;
        if (other == owner) continue;  // Don't hit shooter
        if (!other->bounds) continue;
        if (strcmp(other->name, "Bullet") == 0) continue;  // Don't hit other bullets

        // Check collision
        if (gfc_box_overlap(*self->bounds, *other->bounds))
        {
            // Check if it's an enemy
            int isEnemy = (strcmp(other->name, "Light Turret") == 0 ||
                strcmp(other->name, "Heavy Turret") == 0 ||
                strcmp(other->name, "Fighter") == 0 ||
                strcmp(other->name, "Bomber") == 0 ||
                strcmp(other->name, "Interceptor") == 0);

            int isPlayer = (strcmp(other->name, "PlayerPlane") == 0);

            if (isEnemy) {
                // Player bullet hit enemy
                slog(">>> PLAYER BULLET HIT %s! <<<", other->name);
                enemy_take_damage(other, b->stats.damage);
                entity_free(self);
                return;
            }
            else if (isPlayer) {
                // Enemy bullet hit player
                slog(">>> ENEMY BULLET HIT PLAYER! <<<");
                plane_take_damage(other, b->stats.damage);
                entity_free(self);
                return;
            }
        }
    }
}

void bullet_entity_free(Entity* self)
{
    if (!self) return;

    if (self->data)
    {
        free(self->data);
        self->data = NULL;
    }

    self->_inuse = 0;
}