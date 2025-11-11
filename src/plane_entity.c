#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"
#include "world.h"
#include "plane_entity.h"
#include "gfc_matrix.h"
#include "weapon_system.h"
#include <math.h>

#define GRAVITY -0.2f

static Entity* playerPlane = NULL;

// ============================================================================
// QUATERNION MATH
// ============================================================================

GFC_Vector4D quaternion_from_axis_angle(GFC_Vector3D axis, float angle)
{
    GFC_Vector4D q;
    float halfAngle = angle * 0.5f;
    float s = sin(halfAngle);

    float len = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len > 0.0001f) {
        axis.x /= len;
        axis.y /= len;
        axis.z /= len;
    }

    q.w = cos(halfAngle);
    q.x = axis.x * s;
    q.y = axis.y * s;
    q.z = axis.z * s;
    return q;
}

GFC_Vector4D quaternion_multiply(GFC_Vector4D a, GFC_Vector4D b)
{
    GFC_Vector4D result;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return result;
}

void quaternion_to_vectors(GFC_Vector4D q, GFC_Vector3D* forward, GFC_Vector3D* right, GFC_Vector3D* up)
{
    if (!forward || !right || !up) return;

    forward->x = 2.0f * (q.x * q.y + q.w * q.z);
    forward->y = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
    forward->z = 2.0f * (q.y * q.z - q.w * q.x);

    right->x = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    right->y = 2.0f * (q.x * q.y - q.w * q.z);
    right->z = 2.0f * (q.x * q.z + q.w * q.y);

    up->x = 2.0f * (q.x * q.z - q.w * q.y);
    up->y = 2.0f * (q.y * q.z + q.w * q.x);
    up->z = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
}

void plane_init_data(PlaneData* data)
{
    if (!data) return;

    data->orientation = gfc_vector4d(0, 0, 0, 1);
    data->forward = gfc_vector3d(0, 1, 0);
    data->right = gfc_vector3d(1, 0, 0);
    data->up = gfc_vector3d(0, 0, 1);

    data->pitchRate = 0.0f;
    data->yawRate = 0.0f;
    data->rollRate = 0.0f;

    data->speed = 10.0f;
    data->targetSpeed = 10.0f;
    data->acceleration = 0.3f;
    data->maxSpeed = 25.0f;
    data->minSpeed = 3.0f;

    data->pitchSensitivity = 0.015f;
    data->yawSensitivity = 0.012f;
    data->rollSensitivity = 0.02f;

    data->lift = 0.15f;
    data->drag = 0.02f;
    data->gravity = GRAVITY;

    data->isStalling = 0;
    data->isInverted = 0;
    data->camera = NULL;

    slog("==============================================");
    slog("PLANE INITIALIZED");
    slog("Initial Quaternion: (%.4f, %.4f, %.4f, %.4f)",
        data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);
    slog("Initial Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
    slog("Initial Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
    slog("Initial Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);
    slog("==============================================");
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
    self->bounds->x = position.x;
    self->bounds->y = position.y;
    self->bounds->z = position.z;
    self->bounds->w = 5.0f;
    self->bounds->h = 5.0f;
    self->bounds->d = 5.0f;

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
    weapon_loadout_init(&data->loadout, WEAPON_MACHINE_GUN, 0,0);


    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        self->position.z = groundContact.z + 30.0f;
    }

    GFC_Vector3D camPos = gfc_vector3d(position.x, position.y - 30, position.z + 15);
    Entity* cam = camera_entity_spawn(camPos, self);
    if (cam) {
        data->camera = cam;
    }

    playerPlane = self;
    return self;
}

void plane_handle_controls(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    int inputDetected = 0;

    // LOG INPUT
    if (gfc_input_command_down("pitch_up")) {
        slog(">>> INPUT: PITCH_UP (W key)");
        data->pitchRate = data->pitchSensitivity;
        inputDetected = 1;
    }
    else if (gfc_input_command_down("pitch_down")) {
        slog(">>> INPUT: PITCH_DOWN (S key)");
        data->pitchRate = -data->pitchSensitivity;
        inputDetected = 1;
    }
    else {
        data->pitchRate *= 0.96f;
    }

    if (gfc_input_command_down("roll_left")) {
        slog(">>> INPUT: ROLL_LEFT (A key)");
        data->rollRate = data->rollSensitivity;
        inputDetected = 1;
    }
    else if (gfc_input_command_down("roll_right")) {
        slog(">>> INPUT: ROLL_RIGHT (D key)");
        data->rollRate = -data->rollSensitivity;
        inputDetected = 1;
    }
    else {
        data->rollRate *= 0.96f;
    }

    if (gfc_input_command_down("turn_left")) {
        slog(">>> INPUT: TURN_LEFT (Left Arrow)");
        data->yawRate = data->yawSensitivity;
        inputDetected = 1;
    }
    else if (gfc_input_command_down("turn_right")) {
        slog(">>> INPUT: TURN_RIGHT (Right Arrow)");
        data->yawRate = -data->yawSensitivity;
        inputDetected = 1;
    }
    else {
        data->yawRate *= 0.96f;
    }

    if (gfc_input_command_down("throttle_up")) {
        data->targetSpeed += 0.3f;
        if (data->targetSpeed > data->maxSpeed) data->targetSpeed = data->maxSpeed;
    }
    if (gfc_input_command_down("throttle_down")) {
        data->targetSpeed -= 0.3f;
        if (data->targetSpeed < data->minSpeed) data->targetSpeed = data->minSpeed;
    }
    
    //Weapons
    if (gfc_input_command_down("space"))
        weapon_fire(self, 0);

    // Clamp rates
    if (data->pitchRate > 0.04f) data->pitchRate = 0.04f;
    if (data->pitchRate < -0.04f) data->pitchRate = -0.04f;
    if (data->rollRate > 0.05f) data->rollRate = 0.05f;
    if (data->rollRate < -0.05f) data->rollRate = -0.05f;
    if (data->yawRate > 0.03f) data->yawRate = 0.03f;
    if (data->yawRate < -0.03f) data->yawRate = -0.03f;

    if (inputDetected) {
        slog("Current Rates - Pitch: %.5f, Roll: %.5f, Yaw: %.5f",
            data->pitchRate, data->rollRate, data->yawRate);
    }
}

void plane_update_orientation(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    // Check if any rotation is happening
    int hasRotation = (fabs(data->pitchRate) > 0.0001f ||
        fabs(data->rollRate) > 0.0001f ||
        fabs(data->yawRate) > 0.0001f);

    if (!hasRotation) return;

    slog("----------------------------------------------");
    slog("BEFORE ORIENTATION UPDATE:");
    slog("  Quaternion: (%.4f, %.4f, %.4f, %.4f)",
        data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);
    slog("  Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
    slog("  Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
    slog("  Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);

    // 1. PITCH
    if (fabs(data->pitchRate) > 0.0001f) {
        slog("--- APPLYING PITCH (%.5f rad) around RIGHT axis ---", data->pitchRate);
        slog("  Right axis: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);

        GFC_Vector4D qPitch = quaternion_from_axis_angle(data->right, data->pitchRate);
        slog("  Pitch quaternion: (%.4f, %.4f, %.4f, %.4f)", qPitch.x, qPitch.y, qPitch.z, qPitch.w);

        data->orientation = quaternion_multiply(qPitch, data->orientation);
        slog("  After multiply: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        // Normalize
        float mag = sqrt(data->orientation.w * data->orientation.w +
            data->orientation.x * data->orientation.x +
            data->orientation.y * data->orientation.y +
            data->orientation.z * data->orientation.z);
        if (mag > 0.0001f) {
            data->orientation.w /= mag;
            data->orientation.x /= mag;
            data->orientation.y /= mag;
            data->orientation.z /= mag;
        }
        slog("  After normalize: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        quaternion_to_vectors(data->orientation, &data->forward, &data->right, &data->up);
        slog("  New Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
        slog("  New Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
        slog("  New Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);
    }

    // 2. ROLL
    if (fabs(data->rollRate) > 0.0001f) {
        slog("--- APPLYING ROLL (%.5f rad) around FORWARD axis ---", data->rollRate);
        slog("  Forward axis: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);

        GFC_Vector4D qRoll = quaternion_from_axis_angle(data->forward, data->rollRate);
        slog("  Roll quaternion: (%.4f, %.4f, %.4f, %.4f)", qRoll.x, qRoll.y, qRoll.z, qRoll.w);

        data->orientation = quaternion_multiply(qRoll, data->orientation);
        slog("  After multiply: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        float mag = sqrt(data->orientation.w * data->orientation.w +
            data->orientation.x * data->orientation.x +
            data->orientation.y * data->orientation.y +
            data->orientation.z * data->orientation.z);
        if (mag > 0.0001f) {
            data->orientation.w /= mag;
            data->orientation.x /= mag;
            data->orientation.y /= mag;
            data->orientation.z /= mag;
        }
        slog("  After normalize: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        quaternion_to_vectors(data->orientation, &data->forward, &data->right, &data->up);
        slog("  New Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
        slog("  New Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
        slog("  New Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);
    }

    // 3. YAW
    if (fabs(data->yawRate) > 0.0001f) {
        slog("--- APPLYING YAW (%.5f rad) around UP axis ---", data->yawRate);
        slog("  Up axis: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);

        GFC_Vector4D qYaw = quaternion_from_axis_angle(data->up, data->yawRate);
        slog("  Yaw quaternion: (%.4f, %.4f, %.4f, %.4f)", qYaw.x, qYaw.y, qYaw.z, qYaw.w);

        data->orientation = quaternion_multiply(qYaw, data->orientation);
        slog("  After multiply: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        float mag = sqrt(data->orientation.w * data->orientation.w +
            data->orientation.x * data->orientation.x +
            data->orientation.y * data->orientation.y +
            data->orientation.z * data->orientation.z);
        if (mag > 0.0001f) {
            data->orientation.w /= mag;
            data->orientation.x /= mag;
            data->orientation.y /= mag;
            data->orientation.z /= mag;
        }
        slog("  After normalize: (%.4f, %.4f, %.4f, %.4f)",
            data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);

        quaternion_to_vectors(data->orientation, &data->forward, &data->right, &data->up);
        slog("  New Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
        slog("  New Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
        slog("  New Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);
    }

    slog("FINAL ORIENTATION:");
    slog("  Quaternion: (%.4f, %.4f, %.4f, %.4f)",
        data->orientation.x, data->orientation.y, data->orientation.z, data->orientation.w);
    slog("  Forward: (%.4f, %.4f, %.4f)", data->forward.x, data->forward.y, data->forward.z);
    slog("  Right: (%.4f, %.4f, %.4f)", data->right.x, data->right.y, data->right.z);
    slog("  Up: (%.4f, %.4f, %.4f)", data->up.x, data->up.y, data->up.z);
    slog("----------------------------------------------");

    // Convert to Euler for rendering
    float sinr = 2.0f * (data->orientation.w * data->orientation.x + data->orientation.y * data->orientation.z);
    float cosr = 1.0f - 2.0f * (data->orientation.x * data->orientation.x + data->orientation.y * data->orientation.y);
    self->rotation.x = atan2(sinr, cosr);

    float sinp = 2.0f * (data->orientation.w * data->orientation.y - data->orientation.z * data->orientation.x);
    self->rotation.y = (fabs(sinp) >= 1.0f) ? copysign(GFC_PI / 2.0f, sinp) : asin(sinp);

    float siny = 2.0f * (data->orientation.w * data->orientation.z + data->orientation.x * data->orientation.y);
    float cosy = 1.0f - 2.0f * (data->orientation.y * data->orientation.y + data->orientation.z * data->orientation.z);
    self->rotation.z = atan2(siny, cosy);
}

void plane_apply_physics(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    if (data->speed < data->targetSpeed) {
        data->speed += data->acceleration;
        if (data->speed > data->targetSpeed) data->speed = data->targetSpeed;
    }
    else if (data->speed > data->targetSpeed) {
        data->speed -= data->acceleration;
        if (data->speed < data->targetSpeed) data->speed = data->targetSpeed;
    }

    data->isStalling = (data->speed < data->minSpeed);

    GFC_Vector3D thrust;
    gfc_vector3d_scale(thrust, data->forward, data->speed);

    float liftAmount = data->lift * data->speed * 0.5f;
    if (data->isStalling) liftAmount *= 0.2f;
    GFC_Vector3D lift;
    gfc_vector3d_scale(lift, data->up, liftAmount);

    GFC_Vector3D drag;
    gfc_vector3d_scale(drag, data->forward, -data->drag * data->speed);

    GFC_Vector3D gravity = gfc_vector3d(0, 0, data->gravity);

    self->velocity = thrust;
    gfc_vector3d_add(self->velocity, self->velocity, lift);
    gfc_vector3d_add(self->velocity, self->velocity, drag);
    gfc_vector3d_add(self->velocity, self->velocity, gravity);
}

void plane_think(Entity* self)
{
    if (!self) return;
    plane_handle_controls(self);
}

void plane_update(Entity* self)
{
    if (!self) return;

    float dt = 1.0f / 60.0f;
    weapon_update_cooldowns(&((PlaneData*)self->data)->loadout, dt);   // TODO: Change Dt to use SDL ticks
    plane_update_orientation(self);
    plane_apply_physics(self);
    gfc_vector3d_add(self->position, self->position, self->velocity);


    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        if (self->position.z < groundContact.z + 2.0f) {
            self->position.z = groundContact.z + 2.0f;
            if (self->velocity.z < 0) {
                self->velocity.z = -self->velocity.z * 0.3f;
            }
        }
    }

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

GFC_Vector3D plane_get_forward()
{
    if (!playerPlane || !playerPlane->data) {
        return gfc_vector3d(0, 1, 0);
    }
    PlaneData* data = (PlaneData*)playerPlane->data;
    return data->forward;
}