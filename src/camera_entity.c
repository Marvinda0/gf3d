#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "gf3d_entity.h"
#include "camera_entity.h"
#include "plane_entity.h"
#include "quaternion.h"

void camera_entity_think(Entity* self)
{
    if (!self || !self->data) return;

    CameraEntityData* data = (CameraEntityData*)self->data;
    if (!data->target) return;

    PlaneData* planeData = (PlaneData*)data->target->data;
    if (!planeData) return;

    // Base direction vectors from plane orientation
    GFC_Vector3D forward, up, right;
    quaternion_rotate_v(&forward, planeData->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&up, planeData->orientation, gfc_vector3d(0, 0, 1));
    quaternion_rotate_v(&right, planeData->orientation, gfc_vector3d(1, 0, 0));

    // --- Handle camera orbit input ---
    static float yawOffset = 0.0f; // angle offset from default view (radians)
    float targetOffset = 0.0f;     // desired offset angle
    float dt = 1.0f / 60.0f;

    if (gfc_input_command_down("camera_left")) {
        targetOffset = 0.4f;   // rotate left
    }
    else if (gfc_input_command_down("camera_right")) {
        targetOffset = -0.4f;  // rotate right
    }

    // Smoothly blend toward target offset
    yawOffset += (targetOffset - yawOffset) * 0.1f;

    // --- Compute orbit direction manually using trig ---
    // Base "behind" direction is -forward (we stay behind the plane)
    GFC_Vector3D behind = gfc_vector3d(-forward.x, -forward.y, -forward.z);

    // Apply horizontal rotation using sin/cos
    GFC_Vector3D orbitDir;
    orbitDir.x = behind.x * cosf(yawOffset) + right.x * sinf(yawOffset);
    orbitDir.y = behind.y * cosf(yawOffset) + right.y * sinf(yawOffset);
    orbitDir.z = behind.z * cosf(yawOffset) + right.z * sinf(yawOffset);
    gfc_vector3d_normalize(&orbitDir);

    // Compute final camera position: behind + above + orbit
    GFC_Vector3D cameraPos = data->target->position;
    GFC_Vector3D backOffset, heightOffset;
    gfc_vector3d_scale(backOffset, orbitDir, data->followDist);
    gfc_vector3d_scale(heightOffset, up, data->followHeigth);
    gfc_vector3d_add(cameraPos, cameraPos, backOffset);
    gfc_vector3d_add(cameraPos, cameraPos, heightOffset);

    // Smooth camera follow movement
    float smooth = 0.15f;
    self->position.x += (cameraPos.x - self->position.x) * smooth;
    self->position.y += (cameraPos.y - self->position.y) * smooth;
    self->position.z += (cameraPos.z - self->position.z) * smooth;

    // Prevent going below terrain
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(data->target, get_the_world(), &groundContact)) {
        float minHeight = groundContact.z + 5.0f;
        if (self->position.z < minHeight) self->position.z = minHeight;
    }

    // Apply final camera position & rotation
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