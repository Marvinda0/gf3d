#include "simple_json.h"
#include "simple_logger.h"
#include "world.h"
#include "entity.h"
#include "gfc.h" cofnig
obj data load.h

Static World

World* world_load(const char* filename)
{
	World* world;
	const char* str;
	Sjson* json, * config;
	json = sjson_load(filename);
	if (!json)
	{
		slog("Failed to load world file: %s\n", filename);
		return NULL;
	}
	world = world_new();
	if (!world)
	{
		slog("Failed to create world\n");
		sjson_free(json);
		return NULL;
	}
	config = sj_object_get_value(json, "world");
	if (!config)
	{
		slog("Failed to find 'world' object in world file: %s\n", filename);
		sj_free(world);
		sj_free(json);
		return NULL;
	}
	str = sj_object_get_string(config, "terrainMesh");
	world->terrain = gf3d_mesh_load(str);
	world->texture = gf3d_texture_load(sj_object_get_string(config, "terrainTexture"));
	sj_object_get_color_value
	sj_object_get_vector3d(config, "lightpos", &world->lihghtpos)
	sj_free(json);
}

void world_free(World* world)
{
	if (!world) return;
	if (world->terrain) gf3d_mesh_free(world->terrain);
	if (world->texture) gf3d_texture_free(world->texture);
	free(world);
}

void world_draw(World *world) {


}