#include "simple_logger.h"
#include "monster.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"
#include "world.h"

#define GRAVITY -0.0f

typedef struct {
    Entity* camera;
    CameraEntityData* camData;
    GFC_Vector3D forward; // camera forward direction
} MonsterEntityData;

static MonsterEntityData* MonsterData = NULL;
static Entity* player = NULL;

Entity* monster_spawn(GFC_Vector3D position, GFC_Color color)
{
    Entity* self;
    self = entity_new();
    if (!self) return NULL;

    gfc_line_cpy(self->name, "Manolo");
    self->mesh = gf3d_mesh_load("models/cube.obj");
    self->texture = gf3d_texture_load("models/dino/dino.png");
    if (!self->texture) slog("Warning: failed to load dino texture");

    self->color = color;
    self->position = position;
    self->position.z = 5.0f;
    self->rotation = gfc_vector3d(0, 0, GFC_PI); // face forward
    self->velocity = gfc_vector3d(0, 0, 0);

    // Initialize bounds
    self->bounds = gfc_allocate_array(sizeof(GFC_Box), 1);
    self->bounds->x = position.x;
    self->bounds->y = position.y;
    self->bounds->z = position.z;
    self->bounds->w = 1;
    self->bounds->h = 1;
    self->bounds->d = 1;

    self->think = monster_think;
    self->update = monster_update;
    self->draw = entity_draw;

    // Initialize monster data
    self->data = gfc_allocate_array(sizeof(MonsterEntityData), 1);
    MonsterData = (MonsterEntityData*)self->data;
    if (!MonsterData) {
        slog("Failed to allocate MonsterData");
        entity_free(self);
        return NULL;
    }

    // Place on ground
    GFC_Vector3D groundPos;
    if (entity_get_floor_position(self, get_the_world(), &groundPos)) {
        self->position.z = groundPos.z;
    }

    // Spawn and link camera
    GFC_Vector3D camPos = gfc_vector3d(position.x, position.y - 20, position.z + 10);
    Entity* cam = camera_entity_spawn(camPos, self);
    if (!cam) {
        slog("Failed to spawn camera entity for monster");
    }
    else {
        MonsterData->camera = cam;
        MonsterData->camData = (CameraEntityData*)cam->data;
        MonsterData->forward = gfc_vector3d(0, 1, 0); // default forward
        slog("Camera entity successfully attached to monster");
    }

    player = self;
    return self;
}

void monster_apply_gravity(Entity* self)
{
    if (!self) return;

    GFC_Vector3D groundContact;
    Uint8 hitFloor = entity_get_floor_position(self, get_the_world(), &groundContact);

    if (!hitFloor) {
        // No floor found - stop horizontal movement to prevent falling into void
        self->velocity.x = 0;
        self->velocity.y = 0;
        return;
    }

    // Apply gravity if not at terminal velocity
    if (self->velocity.z > -10.0f) {
        self->velocity.z += GRAVITY;
    }

    // Check if we need to stop falling
    float distanceToGround = self->position.z - groundContact.z;

    if (distanceToGround <= 0.01f && self->velocity.z <= 0) {
        // On the ground
        self->position.z = groundContact.z;
        self->velocity.z = 0;
    }
}

void monster_move(Entity* self)
{
    if (!self || !MonsterData || !MonsterData->camData) {
        slog("monster_move: ABORT - self=%p MonsterData=%p camData=%p",
            self, MonsterData, MonsterData ? MonsterData->camData : NULL);
        return;
    }


    slog("monster_move: camData->forward=(%.2f, %.2f, %.2f)",
        MonsterData->camData->forward.x,
        MonsterData->camData->forward.y,
        MonsterData->camData->forward.z);

    // Get camera forward direction
    GFC_Vector3D camForward = MonsterData->camData->forward;

    // Flatten to XY plane for movement
    GFC_Vector3D forward;
    forward.x = camForward.x;
    forward.y = camForward.y;
    forward.z = 0;
    gfc_vector3d_normalize(&forward);

    slog("monster_move: forward after normalize=(%.2f, %.2f, %.2f)",
        forward.x, forward.y, forward.z);

    // Calculate right vector (perpendicular to forward)
    GFC_Vector3D right;
    gfc_vector3d_cross_product(&right, forward, gfc_vector3d(0, 0, 1));
    gfc_vector3d_normalize(&right);

    slog("monster_move: right=(%.2f, %.2f, %.2f)", right.x, right.y, right.z);

    // Apply camera-relative movement
    GFC_Vector3D moveDir = gfc_vector3d(0, 0, 0);

    if (self->velocity.y != 0) {
        // Forward/backward movement
        GFC_Vector3D forwardMove;
        gfc_vector3d_scale(forwardMove, forward, self->velocity.y);
        gfc_vector3d_add(moveDir, moveDir, forwardMove);
        slog("monster_move: forwardMove=(%.2f, %.2f, %.2f)",
            forwardMove.x, forwardMove.y, forwardMove.z);
    }

    if (self->velocity.x != 0) {
        // Left/right strafe movement
        GFC_Vector3D rightMove;
        gfc_vector3d_scale(rightMove, right, self->velocity.x);
        gfc_vector3d_add(moveDir, moveDir, rightMove);
    }
    slog("monster_move: final moveDir=(%.2f, %.2f, %.2f)",
        moveDir.x, moveDir.y, moveDir.z);

    // Update position
    self->position.x += moveDir.x;
    self->position.y += moveDir.y;

    slog("monster_move: AFTER applying movement pos=(%.2f, %.2f, %.2f)",
        self->position.x, self->position.y, self->position.z);

    // Update rotation to face movement direction if moving
    if (self->velocity.x != 0 || self->velocity.y != 0) {
        self->rotation.z = atan2(moveDir.y, moveDir.x);
    }

    // Store forward for other systems
    MonsterData->forward = forward;
}

void monster_think(Entity* self)
{
    if (!self) return;

    slog("monster_think START");  

    float moveSpeed = 0.5f;

    // Reset horizontal velocity
    self->velocity.x = 0;
    self->velocity.y = 0;

    // Input handling
    if (gfc_input_command_down("walkforward")) {
        slog("INPUT: walkforward detected!"); 
        self->velocity.y += moveSpeed;
    }
    if (gfc_input_command_down("walkback")) {
        slog("INPUT: walkback detected!");  
        self->velocity.y -= moveSpeed;
    }
    if (gfc_input_command_down("walkleft")) {
        slog("INPUT: walkleft detected!");  
        self->velocity.x -= moveSpeed;
    }
    if (gfc_input_command_down("walkright")) {
        slog("INPUT: walkright detected!");  
        self->velocity.x += moveSpeed;
    }

    slog("velocity: (%.2f, %.2f, %.2f)", self->velocity.x, self->velocity.y, self->velocity.z);  
}

void monster_update(Entity* self)
{
    if (!self) return;

    slog("UPDATE START: pos=(%.2f, %.2f, %.2f)",
        self->position.x, self->position.y, self->position.z);

    // Apply gravity and ground detection
    //monster_apply_gravity(self);

    // Apply movement with camera-relative direction
    monster_move(self);

    // Apply vertical velocity (gravity)
    self->position.z += self->velocity.z;
    slog("UPDATE END: pos=(%.2f, %.2f, %.2f)",
        self->position.x, self->position.y, self->position.z);

    // Update bounding box
    if (self->bounds) {
        self->bounds->x = self->position.x;
        self->bounds->y = self->position.y;
        self->bounds->z = self->position.z;
    }
}

Entity* player_get_the()
{
    return player;
}

GFC_Vector3D player_get_forward()
{
    if (!MonsterData) return gfc_vector3d(0, 1, 0);
    return MonsterData->forward;
}