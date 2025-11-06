#ifndef __CAMERA_ENTITY_H__
#define __CAMERA_ENTITY_H__

#include "gfc_text.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"
#include "world.h"
#include "gf3d_mesh.h"
#include "gf3d_texture.h"
#include "gf3d_entity.h"

typedef struct CEntData_S {
    Entity*         target;
    float           angle;
    float           vangle;
    float           followHeight;
    float           followDist;
    void            (*free)(struct CEntData_S* self);
    GFC_Vector3D    forward;

}CameraEntityData;

/**
 * @brief This function is the think function for the camera entity
 * @param self The camera entity
 */
void camera_entity_think(Entity* self);

/**
 * @brief This function spawns a camera entity
 * @param position The position to spawn the camera at
 * @param target The entity for the camera to follow
 * @return The spawned camera entity
 */
Entity* camera_entity_spawn(GFC_Vector3D position, Entity* target);

/**
 * @brief This function frees the camera entity data
 * @param self The camera entity data to free
 */
void camera_entity_free(CameraEntityData* self);
#endif