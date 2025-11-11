#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_config.h"
#include "gfc_primitives.h"
#include "gf3d_obj_load.h"
#include "world.h"

static World* world;
World* world_new()
{
	world = (World*)gfc_allocate_array(sizeof(World), 1);
	if (!world)
	{
		slog("Failed to allocate memory for world");
		return NULL;
	}
	//Do initialization here
	return world;
}

Uint8 world_edge_test(World* world, GFC_Vector3D start, GFC_Vector3D end, GFC_Vector3D* contact)
{
	int i, j, c, d;
	GFC_Edge3D edge;
	GFC_Triangle3D t;
	MeshPrimitive *primitive;
	if (!world)return NULL;

	edge = gfc_edge3d_from_vectors(start, end); // Edge from start to end of the world
	c = gfc_list_count(world->mesh->primitives); // Count of primitives in the mesh
	
	for (i = 0; i < c; i++) {
		primitive = gfc_list_get_nth(world->mesh->primitives, i); // we get the primitive at index i of the mesh 

		if(!primitive)continue; // if primitive is null, continue since there is nothing to test

		d = primitive->objData->face_count; // get the face count of the primitive

		for (j = 0; j < d; j++) {
			// we make t a triangle from the face vertices of the primitive
		
			t.a = primitive->objData->faceVertices[primitive->objData->outFace[j].verts[0]].vertex; // get vertex a of the triangle
			t.b = primitive->objData->faceVertices[primitive->objData->outFace[j].verts[1]].vertex; // get vertex b of the triangle
			t.c = primitive->objData->faceVertices[primitive->objData->outFace[j].verts[2]].vertex	; // get vertex c of the triangle
			if (gfc_trigfc_angle_edge_test(edge, t, contact)) {
				contact->z += 4.9f;
				return 1; // if there is a collision, return 1
			}
			
			}
		}
		return 0; // no collision detected	
	}

World* world_load(const char* filename)
{
	World* world = NULL;
	const char* str;
	SJson *json, *config;
	json = sj_load(filename);
	if (!json)
	{
		slog("Failed to load world file: %s\n", filename);
		return NULL;
	}
	world = world_new();
	if (!world)
	{
		slog("Failed to create world\n");
		sj_free(json);
		return NULL;
	}
	config = sj_object_get_value(json, "world");
	if (!config)
	{
		slog("Failed to find 'world' object in world file: %s\n", filename);
		sj_free(json);
		return NULL;
	}
	str = sj_object_get_string(config, "terrainMesh");
	world->mesh = gf3d_mesh_load(str);
	slog("Loading mesh: %s", str);
	if (!world->mesh) slog("Mesh failed to load!");
	world->Texture = gf3d_texture_load(sj_object_get_string(config, "terrainTexture"));
	world->lightcolor = GFC_COLOR_WHITE;
	sj_object_get_vector3d(config, "lightPos", &world->lightpos);
	sj_free(json);
	return world;
}

void world_free(World* world)
{
	if (!world) return;
	if (world->mesh) gf3d_mesh_free(world->mesh);
	if (world->Texture) gf3d_texture_free(world->Texture);
	free(world);
}

void world_draw(World *world) 
{
	GFC_Matrix4 modelMat;
	if (!world) return;
	gfc_matrix4_identity(modelMat);
	//scale the world
	gfc_matrix4_multiply_scalar(modelMat, modelMat, 10);
	gf3d_mesh_draw(world->mesh, modelMat, GFC_COLOR_WHITE, world->Texture, world->lightpos, world->lightcolor);
}

World * get_the_world()
{
	return world;
}