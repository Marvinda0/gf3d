#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"
#include "world.h"
#include "plane_entity.h"
#include "weapon_system.h"
#include "quaternion.h"
#include <math.h>
#include "data_definitions.h"

#define WORLD_MIN_X -1000.0f
#define WORLD_MAX_X  1000.0f
#define WORLD_MIN_Y -1000.0f
#define WORLD_MAX_Y  1000.0f
#define GRAVITY -0.2f

static Entity* playerPlane = NULL;

void plane_init_data(PlaneData* data)
{
    if (!data) return;

    // Identity quaternion (no rotation)
    quaternion_identity(&data->orientation);

    // Initialize direction vector
    data->forward = gfc_vector3d(0, 1, 0);

    // Speed
    data->speed = 5.0f;
    data->targetSpeed = 5.0f;
    data->acceleration = 0.3f;
    data->maxSpeed = 15.0f;
    data->minSpeed = 1.0f;

    // Control sensitivity
    data->pitchSensitivity = 0.05f;
    data->yawSensitivity = 0.04f;
    data->rollSensitivity = 0.06f;

    // Physics
    data->lift = 0.15f;
    data->drag = 0.02f;
    data->gravity = GRAVITY;

    data->isStalling = 0;
    data->camera = NULL;

    data->health = 500;
    data->maxHealth = 500;

    // Initialize weapon loadout
    weapon_loadout_init(&data->loadout, WEAPON_MACHINE_GUN, WEAPON_MISSILE, WEAPON_HOMING);

    data->outOfBoundsTimer = 0;
    data->isOutOfBounds = 0;

    data->speedBoostTimer = 0;
    data->speedBoostAmount = 0;
    data->shrinkTimer = 0;
    data->shieldHP = 0;
    data->shieldTimer = 0;
    data->invincibilityTimer = 0;
    data->objectivesCollected = 0;

    slog("Plane initialized with weapons and health");
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
        self->bounds->w = 10.0f;  // Made bigger so bullets hit easier
        self->bounds->h = 10.0f;
        self->bounds->d = 10.0f;
    }

    self->think = plane_think;
    self->update = plane_update;
    self->draw = plane_draw;
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
    float absoluteMinZ = 2.0f;  

    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        // We have terrain below us
        float minHeight = groundContact.z + 1.0f;
        if (self->position.z < minHeight) {
            self->position.z = minHeight;
            self->velocity.z = 0;
        }
    }
    else {
        // No terrain below us - enforce absolute floor
        if (self->position.z < absoluteMinZ) {
            self->position.z = absoluteMinZ;
            self->velocity.z = 0;

            // Also trigger out of bounds if we're past terrain edge
            data->outOfBoundsTimer += 10.0f;  // Instant kill if under map
        }
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

Entity* plane_spawn_with_loadout(GFC_Vector3D position, LoadoutType loadoutType)
{
    // Get loadout definition
    LoadoutDefinition* loadout = data_get_loadout_def(loadoutType);
    if (!loadout) {
        slog("Failed to get loadout definition, using default");
        return plane_spawn(position, GFC_COLOR_WHITE);
    }

    Entity* self = entity_new();
    if (!self) {
        slog("Failed to create plane entity");
        return NULL;
    }

    gfc_line_cpy(self->name, "PlayerPlane");
    slog("DEBUG: About to load mesh: %s", loadout->modelPath);
    self->mesh = gf3d_mesh_load(loadout->modelPath);
    slog("DEBUG: Mesh loaded: %p", self->mesh);

    self->color = loadout->color;
    self->position = position;
    self->rotation = gfc_vector3d(0, 0, 0);
    self->velocity = gfc_vector3d(0, 0, 0);
    self->scale = loadout->scale;

    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    if (self->bounds) {
        self->bounds->x = position.x;
        self->bounds->y = position.y;
        self->bounds->z = position.z;
        self->bounds->w = 10.0f;
        self->bounds->h = 10.0f;
        self->bounds->d = 10.0f;
    }

    self->think = plane_think;
    self->update = plane_update;
    self->draw = plane_draw;
    self->free = plane_free;

    self->data = gfc_allocate_array(sizeof(PlaneData), 1);
    if (!self->data) {
        slog("Failed to allocate PlaneData");
        entity_free(self);
        return NULL;
    }

    PlaneData* data = (PlaneData*)self->data;

    // Initialize with default values first
    plane_init_data(data);

    // Override with loadout-specific values
    data->health = loadout->health;
    data->maxHealth = loadout->health;
    data->maxSpeed = loadout->maxSpeed;
    data->minSpeed = loadout->minSpeed;
    data->acceleration = loadout->acceleration;
    data->pitchSensitivity = loadout->pitchSensitivity;
    data->yawSensitivity = loadout->yawSensitivity;
    data->rollSensitivity = loadout->rollSensitivity;

    // Set weapons from loadout
    WeaponType w1 = WEAPON_NONE, w2 = WEAPON_NONE, w3 = WEAPON_NONE;    
    if (loadout->weaponCount > 0) {
        w1 = weapon_type_from_string(loadout->weaponTypes[0]);
    }
    if (loadout->weaponCount > 1) {
        w2 = weapon_type_from_string(loadout->weaponTypes[1]);
    }
    if (loadout->weaponCount > 2) {
        w3 = weapon_type_from_string(loadout->weaponTypes[2]);
    }
    weapon_loadout_init(&data->loadout, w1, w2, w3);
    slog("=== LOADOUT DEBUG ===");
    slog("Loadout: %s", loadout->name);
    slog("Weapon count from JSON: %d", loadout->weaponCount);
    slog("w1=%d, w2=%d, w3=%d", w1, w2, w3);
    slog("Final weaponCount in loadout: %d", data->loadout.weaponCount);
    for (int i = 0; i < 3; i++) {
        slog("  Weapon slot %d: type=%d, damage=%d", i, data->loadout.weapons[i].type, data->loadout.weapons[i].damage);
    }
    slog("====================");

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
    slog("Plane spawned with loadout '%s' at (%.1f, %.1f, %.1f)", loadout->name, position.x, position.y, position.z);
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

    // Roll controls (A/D which you use for YAW/turning)
    if (gfc_input_command_down("roll_left")) {
        dz += data->rollSensitivity;
    }
    if (gfc_input_command_down("roll_right")) {
        dz -= data->rollSensitivity;
    }

    // Yaw controls (arrow keys)
    if (gfc_input_command_down("turn_left")) {
        dy += data->yawSensitivity;
    }
    if (gfc_input_command_down("turn_right")) {
        dy -= data->yawSensitivity;
    }

    // Apply rotations if any input
    if (fabs(dx) > 0.001f || fabs(dy) > 0.001f || fabs(dz) > 0.001f) {
        Quaternion delta;

        // Get current local axes
        GFC_Vector3D right, up, forward;
        quaternion_rotate_v(&right, data->orientation, gfc_vector3d(1, 0, 0));
        quaternion_rotate_v(&up, data->orientation, gfc_vector3d(0, 0, 1));
        quaternion_rotate_v(&forward, data->orientation, gfc_vector3d(0, 1, 0));

        // Pitch around the RIGHT axis (local X) - this is correct for flight
        if (fabs(dx) > 0.001f) {
            quaternion_from_axis_angle(&delta, right, dx);
            quaternion_multiply_q(&data->orientation, delta, data->orientation); // WORLD SPACE
        }

        // Yaw around WORLD UP axis (not local up) - this keeps turning natural
        if (fabs(dz) > 0.001f) {
            GFC_Vector3D worldUp = gfc_vector3d(0, 0, 1);
            quaternion_from_axis_angle(&delta, worldUp, dz);
            quaternion_multiply_q(&data->orientation, delta, data->orientation); // WORLD SPACE
        }

        // Roll around arrow keys - around FORWARD axis
        if (fabs(dy) > 0.001f) {
            quaternion_from_axis_angle(&delta, forward, dy);
            quaternion_multiply_q(&data->orientation, delta, data->orientation); // WORLD SPACE
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

    // Weapon controls
    if (gfc_input_command_pressed("space")) {
        weapon_fire(self, 0);
    }
    if (gfc_input_command_pressed("missile")) {
        weapon_fire(self, 1);
    }
    if (gfc_input_command_pressed("homing")) {
        weapon_fire(self, 2);
    }
}

void plane_apply_physics(Entity* self)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;
    float effectiveMaxSpeed = data->maxSpeed;
    if (data->speedBoostTimer > 0) {
        effectiveMaxSpeed += data->speedBoostAmount;
    }
    // Adjust speed
    if (data->speed < data->targetSpeed) {
        data->speed += data->acceleration;
        if (data->speed > data->targetSpeed) data->speed = data->targetSpeed;
    }
    else if (data->speed > data->targetSpeed) {
        data->speed -= data->acceleration;
        if (data->speed < data->targetSpeed) data->speed = data->targetSpeed;
    }
    if (data->targetSpeed > effectiveMaxSpeed) data->targetSpeed = effectiveMaxSpeed;
    if (data->speed > effectiveMaxSpeed) data->speed = effectiveMaxSpeed;
    if (data->speed < data->minSpeed) data->speed = data->minSpeed;

    data->isStalling = (data->speed < data->minSpeed); // check for stall (should only happen at low speed)

    // Get forward direction from quaternion and store it
    GFC_Vector3D up;
    quaternion_rotate_v(&data->forward, data->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&up, data->orientation, gfc_vector3d(0, 0, 1));

    // Thrust - forward
    GFC_Vector3D thrust;
    gfc_vector3d_scale(thrust, data->forward, data->speed);

    // Lift - upward relative to wings
    float liftAmount = data->lift * data->speed * 0.5f;
    if (data->isStalling) liftAmount *= 0.2f;
    GFC_Vector3D lift;
    gfc_vector3d_scale(lift, up, liftAmount);

    // Drag
    GFC_Vector3D drag;
    gfc_vector3d_scale(drag, data->forward, -data->drag * data->speed);

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

    Quaternion q = data->orientation;

    // Get directional vectors
    GFC_Vector3D forward, right, up;
    quaternion_rotate_v(&forward, data->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&right, data->orientation, gfc_vector3d(1, 0, 0));
    quaternion_rotate_v(&up, data->orientation, gfc_vector3d(0, 0, 1));

    // Yaw (Z-axis) - use direct quaternion conversion to preserve A/D input
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    self->rotation.z = atan2f(siny_cosp, cosy_cosp);

    // Pitch (Y-axis) - use vector method to avoid gimbal lock during loops
    float horizontalDist = sqrtf(forward.x * forward.x + forward.y * forward.y);
    self->rotation.y = atan2f(forward.z, horizontalDist);

    // Roll (X-axis) - use vector method
    self->rotation.x = atan2f(-right.z, up.z);
}

void plane_think(Entity* self)
{
    if (!self) return;
    plane_handle_controls(self);
}

void plane_update(Entity* self)
{
    int onGround = 0;
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;

    plane_apply_physics(self);
    if (data->speedBoostTimer > 0) {
        data->speedBoostTimer -= 1.0f / 60.0f;
        // Boost velocity by 1.5x
        gfc_vector3d_scale(self->velocity, self->velocity, 1.5f);
    }
    plane_update_rotation_for_rendering(self);
    weapon_update_cooldowns(&data->loadout, 1.0f / 60.0f);

    // Apply movement
    if (onGround) {
        self->velocity.x = 0;
        self->velocity.y = 0;
    }
    gfc_vector3d_add(self->position, self->position, self->velocity);

    // === WORLD BOUNDS CLAMP ===
    if (self->position.x < WORLD_MIN_X) {
        self->position.x = WORLD_MIN_X;
        self->velocity.x = 0;
    }
    else if (self->position.x > WORLD_MAX_X) {
        self->position.x = WORLD_MAX_X;
        self->velocity.x = 0;
    }

    if (self->position.y < WORLD_MIN_Y) {
        self->position.y = WORLD_MIN_Y;
        self->velocity.y = 0;
    }
    else if (self->position.y > WORLD_MAX_Y) {
        self->position.y = WORLD_MAX_Y;
        self->velocity.y = 0;
    }

    // Absolute floor failsafe
    if (self->position.z < -10.0f) {
        self->position.z = 0;
        self->velocity.z = 0;
    }
    // === GROUND COLLISION ===
    GFC_Vector3D groundContact;
    if (entity_get_floor_position(self, get_the_world(), &groundContact)) {
        float groundZ = groundContact.z + 1.0f;
        if (self->position.z <= groundZ) {
            onGround = 1;
            self->position.z = groundZ;
            self->velocity.z = 0;
            
        }

        // We have floor - clear warning
        data->isOutOfBounds = 0;
    }
    else {
        // No floor detected (likely near edge)
        data->isOutOfBounds =
            (self->position.x <= WORLD_MIN_X || self->position.x >= WORLD_MAX_X ||
                self->position.y <= WORLD_MIN_Y || self->position.y >= WORLD_MAX_Y);
    }

    // Update bounds
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
    float dt = 1.0f / 60.0f;
    if (data->speedBoostTimer > 0) {
        data->speedBoostTimer -= dt;
        data->maxSpeed = 15.0f + data->speedBoostAmount;  // Boost speed
    }
    else {
        data->maxSpeed = 15.0f;  // Normal speed
    }

    if (data->shrinkTimer > 0) {
        data->shrinkTimer -= dt;
        self->scale = gfc_vector3d(0.5, 0.5, 0.5);  // Shrink
    }
    else {
        self->scale = gfc_vector3d(1.0, 1.0, 1.0);  // Normal size
    }

    if (data->shieldTimer > 0) {
        data->shieldTimer -= dt;
    }
    else {
        data->shieldHP = 0;  // Shield expired
    }

    if (data->invincibilityTimer > 0) {
        data->invincibilityTimer -= dt;
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

void plane_take_damage(Entity* self, int damage)
{
    if (!self || !self->data) return;
    PlaneData* data = (PlaneData*)self->data;
    if (data->invincibilityTimer > 0) {
        slog("INVINCIBLE! Damage blocked");
        return;
    }

    // Check shield
    if (data->shieldHP > 0) {
        data->shieldHP -= damage;
        if (data->shieldHP < 0) {
            int overflow = -(int)data->shieldHP;
            data->shieldHP = 0;
            data->health -= overflow;
            slog("Shield broken! Overflow damage: %d", overflow);
        }
        else {
            slog("Shield absorbed damage! Shield: %.0f HP", data->shieldHP);
            return;
        }
    }
    else {
        data->health -= damage;
    }

    if (data->health < 0) data->health = 0;
    slog("PLAYER HIT! Damage=%d | Health=%d/%d", damage, data->health, data->maxHealth);

    if (data->health <= 0) {
        slog("PLAYER DESTROYED!");
        plane_free(self);
        playerPlane = NULL; 
    }
}

void plane_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod)
{
    if (!self || !self->data) return;

    PlaneData* data = (PlaneData*)self->data;

    GFC_Color planeColor = self->color;
    if (data->invincibilityTimer > 0) {
        // Flash between white and original color every 0.1 seconds
        int flashFrame = (int)(data->invincibilityTimer * 10.0f);
        if (flashFrame % 2 == 0) {
            planeColor = gfc_color(1.0f, 1.0f, 1.0f, 1.0f);  // White
        }
    }


    // Extract basis vectors directly from quaternion
    GFC_Vector3D right, forward, up;
    quaternion_rotate_v(&right, data->orientation, gfc_vector3d(1, 0, 0));  
    quaternion_rotate_v(&forward, data->orientation, gfc_vector3d(0, 1, 0));
    quaternion_rotate_v(&up, data->orientation, gfc_vector3d(0, 0, 1));

    // Build matrix manually from basis vectors
    GFC_Matrix4 modelMat;
    gfc_matrix4_identity(modelMat);

    // Column 0: Right vector (scaled)
    modelMat[0][0] = right.x * self->scale.x;
    modelMat[0][1] = right.y * self->scale.x;
    modelMat[0][2] = right.z * self->scale.x;
    modelMat[0][3] = 0;

    // Column 1: Forward vector (scaled)
    modelMat[1][0] = forward.x * self->scale.y;
    modelMat[1][1] = forward.y * self->scale.y;
    modelMat[1][2] = forward.z * self->scale.y;
    modelMat[1][3] = 0;

    // Column 2: Up vector (scaled)
    modelMat[2][0] = up.x * self->scale.z;
    modelMat[2][1] = up.y * self->scale.z;
    modelMat[2][2] = up.z * self->scale.z;
    modelMat[2][3] = 0;

    // Column 3: Translation
    modelMat[3][0] = self->position.x;
    modelMat[3][1] = self->position.y;
    modelMat[3][2] = self->position.z;
    modelMat[3][3] = 1;

    // Render
    gf3d_mesh_draw(
        self->mesh,
        modelMat,
        planeColor,
        self->texture,
        lightPos,
        colorMod
    );
}