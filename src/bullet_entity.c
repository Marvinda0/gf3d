#include "bullet_entity.h"
#include "simple_logger.h"
#include "gfc_vector.h"
#include "math.h"

Entity* bullet_entity_spawn(GFC_Vector3D pos, GFC_Vector3D forward, Entity* owner, WeaponStats stats)
{
    slog("BULLET SPAWN");
    Entity* self = entity_new();
    if (!self) return NULL;

    gfc_line_cpy(self->name, "Bullet");
    slog("NAME Bullet");
    self->mesh = gf3d_mesh_load(stats.modelPath);
    slog("MODEL SPAWN");
    self->color = stats.color;
    self->position = pos;
    self->scale = gfc_vector3d(0.5f, 0.5f, 0.5f);
    self->rotation = gfc_vector3d(0, 0, 0);
    slog("other");
    self->data = gfc_allocate_array(sizeof(BulletData), 1);
    slog("DATA SPAWN");
    BulletData* b = (BulletData*)self->data;
    b->stats = stats;
    b->age = 0;
    b->owner = owner;
    b->target = NULL;

    self->think = bullet_entity_think;
    self->update = bullet_entity_update;
    self->free = self->free = bullet_entity_free;;

    self->velocity = forward;

    slog("Spawned bullet at (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    return self;
}

void bullet_entity_think(Entity* self)
{
    if (!self || !self->data) return;
    BulletData* b = (BulletData*)self->data;

    float dt = 1.0f / 60.0f;
    b->age += dt;

    if (b->age > b->stats.lifetime)
    {
        entity_free(self);
        return;
    }

    // Homing logic
    if (b->stats.homing && b->target)
    {
        GFC_Vector3D toTarget;
        gfc_vector3d_sub(toTarget, b->target->position, self->position);
        gfc_vector3d_normalize(&toTarget);

        // Smooth turn toward target
        GFC_Vector3D diff;  
        gfc_vector3d_sub(diff, toTarget, self->velocity);              // diff = target - current
        gfc_vector3d_scale(diff, diff, b->stats.turnRate);            // diff *= turnRate
        gfc_vector3d_add(self->velocity, self->velocity, diff);      // current += diff
        gfc_vector3d_normalize(&self->velocity);                   // re-normalize
    }
}

void bullet_entity_update(Entity* self)
{
    if (!self || !self->data) return;
    BulletData* b = (BulletData*)self->data;

    float dt = 1.0f / 60.0f;
    GFC_Vector3D move;
    gfc_vector3d_scale(move, self->velocity, b->stats.speed * dt);
    gfc_vector3d_add(self->position, self->position, move);
}
void bullet_entity_free(Entity* self)
{
    if (!self) return;

    slog("Freeing bullet: %s", self->name);

    if (self->data)
    {
        free(self->data);
        self->data = NULL;
    }

    // You don’t free the mesh here — the entity system will do that.
    // Just mark as not in use.
    self->_inuse = 0;
}
