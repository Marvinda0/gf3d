#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "gf3d_entity.h"
#include "camera_entity.h"


void camera_entity_think(Entity *self)
{
    if ((!self) || (!self->data))return;
    CameraEntityData* data;
    GFC_Vector3D offset;
    float followDist, angle, vangle, panStep = .05, vpanStep = .25;
    //followHeight,     followHeight = data->followHeight; //Maybe we can use this as max in either direction
    data = self->data;
    followDist = data->followDist;
    angle = data->angle;
    offset = gfc_vector3d(0, 1, 0);

    //Camera Movement
    if (gfc_input_command_down("panright")) {
        angle += panStep;
    }
    if (gfc_input_command_down("panleft")) {
        angle -= panStep;
    }
    data->angle = angle;
    //dampen? is it fine till we get our mouse setup?
    gfc_vector3d_rotate_about_z(&offset, angle);

    //Up and down: part of vangle
    vangle = data->vangle;
    if (gfc_input_command_down("panup")) {
        vangle += vpanStep;
    }
    if (gfc_input_command_down("pandown")) {
        vangle -= vpanStep;
    }
    if (vangle < -2.25) { vangle = -2.25; }
    if (vangle > 10.75) { vangle = 10.75; }
    data->vangle = vangle;


    //gfc_vector3d_rotate_about_x(&offset, vangle);

    gfc_vector3d_scale(offset, offset, followDist);
    gfc_vector3d_add(self->position, data->target->position, offset);

    //TODO make it so it cant go below a certain point

    self->position.z = vangle + data->target->position.z;
    //slog("%f, %f, %f: %f",self->position.x, self->position.y, self->position.z, vangle);
    gf3d_camera_look_at(data->target->position, &self->position);
    //Is is subtract to get the forward vector?
    gfc_vector3d_sub(data->forward, self->position, data->target->position);
    gfc_vector3d_normalize(&data->forward);
    //slog("FORWARD: %f %f %f", data->forward.x, data->forward.y, data->forward.z);

}

Entity* camera_entity_spawn(GFC_Vector3D position, Entity* target)
{
    CameraEntityData* data;
    //GFC_Vector3D dir;
    Entity* self;
    self = entity_new();
    if (!self)return NULL;

    data = gfc_allocate_array(sizeof(CameraEntityData), 1);
    self->data = data;
    self->position = position;
    self->think = camera_entity_think;
    //self->free = camera_entity_free;
    data->target = target;
    //15 and 8 originally
    data->followDist = 20;
    data->followHeight = 13;
    data->angle = GFC_PI;
    //set the data tartget as target
    //subtract to get the unit vector in front of our face?
    //gfc_vector3d_sub()

    return self;
}

//void camera_entity_free(CameraEntityData* self)
//{
//	if (!self)return;
//	if (self->free)self->free(self);
//	free(self);
//}
