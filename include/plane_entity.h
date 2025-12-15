#ifndef __PLANE_ENTITY_H__
#define __PLANE_ENTITY_H__

#include "gf3d_entity.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "weapon_system.h"
#include "quaternion.h"
#include "data_definitions.h"

typedef struct
{
    Quaternion orientation;  // Quaternion representing plane's rotation

    GFC_Vector3D forward;    // which way plane is facing

    float speed;             // Current forward speed
    float targetSpeed;       // Speed we're trying to reach
    float acceleration;      // How fast we speed up/slow down
    float maxSpeed;          // Maximum speed
    float minSpeed;          // Stall speed (too slow = drop)

    float pitchSensitivity;  // How responsive pitch control is
    float yawSensitivity;    // How responsive yaw control is
    float rollSensitivity;   // How responsive roll control is

    float lift;              // Upward force from wings
    float drag;              // Air resistance
    float gravity;           // Downward force

    Entity* camera;

    Uint8 isStalling;        

    int health;
    int maxHealth;

    // Weapons
    WeaponLoadout loadout;

    float outOfBoundsTimer;
    int isOutOfBounds;

} PlaneData;

/**
 * @brief Spawn a player-controlled plane
 * @param position Starting position in world
 * @param color Color tint for the plane
 * @return Pointer to the created plane entity
 */
Entity* plane_spawn(GFC_Vector3D position, GFC_Color color);

/**
 * @brief Think function - handles input and flight control logic
 * @param self The plane entity
 */
void plane_think(Entity* self);

/**
 * @brief Update function - applies physics and updates orientation
 * @param self The plane entity
 */
void plane_update(Entity* self);

/**
 * @brief Apply flight physics (lift, drag, gravity)
 * @param self The plane entity
 */
void plane_apply_physics(Entity* self);

/**
 * @brief Handle flight controls (pitch, yaw, roll)
 * @param self The plane entity
 */
void plane_handle_controls(Entity* self);

/**
 * @brief Get the player's plane (singleton accessor)
 * @return Pointer to player plane entity
 */
Entity* plane_get_player();

/**
 * @brief Get the orientation quaternion of the player's plane
 * @return Quaternion representing orientation
 */
Quaternion plane_get_orientation();

/**
 * @brief Get the forward direction of the player's plane
 * @return Forward direction vector
 */
GFC_Vector3D plane_get_forward();

/**
 * @brief Player takes damage
 * @param self The plane entity
 * @param damage Amount of damage to take
 */
void plane_take_damage(Entity* self, int damage);

/**
 * @brief Free plane data
 * @param self The plane entity data
 */
void plane_free(Entity* self);

void plane_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod);

/**
 * @brief Spawn player plane with a specific loadout
 * @param position Starting position
 * @param loadoutType Which loadout to use
 * @return Pointer to spawned plane entity
 */
Entity* plane_spawn_with_loadout(GFC_Vector3D position, LoadoutType loadoutType);
#endif // __PLANE_ENTITY_H__