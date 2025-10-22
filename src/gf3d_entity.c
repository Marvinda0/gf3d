#include "simple_logger.h"

#include "gf3d_entity.h"

typedef struct
{
	Entity *entity_list;
	Uint32 entity_max;
}EntitySystem;

static EntitySystem entity_system = {0};

Entity* entity_new()
{
	Uint32 i;
	if(!entity_system.entity_list)
	{
		slog("entity system not initialized");
		return NULL;
	}
	for (i = 0; i < entity_system.entity_max; i++)
	{
		if (!entity_system.entity_list[i]._inuse)
		{
			entity_system.entity_list[i]._inuse = 1;
			//set default values
			entity_system.entity_list[i].color = GFC_COLOR_WHITE;
			entity_system.entity_list[i].scale = gfc_vector3d(1, 1, 1);
			return &entity_system.entity_list[i];
		}
	}
	slog("no entity list");
	return NULL;
}

void entity_free(Entity * self)
{
	if (!self) return;
	if (self->free) self->free(self);

	gf3d_mesh_free(self->mesh);
	if (!self)
	{
		slog("cannot free a NULL entity");
		return;
	}
	memset(self, 0, sizeof(Entity));
}

void entity_system_init(Uint8 max_entities)
{
	if (!max_entities)
	{
		slog("cannot initialize entity system for 0 entities");
		return;
	}
	entity_system.entity_list = gfc_allocate_array(sizeof(Entity), max_entities);
if (!entity_system.entity_list)
	{
		slog("failed to allocate entity system");
		return;
	}
	entity_system.entity_max = max_entities;
	slog("entity system initialized for %d entities", max_entities);
	atexit(entity_system_close);
}

void entity_system_close()
{
	if (entity_system.entity_list)
	{
		int i = 0;
		for (i = 0; i < entity_system.entity_max; i++)
		{
			if (entity_system.entity_list[i]._inuse)
			{
				entity_free(&entity_system.entity_list[i]);
			}
		}
		free(entity_system.entity_list);
	}
	memset(&entity_system, 0, sizeof(EntitySystem));
	slog("entity system closed");
}

void entity_draw(Entity* self, GFC_Vector3D lightPos, GFC_Color colorMod)
{
	GFC_Matrix4 modelMat;
	if (!self) return;
	gfc_matrix4_from_vectors(modelMat, self->position, self->rotation, self->scale);
	gf3d_mesh_draw
	(
		self->mesh,
		modelMat,
		self->color,
		self->texture,
		lightPos,
		colorMod);

}

void entity_draw_all() {
	int i;
	for (i = 0; i < entity_system.entity_max; i++) {
		if (!entity_system.entity_list[i]._inuse) continue;
		entity_draw(&entity_system.entity_list[i], gfc_vector3d(0, 50, 0), GFC_COLOR_WHITE);
		
	}
}

void entity_think(Entity* self) {
	if (!self) return;
	if (self->think) {
		self->think(self);
	}
}

void entity_think_all() {	
	int i;
	for (i = 0; i < entity_system.entity_max; i++) {
		if (!entity_system.entity_list[i]._inuse) continue;
		entity_think(&entity_system.entity_list[i]);
	}
}

void entity_update(Entity* self) {
	if (!self) return;
	if (self->update) {
		self->update(self);
	}
}

void entity_update_all() {
	int i;
	for (i = 0; i < entity_system.entity_max; i++) {
		if (!entity_system.entity_list[i]._inuse) continue;
		entity_update(&entity_system.entity_list[i]);
	}
}



