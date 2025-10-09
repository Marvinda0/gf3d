#include <stdalign.h>
#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_shape.h"

#include "gf3d_vgraphics.h"
#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_pipeline.h"
#include "gf3d_commands.h"
#include "gf3d_obj_load.h"
#include "gf3d_mesh.h"
#include "gf3d_camera.h"	



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
	VkVertexInputAttributeDescription	attributeDescriptions[MESH_ATTRIBUTE_COUNT];	//how the vertex is laid out
	VkVertexInputBindingDescription		bindingDescription;		//how the vertex is described
	Texture 							*defaultTexture;		//a default texture to use when none is specified
}MeshManager;

static MeshManager mesh_manager = {0};
	
//foward declarations of local functions
void gf3d_mesh_delete(Mesh* mesh);
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count);
VkVertexInputBindingDescription * gf3d_mesh_get_bind_description();
void gf3d_mesh_primitive_create_vertex_buffers(MeshPrimitive *prim);
void gf3d_mesh_setup_face_buffer(MeshPrimitive *prim);
static void gf3d_mesh_primitive_queue_render(MeshPrimitive* prim, Pipeline* pipe, MeshUBO* ubo, Texture* texture);
static void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, MeshUBO* ubo, Texture* texture);

Mesh* gf3d_mesh_new()
{
	Uint32 i;
	for (i = 0; i < mesh_manager.mesh_count; i++)
	{
		if (mesh_manager.mesh_list[i]._refCount == 0)
		{
			memset(&mesh_manager.mesh_list[i], 0, sizeof(Mesh));
			mesh_manager.mesh_list[i]._refCount = 1;
			mesh_manager.mesh_list[i].primitives = gfc_list_new();
			return &mesh_manager.mesh_list[i];
		}
	}
	slog("No free meshes left!");
	return NULL;
}

MeshPrimitive* gf3d_mesh_primitive_new()
{
	MeshPrimitive* prim = gfc_allocate_array(sizeof(MeshPrimitive), 1);
	if (!prim) slog("Failed to allocate mesh primitive");
	return prim;
}

VkVertexInputAttributeDescription* gf3d_mesh_get_attribute_descriptions(Uint32* count)
{
	static VkVertexInputAttributeDescription attributeDescriptions[MESH_ATTRIBUTE_COUNT] = { 0 };

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, vertex);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texel);

	if (count)*count = MESH_ATTRIBUTE_COUNT;
	return attributeDescriptions;
}

VkVertexInputBindingDescription * gf3d_mesh_get_bind_description()
{
	static VkVertexInputBindingDescription bindingDescription = { 0 };

	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return &bindingDescription;
}

void gf3d_mesh_init(Uint32 mesh_max) 
{
	if (mesh_max == 0)
	{
		slog("cannot intilizat mesh manager for 0 meshes, must be greater than 0");
		return;
	}
	mesh_manager.chain_length = gf3d_swapchain_get_chain_length();
	slog("swapchain length = %u", mesh_manager.chain_length);
	mesh_manager.mesh_list = (Mesh*)gfc_allocate_array(sizeof(Mesh),mesh_max);
	mesh_manager.mesh_count = mesh_max;
	mesh_manager.device = gf3d_vgraphics_get_default_logical_device();

	Uint32 count = 0;
	VkVertexInputAttributeDescription* ads = gf3d_mesh_get_attribute_descriptions(&count);
	if (count != MESH_ATTRIBUTE_COUNT)
	{
		slog("error in mesh attribute count");
		return;
	}
	memcpy(mesh_manager.attributeDescriptions, ads, sizeof(VkVertexInputAttributeDescription) * count);
	mesh_manager.bindingDescription = *gf3d_mesh_get_bind_description();

	mesh_manager.pipeline = gf3d_pipeline_create_from_config(
		mesh_manager.device,
		"config/model.cfg",
		gf3d_vgraphics_get_view_extent(),
		mesh_manager.chain_length,
		&mesh_manager.bindingDescription,
		ads,
		count,
		sizeof(MeshUBO),
		VK_INDEX_TYPE_UINT16   // or UINT32 depending on your Face struct
	);
	if (!mesh_manager.pipeline)
	{
		slog("failed to load mesh pipeline");
		return;
	}
	mesh_manager.defaultTexture = gf3d_texture_load("images/default.png", false);

	slog("initializing mesh manager for %d meshes",mesh_max);

}

void gf3d_mesh_setup_face_buffer(MeshPrimitive* prim)
{
	if (!prim || !prim->objData) {
		slog("Mesh setup face buffer: prim or objData NULL");
		return;
	}

	Face* faces = prim->objData->outFace;
	Uint32 fcount = prim->objData->face_count;
	if (!faces || fcount == 0) {
		slog("No faces found in objData->outFace");
		return;
	}

	size_t bufferSize = sizeof(Face) * fcount;
	VkDevice device = gf3d_vgraphics_get_default_logical_device();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	void* data = NULL;

	slog("IB: staging create %zu bytes", bufferSize); slog_sync();
	gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	slog("IB: map"); slog_sync();
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, faces, bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	slog("IB: device-local create"); slog_sync();
	gf3d_buffer_create(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&prim->faceBuffer, &prim->faceBufferMemory);
	slog("IB: copy"); slog_sync();
	gf3d_buffer_copy(stagingBuffer, prim->faceBuffer, bufferSize);
	slog("IB: cleanup staging"); slog_sync();
	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

	slog("Created face buffer with %u faces", fcount);
}



void gf3d_mesh_primitive_create_vertex_buffers(MeshPrimitive *prim)
{
	if (!prim || !prim->objData) { slog("prim/objData NULL"); return; }
	if (!prim->objData->faceVertices || prim->objData->face_vert_count == 0) {
		slog("faceVertices empty"); return;
	}
	void *data = NULL;
	VkDevice device = gf3d_vgraphics_get_default_logical_device();
	Vertex* vertices;
	Uint32 vcount;
	size_t bufferSize;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if (!prim)
	{
		slog("Null mesh primitive");
		return;
	}

	vertices = prim->objData->faceVertices;
	vcount = prim->objData->face_vert_count;
	bufferSize = sizeof(Vertex) * vcount;
	slog("VB: staging create %zu bytes", bufferSize); slog_sync();
	gf3d_buffer_create(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,&stagingBuffer,&stagingBufferMemory);
	
	slog("VB: staging create %zu bytes", bufferSize); slog_sync();
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices, (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	slog("VB: device-local create"); slog_sync();
	gf3d_buffer_create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &prim->vertexBuffer, &prim->vertexBufferMemory);
	slog("VB: copy"); slog_sync();
	gf3d_buffer_copy(stagingBuffer, prim->vertexBuffer, bufferSize);

	slog("VB: cleanup staging"); slog_sync();
	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void gf3d_mesh_manager_close()
{
	Uint32 i;
	if (mesh_manager.mesh_list)
	{
		for (i = 0; i < mesh_manager.mesh_count; i++)
		{
			gf3d_mesh_delete(&mesh_manager.mesh_list[i]);
		}
		free(mesh_manager.mesh_list);
	}
	if (mesh_manager.pipeline)
	{
		gf3d_pipeline_free(mesh_manager.pipeline);
	}
	if (mesh_manager.defaultTexture)
	{
		gf3d_texture_free(mesh_manager.defaultTexture);
	}
	memset(&mesh_manager,0,sizeof(MeshManager));
}

Mesh* gf3d_mesh_load(const char* filename)
{
	Mesh* mesh;
	ObjData* obj;
	MeshPrimitive* prim;

	if (!filename) {
		slog("gf3d_mesh_load: NULL filename");
		return NULL;
	}

	slog("mesh_load: calling obj_load: %s", filename); slog_sync();
	obj = gf3d_obj_load_from_file(filename);
	if (!obj)
	{
		slog("Failed to load OBJ: %s", filename);
		return NULL;
	}

	mesh = gf3d_mesh_new();
	if (!mesh)
	{
		slog("Failed to allocate mesh");
		return NULL;
	}

	strncpy(mesh->filename, filename, sizeof(mesh->filename));
	slog("mesh_new ok: %p", mesh); slog_sync();
	mesh->_refCount = 1;

	prim = gf3d_mesh_primitive_new();
	if (!prim) { slog("primitive alloc failed"); slog_sync(); return NULL; }
	prim->objData = obj;
	slog("Loaded OBJ: verts=%u faces=%u faceVerts=%u",
		obj->vertex_count, obj->face_count, obj->face_vert_count);
	if (!obj->faceVertices || obj->face_vert_count == 0) { slog("no faceVertices"); slog_sync(); return NULL; }
	if (!obj->outFace || obj->face_count == 0) { slog("no outFace"); slog_sync(); return NULL; }
	gf3d_mesh_primitive_create_vertex_buffers(prim);
	gf3d_mesh_setup_face_buffer(prim);
	prim->vertexCount = obj->face_vert_count;
	prim->faceCount = obj->face_count;
	if (!mesh->primitives) { slog("mesh->primitives was NULL, creating list"); mesh->primitives = gfc_list_new(); }
	slog("appending primitive to mesh list"); slog_sync();

	gfc_list_append(mesh->primitives, prim);
	slog("append done. mesh->primitives=%p count=%u",
		mesh->primitives,
		gfc_list_get_count(mesh->primitives));
	slog("first element=%p", gfc_list_get_nth(mesh->primitives, 0));
	slog_sync();
	slog("loaded mesh %s", filename);
	return mesh;
}

void gf3d_mesh_draw(Mesh* mesh, GFC_Matrix4 modelMat, GFC_Color mod, Texture* texture) 
{
	MeshUBO ubo = { 0 };

	if (!mesh) { slog("gf3d_mesh_draw: mesh is NULL"); return; }
	slog("drawing mesh: %p", mesh); slog_sync();
	
	gfc_matrix4_copy(ubo.model, modelMat);
	gf3d_vgraphics_get_view(&ubo.view);
	gf3d_vgraphics_get_projection_matrix(&ubo.proj);

	ubo.color = gfc_color_to_vector4f(mod);
	//ubo.lightColor = gfc_color_to_vector4f(lightColor);
	//ubo.lightPos = gfc_vector3dw(lightPos, 1.0);
	ubo.camera = gfc_vector3dw(gf3d_camera_get_position(), 1.0);
	gf3d_mesh_queue_render(mesh, mesh_manager.pipeline, &ubo, texture);
}
void gf3d_mesh_primitive_queue_render(MeshPrimitive prim, Pipelinepipe, void uboData, Texturetexture)
{
	if ((!prim)(!pipe)(!uboData))return;
	if (!texture)texture = mesh_manager.defaultTexture;
	gf3d_pipeline_queue_render(
		pipe,
		prim->vertexBuffer,
		prim->vertexCount,
		prim->faceBuffer,
		uboData,
		texture);
}

static void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, MeshUBO* ubo, Texture* texture)
{
	if (!mesh || !pipe || !ubo) return;
	if (!mesh->primitives) return;

	int count = gfc_list_get_count(mesh->primitives);
	for (int i = 0; i < count; i++)
	{
		MeshPrimitive* prim = (MeshPrimitive*)gfc_list_get_nth(mesh->primitives, i);
		if (!prim) continue;
		gf3d_mesh_primitive_queue_render(prim, pipe, ubo, texture);
	}
}

Pipeline* gf3d_mesh_get_pipeline()
{
	return mesh_manager.pipeline;	
}

static void gf3d_mesh_delete(Mesh* mesh)
{
	if (!mesh || mesh->_refCount == 0) return;

	// free primitives
	if (mesh->primitives)
	{
		int c = gfc_list_get_count(mesh->primitives);
		for (int i = 0; i < c; ++i)
		{
			MeshPrimitive* p = (MeshPrimitive*)gfc_list_get_nth(mesh->primitives, i);
			if (!p) continue;

			VkDevice device = gf3d_vgraphics_get_default_logical_device();
			if (p->vertexBuffer)    vkDestroyBuffer(device, p->vertexBuffer, NULL);
			if (p->vertexBufferMemory) vkFreeMemory(device, p->vertexBufferMemory, NULL);
			if (p->faceBuffer)      vkDestroyBuffer(device, p->faceBuffer, NULL);
			if (p->faceBufferMemory)   vkFreeMemory(device, p->faceBufferMemory, NULL);

			// if ObjData ownership belongs here, free it; otherwise omit:
			// gf3d_obj_free(p->objData);

			free(p);
		}
		gfc_list_delete(mesh->primitives);
	}

	memset(mesh, 0, sizeof(*mesh));
}










