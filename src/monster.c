#include "simple_logger.h"

#include "gf3d_entity.h"
#include "monster.h"

Entity* monster_spawn(GFC_Vector3D position, GFC_Color color)
{
	Entity* self;
	self = entity_new();
	if (!self)return;

	gfc_line_cpy(self->name, "Manolo");
	self->mesh = gf3d_mesh_load("models/dino/dino.obj");
	self->texture = gf3d_texture_load("models/dino/dino.png");
	self->color = color;
	self->position = position;
	self->rotation = gfc_vector3d(0, 0, 0);
	//self->scale = gfc_vector3d(1, 1, 1); already set in entity_new
	self->speed = gfc_vector3d(0, 0, 0);

	self->think = monster_think;
	self->update = monster_update;
	self->draw = entity_draw;
	
	entity_get_floor_position(self, get_the_world(), &self->position); // place on ground

	return self;
}

void monster_think(Entity* self)
{
	if (!self)return;
	//slog("Monster %s is thinking!", self->name);
}

void monster_update(Entity* self)
{
	if (!self)return;
	self->rotation.z += 0.01;
}