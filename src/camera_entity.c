#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "gf3d_entity.h"
#include "camera_entity.h"
#include "plane_entity.h"
#include "quaternion.h"

void camera_entity_think(Entity* self)
{
    if ((!self) || (!self->data)) return;

    CameraEntityData* data = self->data;
    if (!data->target) return;

    PlaneData* planeData = (PlaneData*)data->target->data;
    if (!planeData) return;

    // Get forward and up vectors from plane's quaternion
    GFC_Vector3D forward, up;
    quaternion_rotate_v(&forward, planeData->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&up, planeData->orientation, gfc_vector3d(0, 0, 1));

    // Calculate camera position behind and above plane
    GFC_Vector3D followBehindVec, followHeightVec;
    GFC_Vector3D cameraPos;

    gfc_vector3d_scale(followBehindVec, forward, -data->followDist);
    gfc_vector3d_scale(followHeightVec, up, data->followHeigth);

    gfc_vector3d_copy(cameraPos, data->target->position);
    gfc_vector3d_add(cameraPos, cameraPos, followBehindVec);
    gfc_vector3d_add(cameraPos, cameraPos, followHeightVec);

    // Smooth camera movement
    float smoothFactor = 0.15f;
    self->position.x = self->position.x + (cameraPos.x - self->position.x) * smoothFactor;
    self->position.y = self->position.y + (cameraPos.y - self->position.y) * smoothFactor;
    self->position.z = self->position.z + (cameraPos.z - self->position.z) * smoothFactor;

    // Prevent camera from going below terrain
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(data->target, get_the_world(), &groundContact)) {
        float minHeight = groundContact.z + 5.0f;
        if (self->position.z < minHeight) {
            self->position.z = minHeight;
        }
    }

    // Set camera position and rotation
    gf3d_camera_set_position(self->position);
    gf3d_camera_set_rotation_q(planeData->orientation);
    gf3d_camera_update_view_q();
}

Entity* camera_entity_spawn(GFC_Vector3D position, Entity* target)
{
    Entity* self = entity_new();
    if (!self) return NULL;

    CameraEntityData* data = gfc_allocate_array(sizeof(CameraEntityData), 1);
    self->data = data;
    self->position = position;
    self->think = camera_entity_think;

    data->target = target;
    data->followDist = 25.0f;
    data->followHeigth = 10.0f;
    data->angle = GFC_PI;

    return self;
}