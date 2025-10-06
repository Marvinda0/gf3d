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
	Uint32								mesh_count;
	Uint32								chain_length;			// length of swap chain
	VkDevice							device;					//logical vulakn device handle
	Pipeline							*pipeline;				//the pipeline to use for rendering meshes
	VkVertexInputAttributeDescription	*attributeDescriptions[MESH_ATTRIBUTE_COUNT];	//how the vertex is laid out
	VkVertexInputBindingDescription		bindingDescription;		//how the vertex is described
	Texture 							*defaultTexture;		//a default texture to use when none is specified
}MeshManager;

static MeshManager gf3d_mesh_manager = {0};
	
//foward declarations of local functions
void gf3d_mesh_delete(Mesh* mesh);
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count);
VkVertexInputBindingDescription * gf3d_mesh_manager_get_bind_description();
void gf3d_mesh_primitive_create_vertex_buffer_from_vertices(MeshPrimitive *prim);
void gf3d_mesh_setup_face_buffer(MeshPrimitive *prim);

void gf3d_mesh_init(Uint32mesh_max) 
{
	if (Uint32mesh_max == 0)
	{
		slog("cannot intilizat mesh manager for 0 meshes, must be greater than 0");
		return;
	}

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








