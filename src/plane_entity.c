#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"
#include "world.h"
#include "plane_entity.h"
#include "gfc_matrix.h"
#include <math.h>

#define GRAVITY -0.2f

static Entity* playerPlane = NULL;


GFC_Vector4D quaternion_from_axis_angle(GFC_Vector3D axis, float angle)
{
    GFC_Vector4D q;
    float halfAngle = angle * 0.5f;
    float s = sin(halfAngle);

    q.x = axis.x * s;
    q.y = axis.y * s;
    q.z = axis.z * s;
    q.w = cos(halfAngle);

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

    // forward vector (0, 1, 0 rotated by quaternion)
    forward->x = 2.0f * (q.x * q.y + q.w * q.z);
    forward->y = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
    forward->z = 2.0f * (q.y * q.z - q.w * q.x);

    // right vector (1, 0, 0 rotated by quaternion)
    right->x = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    right->y = 2.0f * (q.x * q.y - q.w * q.z);
    right->z = 2.0f * (q.x * q.z + q.w * q.y);

    // up vector (0, 0, 1 rotated by quaternion)
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

    data->yawAngle = 0.0f;
    data->pitchAngle = 0.0f;
    data->rollAngle = 0.0f;

    data->speed = 5.0f;           
    data->targetSpeed = 5.0f;
    data->acceleration = 0.2f;     
    data->maxSpeed = 20.0f;        
    data->minSpeed = 2.0f;        

    data->pitchSensitivity = 0.02f;
    data->yawSensitivity = 0.02f;
    data->rollSensitivity = 0.03f;


    data->lift = 0.08f;             // how much lift wings generate
    data->drag = 0.01f;            // air resistance
    data->gravity = GRAVITY;         // gravity pull

    data->isStalling = 0;
    data->isInverted = 0;
    data->camera = NULL;
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
    //self->texture = gf3d_texture_load("models/dino/dino.png"); // replace with plane texture

    self->color = color;
    self->position = position;
    //self->position.z += 100.0f; // start in the air

    self->rotation = gfc_vector3d(0, 0, GFC_PI);
    self->velocity = gfc_vector3d(0, 0, 0);

    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    self->bounds->x = position.x;
    self->bounds->y = position.y;
    self->bounds->z = position.z;
    self->bounds->w = 5.0f; // Collision radius
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

    // place on ground
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        self->position.z = groundContact.z;
    }

    // camera
    GFC_Vector3D camPos = gfc_vector3d(position.x, position.y - 30, position.z + 15);
    Entity* cam = camera_entity_spawn(camPos, self);
    if (cam) {
        data->camera = cam;
        slog("Camera attached to plane");
    }
    else {
        slog("Warning: Failed to spawn camera for plane");
    }

    playerPlane = self;
    slog("Plane spawned at (%.1f, %.1f, %.1f)", position.x, position.y, position.z);

    return self;
}


void plane_handle_controls(Entity* self)
{
    if (!self || !self->data) return;

    PlaneData* data = (PlaneData*)self->data;

    if (gfc_input_command_down("pitch_up")) {        
        data->pitchAngle += data->pitchSensitivity;
        slog("PITCH UP");
    }
    if (gfc_input_command_down("pitch_down")) {     
        data->pitchAngle -= data->pitchSensitivity;
        slog("PITCH DOWN");
    }

    if (gfc_input_command_down("roll_left")) {       
        data->rollAngle += data->rollSensitivity;
        slog("ROLL LEFT");
    }
    if (gfc_input_command_down("roll_right")) {       
        data->rollAngle -= data->rollSensitivity;
        slog("ROLL RIGHT");
    }

    if (gfc_input_command_down("turn_left")) {         
        data->yawAngle += data->yawSensitivity;
    }
    if (gfc_input_command_down("turn_right")) {       
        data->yawAngle -= data->yawSensitivity;
    }

    if (gfc_input_command_down("throttle_up")) {           
        data->targetSpeed += 0.3f;
        if (data->targetSpeed > data->maxSpeed) {
            data->targetSpeed = data->maxSpeed;
        }
    }
    if (gfc_input_command_down("throttle_down")) {         
        data->targetSpeed -= 0.3f;
        if (data->targetSpeed < data->minSpeed) {
            data->targetSpeed = data->minSpeed;
        }
    }

    //DAMPING (prevent endless spinning)
    data->pitchRate *= 0.85f; 
    data->rollRate *= 0.85f;
    data->yawRate *= 0.85f;

    // Clamp rates
    float maxPitchRate = 0.04f;  
    float maxRollRate = 0.05f;  
    float maxYawRate = 0.03f;

    if (data->pitchRate > maxPitchRate) data->pitchRate = maxPitchRate;
    if (data->pitchRate < -maxPitchRate) data->pitchRate = -maxPitchRate;
    if (data->rollRate > maxRollRate) data->rollRate = maxRollRate;
    if (data->rollRate < -maxRollRate) data->rollRate = -maxRollRate;
    if (data->yawRate > maxYawRate) data->yawRate = maxYawRate;
    if (data->yawRate < -maxYawRate) data->yawRate = -maxYawRate;
}

void plane_update_orientation(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    // Build orientation from stable Euler angles
    GFC_Vector4D qYaw   = quaternion_from_axis_angle(gfc_vector3d(0, 0, 1), data->yawAngle);
    GFC_Vector4D qPitch = quaternion_from_axis_angle(gfc_vector3d(1, 0, 0), data->pitchAngle);
    GFC_Vector4D qRoll  = quaternion_from_axis_angle(gfc_vector3d(0, 1, 0), data->rollAngle);

    // Combine yaw  pitch  roll
    data->orientation = quaternion_multiply(qYaw, qPitch);
    data->orientation = quaternion_multiply(data->orientation, qRoll);

    // Extract vectors
    quaternion_to_vectors(data->orientation, &data->forward, &data->right, &data->up);

    // Normalize
    gfc_vector3d_normalize(&data->forward);
    gfc_vector3d_normalize(&data->right);
    gfc_vector3d_normalize(&data->up);

    // Update Euler angles for rendering
    self->rotation.z = data->yawAngle;
    self->rotation.x = data->pitchAngle;
    self->rotation.y = data->rollAngle;
}




void plane_apply_physics(Entity* self)
{
    if (!self || !self->data) return;

    PlaneData* data = (PlaneData*)self->data;

    // smoothly adjust speed toward target speed
    if (data->speed < data->targetSpeed) {
        data->speed += data->acceleration;
        if (data->speed > data->targetSpeed) data->speed = data->targetSpeed;
    }
    else if (data->speed > data->targetSpeed) {
        data->speed -= data->acceleration;
        if (data->speed < data->targetSpeed) data->speed = data->targetSpeed;
    }

    // Check for stall
    data->isStalling = (data->speed < data->minSpeed);
    if (data->isStalling) {
        slog("WARNING: STALLING!");
    }

    //velocity
    GFC_Vector3D forwardVelocity;
    gfc_vector3d_scale(forwardVelocity, data->forward, data->speed);
    self->velocity = forwardVelocity;

    //lift 
    float speedRatio = data->speed / data->maxSpeed;
    float liftForce = data->lift * (0.5f + speedRatio * 0.5f);

    GFC_Vector3D liftVector;
    gfc_vector3d_scale(liftVector, data->up, liftForce);
    gfc_vector3d_add(self->velocity, self->velocity, liftVector);

    //drag
    GFC_Vector3D dragVector;
    gfc_vector3d_scale(dragVector, data->forward, -data->drag * data->speed);
    gfc_vector3d_add(self->velocity, self->velocity, dragVector);
    /*NO GRAVITY FOR NOW
    //gravity
    self->velocity.z += data->gravity;

    // If stalling, reduce lift significantly
    if (data->isStalling) {
        self->velocity.z += data->gravity * 2.0f; // Fall faster when stalling
    }*/
}

void plane_think(Entity* self)
{
    if (!self) return;

    slog("plane_think START");

    plane_handle_controls(self);

    PlaneData* data = (PlaneData*)self->data;
    if (data) {
        slog("Speed: %.1f, Pitch: %.3f, Roll: %.3f, Yaw: %.3f",
            data->speed, data->pitchRate, data->rollRate, data->yawRate);
    }
}

void plane_update(Entity* self)
{
    slog(">>> BEFORE PHYSICS: pos=(%.2f, %.2f, %.2f) forward.z=%.2f up.z=%.2f",
        self->position.x, self->position.y, self->position.z,
        ((PlaneData*)self->data)->forward.z,
        ((PlaneData*)self->data)->up.z);
    if (!self) return;

    slog("plane_update START: pos=(%.2f, %.2f, %.2f)",
        self->position.x, self->position.y, self->position.z);

    plane_update_orientation(self);

    plane_apply_physics(self);
    slog(">>> AFTER PHYSICS: vel=(%.2f, %.2f, %.2f) pos=(%.2f, %.2f, %.2f)",
        self->velocity.x, self->velocity.y, self->velocity.z,
        self->position.x, self->position.y, self->position.z);


    gfc_vector3d_add(self->position, self->position, self->velocity);

    //(bounce off ground)
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        if (self->position.z < groundContact.z) {
            self->position.z = groundContact.z;
            // TODO:Bounce logic here
            slog("GROUND CONTACT!");
        }
    }

    // Update bounding box
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }

    slog("plane_update END: pos=(%.2f, %.2f, %.2f)",
        self->position.x, self->position.y, self->position.z);
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
        return gfc_vector3d(0, 1, 0); // Default forward
    }

    PlaneData* data = (PlaneData*)playerPlane->data;
    return data->forward;
}