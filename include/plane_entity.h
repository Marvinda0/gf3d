#ifndef __PLANE_ENTITY_H__
#define __PLANE_ENTITY_H__

#include "gf3d_entity.h"
#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "weapon_system.h"

typedef struct
{
    
    GFC_Vector4D orientation;  // Quaternion representing plane's rotation

    GFC_Vector3D forward;    // which way plane is facing
    GFC_Vector3D right;      // right wing direction
    GFC_Vector3D up;         // top of plane (for rolls)

    float pitchRate;         // nose up/down rotation speed
    float yawRate;           // left/right turning speed
    float rollRate;          // banking/rolling speed

    float yawAngle;
    float pitchAngle;
    float rollAngle;

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

    WeaponLoadout loadout;    // Plane's weapons

    Entity* camera;

    Uint8 isStalling;        // Are we going too slow?
    Uint8 isInverted;        // Are we upside down?

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
 * @brief Update the plane's orientation vectors based on rotation
 * @param self The plane entity
 */
void plane_update_orientation(Entity* self, PlaneData* data);

/**
 * @brief Get the player's plane (singleton accessor)
 * @return Pointer to player plane entity
 */
Entity* plane_get_player();

/**
 * @brief Get the forward direction of the player's plane
 * @return Forward direction vector
 */
GFC_Vector3D plane_get_forward();

/**
 * @brief Free plane data
 * @param self The plane entity data
 */
void plane_free(Entity* self);

#endif // __PLANE_ENTITY_H__