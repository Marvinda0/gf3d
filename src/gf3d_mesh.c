#include <stdalign.h>
#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_shape.h"

#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_vgraphics.h"
#include "gf3d_pipeline.h"
#include "gf3d_commands.h"
#include "gf3d_mesh.h"


#define MESH_ATTRIBUTE_COUNT 3

extern int __DEBUG;

//Check gf2d
typedef struct
{
	Mesh*								mesh_list;
	Unit32								mesh_count;
	Unit32								chain_length;			// length of swap chain
	VkDevice							device;					//logical vulakn device handle
	Pipeline							*pipeline;				//the pipeline to use for rendering meshes
	VkVertexInutAttributeDescription	*attributeDescriptions[MESH_ATTRIBUTE_COUNT];	//how the vertex is laid out
	VkVertexInputBindingDescription		bindingDescription;		//how the vertex is described
	Texture 							*defaultTexture;		//a default texture to use when none is specified
}MeshManager;

static MeshManager gf3d_mesh_manager = {0};

MeshUBO gf3d_mesh_get_ubo(
	GFC_Matrix4 modelMat,
	GFC_Color colorMod);
	
//foward declarations of local functions
void gf3d_mesh_delete(Mesh* mesh);
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count);
VkVertexInputBindingDescription * gf3d_mesh_manager_get_bind_description();
void gf3d_mesh_primitive_create_vertex_buffer_from_vertices(MeshPrimitive *prim);
void gf3d_mesh_setup_face_buffer(MeshPrimitive *prim);

Mesh *gf3d_mesh_new()
{
	Mesh *mesh;
	if (!gf3d_mesh_manager.device)
	{
		slog("cannot create mesh before gf3d_mesh_init has been called");
		return NULL;
	}
	mesh = (Mesh*)gfc_allocate_array(sizeof(Mesh),1);
	if (!mesh)
	{
		slog("failed to allocate mesh");
		return NULL;
	}
	mesh->refCount = 1;// start with a ref count of 1
	mesh->primitiveList = NULL;//no primitives yet
	mesh->primitiveCount = 0;// no primitives yet
	mesh->modelMatrix = gfc_matrix4_identity();//start with identity matrix
	mesh->color = gfc_color8(255,255,255,255); //default to white
	return mesh;
}

void gf3d_mesh_manager_close()
{
	Uint32 i;
	if (gf3d_mesh_manager.mesh_list)
	{
		for (i = 0; i < gf3d_mesh_manager.mesh_count; i++)
		{
			gf3d_mesh_delete(&gf3d_mesh_manager.mesh_list[i]);
		}
		free(gf3d_mesh_manager.mesh_list);
	}
	if (gf3d_mesh_manager.pipeline)
	{
		gf3d_pipeline_free(gf3d_mesh_manager.pipeline);
	}
	if (gf3d_mesh_manager.defaultTexture)
	{
		gf3d_texture_free(gf3d_mesh_manager.defaultTexture);
	}
	memset(&gf3d_mesh_manager,0,sizeof(MeshManager));
}








