#include "simple_logger.h"
#include "item_entity.h"
#include "plane_entity.h"
#include <math.h>

Entity* item_spawn(GFC_Vector3D position, ItemType type) {
    ItemDefinition* def = data_get_item_def(type);
    if (!def) {
        slog("Failed to get item definition for type %d", type);
        return NULL;
    }

    Entity* self = entity_new();
    if (!self) {
        slog("Failed to create item entity");
        return NULL;
    }

    gfc_line_cpy(self->name, "Item");

    // Load model (or use default if missing)
    self->mesh = gf3d_mesh_load(def->modelPath);
    if (!self->mesh) {
        slog("Warning: Failed to load item model %s, using fallback", def->modelPath);
        // Try cube as fallback
        self->mesh = gf3d_mesh_load("models/cube.obj");
    }

    self->position = position;
    self->rotation = gfc_vector3d(0, 0, 0);
    self->velocity = gfc_vector3d(0, 0, 0);
    self->scale = def->scale;
    self->color = def->color;

    // Set up collision bounds (generous for easy collection)
    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    if (self->bounds) {
        self->bounds->x = position.x;
        self->bounds->y = position.y;
        self->bounds->z = position.z;
        self->bounds->w = 20.0f;
        self->bounds->h = 20.0f;
        self->bounds->d = 20.0f;
    }

    // Setup callbacks
    self->think = item_think;
    self->update = item_update;
    self->draw = item_draw;
    self->free = item_free;

    // Create item data
    self->data = gfc_allocate_array(sizeof(ItemData), 1);
    if (!self->data) {
        slog("Failed to allocate ItemData");
        entity_free(self);
        return NULL;
    }

    ItemData* data = (ItemData*)self->data;
    data->type = type;
    data->rotationAngle = 0.0f;
    data->bobTimer = 0.0f;
    data->baseZ = position.z;
    data->collected = 0;

    slog("Item spawned: %s at (%.1f, %.1f, %.1f)",
        def->name, position.x, position.y, position.z);

    return self;
}

void item_think(Entity* self) {
    if (!self || !self->data) return;

    ItemData* data = (ItemData*)self->data;
    if (data->collected) return;

    // Check collision with player
    Entity* player = plane_get_player();
    if (player && item_check_collision(self, player)) {
        item_apply_effect(self, player);
        data->collected = 1;
    }
}

void item_update(Entity* self) {
    if (!self || !self->data) return;

    ItemData* data = (ItemData*)self->data;

    // If collected, delete entity
    if (data->collected) {
        entity_free(self);
        return;
    }

    ItemDefinition* def = data_get_item_def(data->type);
    if (!def) return;

    float dt = 1.0f / 60.0f;

    // Rotate around Z axis
    data->rotationAngle += def->rotationSpeed * dt;
    if (data->rotationAngle > M_PI * 2.0f) {
        data->rotationAngle -= M_PI * 2.0f;
    }
    self->rotation.z = data->rotationAngle;

    // Bob up and down
    data->bobTimer += def->bobSpeed * dt;
    float bobOffset = sinf(data->bobTimer) * def->bobHeight;
    self->position.z = data->baseZ + bobOffset;

    // Update bounds
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
}

void item_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod) {
    if (!self) return;
    entity_draw(self, lightPos, colorMod);
}

void item_free(Entity* self) {
    if (!self) return;
    if (self->data) {
        free(self->data);
        self->data = NULL;
    }
}

int item_check_collision(Entity* item, Entity* player) {
    if (!item || !player || !item->bounds || !player->bounds) return 0;

    // Check box overlap
    return gfc_box_overlap(*item->bounds, *player->bounds);
}

void item_apply_effect(Entity* item, Entity* player) {
    if (!item || !player || !item->data || !player->data) return;

    ItemData* itemData = (ItemData*)item->data;
    PlaneData* planeData = (PlaneData*)player->data;
    ItemDefinition* def = data_get_item_def(itemData->type);

    if (!def) return;

    slog("=== PLAYER COLLECTED: %s ===", def->name);

    // Apply effect based on type
    if (strcmp(def->effect, "restore_health") == 0) {
        // Health pack
        int oldHealth = planeData->health;
        planeData->health += def->value;
        if (planeData->health > planeData->maxHealth) {
            planeData->health = planeData->maxHealth;
        }
        slog("Health restored: %d -> %d (+%d)",
            oldHealth, planeData->health, planeData->health - oldHealth);
    }
    else if (strcmp(def->effect, "increase_speed") == 0) {
        // Speed boost
        planeData->speedBoostTimer = def->duration;
        planeData->speedBoostAmount = (float)def->value;
        slog("Speed boost activated: +%.1f for %.1fs",
            planeData->speedBoostAmount, def->duration);
    }
    else if (strcmp(def->effect, "shrink") == 0) {
        // Shrink powerup
        planeData->shrinkTimer = def->duration;
        slog("Shrink activated for %.1fs (harder to hit!)", def->duration);
    }
    else if (strcmp(def->effect, "add_shield") == 0) {
        // Shield powerup
        planeData->shieldHP = def->value;
        planeData->shieldTimer = def->duration;
        slog("Shield activated: %d HP for %.1fs", def->value, def->duration);
    }
    else if (strcmp(def->effect, "invincibility") == 0) {
        // Invincibility powerup
        planeData->invincibilityTimer = def->duration;
        slog("Invincibility activated for %.1fs!", def->duration);
    }
    else if (strcmp(def->effect, "collect") == 0) {
        // Mission objective
        planeData->objectivesCollected++;
        slog("Objective collected! (%d total)", planeData->objectivesCollected);
    }
}