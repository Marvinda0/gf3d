#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"
#include "world.h"
#include "plane_entity.h"
#include "quaternion.h"
#include <math.h>

#define GRAVITY -0.2f
#define PLAYER_SPEED 15.0f

static Entity* playerPlane = NULL;

void plane_init_data(PlaneData* data)
{
    if (!data) return;

    // Identity quaternion (no rotation)
    quaternion_identity(&data->orientation);

    // Speed
    data->speed = 10.0f;
    data->targetSpeed = 10.0f;
    data->acceleration = 0.3f;
    data->maxSpeed = 25.0f;
    data->minSpeed = 3.0f;

    // Control sensitivity - how much rotation per frame
    data->pitchSensitivity = 0.05f;
    data->yawSensitivity = 0.04f;
    data->rollSensitivity = 0.06f;

    // Physics
    data->lift = 0.15f;
    data->drag = 0.02f;
    data->gravity = GRAVITY;

    data->isStalling = 0;
    data->camera = NULL;

    slog("Plane initialized");
}

Entity* plane_spawn(GFC_Vector3D position, GFC_Color color)
{
    Entity* self = entity_new();
    if (!self) {
        slog("Failed to create plane entity");
        return NULL;
    }

    gfc_line_cpy(self->name, "PlayerPlane");
    self->mesh = gf3d_mesh_load("models/plane1/plane1.obj");
    self->color = color;
    self->position = position;
    self->rotation = gfc_vector3d(0, 0, 0);
    self->velocity = gfc_vector3d(0, 0, 0);

    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    if (self->bounds) {
        self->bounds->x = position.x;
        self->bounds->y = position.y;
        self->bounds->z = position.z;
        self->bounds->w = 5.0f;
        self->bounds->h = 5.0f;
        self->bounds->d = 5.0f;
    }

    self->think = plane_think;
    self->update = plane_update;
    self->draw = entity_draw;
    self->free = plane_free;

    self->data = gfc_allocate_array(sizeof(PlaneData), 1);
    if (!self->data) {
        slog("Failed to allocate PlaneData");
        entity_free(self);
        return NULL;
    }

    PlaneData* data = (PlaneData*)self->data;
    plane_init_data(data);

    // Place in air
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        self->position.z = groundContact.z + 30.0f;
    }

    // Spawn camera
    GFC_Vector3D camPos = gfc_vector3d(position.x, position.y - 30, position.z + 15);
    Entity* cam = camera_entity_spawn(camPos, self);
    if (cam) {
        data->camera = cam;
    }

    playerPlane = self;
    slog("Plane spawned at (%.1f, %.1f, %.1f)", position.x, position.y, position.z);
    return self;
}

void plane_handle_controls(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    float dx = 0.0f;  // pitch
    float dy = 0.0f;  // yaw  
    float dz = 0.0f;  // roll

    // Pitch controls
    if (gfc_input_command_down("pitch_up")) {
        dx += data->pitchSensitivity;
    }
    if (gfc_input_command_down("pitch_down")) {
        dx -= data->pitchSensitivity;
    }

    // Roll controls
    if (gfc_input_command_down("roll_left")) {
        dz += data->rollSensitivity;
    }
    if (gfc_input_command_down("roll_right")) {
        dz -= data->rollSensitivity;
    }

    // Yaw controls
    if (gfc_input_command_down("turn_left")) {
        dy += data->yawSensitivity;
    }
    if (gfc_input_command_down("turn_right")) {
        dy -= data->yawSensitivity;
    }

    // Apply rotations if any input
    if (fabs(dx) > 0.001f || fabs(dy) > 0.001f || fabs(dz) > 0.001f) {
        Quaternion delta;
        float half;

        // Pitch (X-axis rotation)
        if (fabs(dx) > 0.001f) {
            half = dx * 0.5f;
            delta = quaternion_create(sinf(half), 0, 0, cosf(half));
            quaternion_multiply_q(&data->orientation, data->orientation, delta);
        }

        // Yaw (Y-axis rotation)
        if (fabs(dy) > 0.001f) {
            half = dy * 0.5f;
            delta = quaternion_create(0, sinf(half), 0, cosf(half));
            quaternion_multiply_q(&data->orientation, data->orientation, delta);
        }

        // Roll (Z-axis rotation)
        if (fabs(dz) > 0.001f) {
            half = dz * 0.5f;
            delta = quaternion_create(0, 0, sinf(half), cosf(half));
            quaternion_multiply_q(&data->orientation, data->orientation, delta);
        }

        // Normalize to prevent drift
        quaternion_normalize(&data->orientation);
    }

    // Throttle
    if (gfc_input_command_down("throttle_up")) {
        data->targetSpeed += 0.3f;
        if (data->targetSpeed > data->maxSpeed) data->targetSpeed = data->maxSpeed;
    }
    if (gfc_input_command_down("throttle_down")) {
        data->targetSpeed -= 0.3f;
        if (data->targetSpeed < data->minSpeed) data->targetSpeed = data->minSpeed;
    }
}

void plane_apply_physics(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    // Adjust speed
    if (data->speed < data->targetSpeed) {
        data->speed += data->acceleration;
        if (data->speed > data->targetSpeed) data->speed = data->targetSpeed;
    }
    else if (data->speed > data->targetSpeed) {
        data->speed -= data->acceleration;
        if (data->speed < data->targetSpeed) data->speed = data->targetSpeed;
    }

    data->isStalling = (data->speed < data->minSpeed);

    // Get forward direction from quaternion
    GFC_Vector3D forward, up;
    quaternion_rotate_v(&forward, data->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&up, data->orientation, gfc_vector3d(0, 0, 1));

    // Thrust - forward
    GFC_Vector3D thrust;
    gfc_vector3d_scale(thrust, forward, data->speed);

    // Lift - upward relative to wings
    float liftAmount = data->lift * data->speed * 0.5f;
    if (data->isStalling) liftAmount *= 0.2f;
    GFC_Vector3D lift;
    gfc_vector3d_scale(lift, up, liftAmount);

    // Drag
    GFC_Vector3D drag;
    gfc_vector3d_scale(drag, forward, -data->drag * data->speed);

    // Gravity
    GFC_Vector3D gravity = gfc_vector3d(0, 0, data->gravity);

    // Combine forces
    self->velocity = thrust;
    gfc_vector3d_add(self->velocity, self->velocity, lift);
    gfc_vector3d_add(self->velocity, self->velocity, drag);
    gfc_vector3d_add(self->velocity, self->velocity, gravity);
}

void plane_update_rotation_for_rendering(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    // Convert quaternion to Euler angles for rendering
    // This is simplified - you can use a more accurate conversion if needed
    Quaternion q = data->orientation;

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    self->rotation.x = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1.0f)
        self->rotation.y = copysignf(GFC_PI / 2.0f, sinp);
    else
        self->rotation.y = asinf(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    self->rotation.z = atan2f(siny_cosp, cosy_cosp);
}

void plane_think(Entity* self)
{
    if (!self) return;
    plane_handle_controls(self);
}

void plane_update(Entity* self)
{
    if (!self) return;

    plane_apply_physics(self);
    plane_update_rotation_for_rendering(self);

    gfc_vector3d_add(self->position, self->position, self->velocity);

    // Ground collision
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        if (self->position.z < groundContact.z + 2.0f) {
            self->position.z = groundContact.z + 2.0f;
            if (self->velocity.z < 0) {
                self->velocity.z = -self->velocity.z * 0.3f;
            }
        }
    }

    // Update bounds
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
}

void plane_free(Entity* self)
{
    if (!self) return;
    if (self->data) {
        free(self->data);
        self->data = NULL;
    }
}

Entity* plane_get_player()
{
    return playerPlane;
}

Quaternion plane_get_orientation()
{
    if (!playerPlane || !playerPlane->data) {
        Quaternion identity;
        quaternion_identity(&identity);
        return identity;
    }
    PlaneData* data = (PlaneData*)playerPlane->data;
    return data->orientation;
}