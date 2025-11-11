#ifndef __PLANE_ENTITY_H__
#define __PLANE_ENTITY_H__

#include "gf3d_entity.h"
#include "quaternion.h"
#include "weapon_system.h"

typedef struct
{
    Quaternion orientation;     // Plane's rotation as quaternion

    GFC_Vector3D forward;         // Forward direction vector
    float speed;
    float targetSpeed;
    float acceleration;
    float maxSpeed;
    float minSpeed;

    float pitchSensitivity;
    float yawSensitivity;
    float rollSensitivity;

    float lift;
    float drag;
    float gravity;

    int isStalling;

    WeaponLoadout loadout;    // Plane's weapons

    Entity* camera;
} PlaneData;

/**
 * @brief Spawns a plane entity
 * @param position Starting position
 * @param color Plane color
 * @return Pointer to the created plane entity
 */
Entity* plane_spawn(GFC_Vector3D position, GFC_Color color);

/**
 * @brief Handles player input for plane controls
 */
void plane_think(Entity* self);

/**
 * @brief Updates plane physics and position
 */
void plane_update(Entity* self);

/**
 * @brief Frees plane data
 */
void plane_free(Entity* self);

/**
 * @brief Gets the player's plane entity
 * @return Pointer to player plane
 */
Entity* plane_get_player();

/**
 * @brief Gets the plane's orientation quaternion
 * @return Current orientation as quaternion
 */
Quaternion plane_get_orientation();

#endif