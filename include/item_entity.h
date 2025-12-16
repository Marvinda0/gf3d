#ifndef __ITEM_ENTITY_H__
#define __ITEM_ENTITY_H__

#include "gf3d_entity.h"
#include "data_definitions.h"

/**
 * @brief Item entity data
 */
typedef struct {
    ItemType type;
    float rotationAngle;    // Current rotation angle
    float bobTimer;         // Timer for bob animation
    float baseZ;            // Original Z position
    int collected;          // 1 if collected, 0 otherwise
} ItemData;

/**
 * @brief Spawn an item at a position
 * @param position Where to spawn the item
 * @param type Type of item to spawn
 * @return Pointer to spawned item entity
 */
Entity* item_spawn(GFC_Vector3D position, ItemType type);

/**
 * @brief Item think callback
 */
void item_think(Entity* self);

/**
 * @brief Item update callback
 */
void item_update(Entity* self);

/**
 * @brief Item draw callback
 */
void item_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod);

/**
 * @brief Item free callback
 */
void item_free(Entity* self);

/**
 * @brief Check if player is close enough to collect item
 * @param item The item entity
 * @param player The player entity
 * @return 1 if collected, 0 otherwise
 */
int item_check_collision(Entity* item, Entity* player);

/**
 * @brief Apply item effect to player
 * @param item The item entity
 * @param player The player entity
 */
void item_apply_effect(Entity* item, Entity* player);

#endif