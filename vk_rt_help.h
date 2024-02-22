#ifndef VK_RT_HELP_H_
#define VK_RT_HELP_H_
#include <assert.h>

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"
// TODO: BAD
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// p appended to names since some of these symbols clash with function definitions
// even though there is no implementation associated with them...
PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHRp;
PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHRp;
PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHRp;
PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHRp;
PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHRp;
PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHRp;
PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHRp;
PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHRp;

void vkrt_get_device_functions(VkDevice device) {
  VKH_RESOLVE_DEVICE_PFN(device, vkGetAccelerationStructureBuildSizesKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkCreateAccelerationStructureKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkCmdBuildAccelerationStructuresKHR);  
  VKH_RESOLVE_DEVICE_PFN(device, vkGetAccelerationStructureDeviceAddressKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkCreateRayTracingPipelinesKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkGetRayTracingShaderGroupHandlesKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkCmdTraceRaysKHR);
  VKH_RESOLVE_DEVICE_PFN(device, vkDestroyAccelerationStructureKHR);
}

typedef struct {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
  VkDeviceAddress device_address;
} vkrt_memory;

typedef vkrt_memory vkrt_as_memory;

vkrt_memory
vkrt_allocate_memory(VkDevice device, VmaAllocator allocator, uint64_t size,
		     void *data, VkBufferUsageFlagBits usage) {
  VkBufferCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
  };
  VmaAllocationCreateInfo vma_info = {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
    | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
  };

  vkrt_memory res = {};
  VK_CHECK(vmaCreateBuffer(allocator, &info, &vma_info, &res.buffer, &res.allocation,
			   &res.info));
  VkBufferDeviceAddressInfo addr_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = res.buffer,
  };
  res.device_address = vkGetBufferDeviceAddress(device, &addr_info);

  if (data != NULL) {
    VK_CHECK(vmaCopyMemoryToAllocation(allocator, data, res.allocation, 0, size));
  }

  return res;
}

void vkrt_memory_free(VmaAllocator allocator, vkrt_memory memory) {
  vmaDestroyBuffer(allocator, memory.buffer, memory.allocation);
}

typedef struct {
  VkAccelerationStructureKHR as;
  vkrt_as_memory memory;
  uint32_t handle;
} vkrt_as;

typedef enum {
  vkrt_as_bottom = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
  vkrt_as_top = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
} vkrt_as_level;

VkAccelerationStructureBuildGeometryInfoKHR
vkrt_as_build_geometry_info(vkrt_as_level lvl, uint64_t geom_cnt,
			    const VkAccelerationStructureGeometryKHR *geoms) {
  return (VkAccelerationStructureBuildGeometryInfoKHR) {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
    .type = lvl,
    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
    .geometryCount = geom_cnt,
    .pGeometries = geoms,
  };
}

vkrt_as vkrt_create_as(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		       vkw_immediate_submit_buffer immediate, vkrt_as_level lvl,
		       VkAccelerationStructureBuildGeometryInfoKHR geom_info,
		       uint32_t primitive_count) {  
  vkrt_as as;
  VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };
  
  vkGetAccelerationStructureBuildSizesKHRp(device,
					   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					   &geom_info, &primitive_count,
					   &as_build_sizes_info); 
  as.memory =
    vkrt_allocate_memory(device, allocator,
			 as_build_sizes_info.accelerationStructureSize, NULL,
			 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as.memory.buffer,
    .size = as_build_sizes_info.accelerationStructureSize,
    .type = lvl,
  };

  VK_CHECK(vkCreateAccelerationStructureKHRp(device, &as_info, NULL, &as.as));
  
  vkrt_memory as_scratch_buffer =
    vkrt_allocate_memory(device, allocator, as_build_sizes_info.buildScratchSize, NULL,
			 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  
  geom_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  geom_info.scratchData.deviceAddress = as_scratch_buffer.device_address;
  geom_info.dstAccelerationStructure = as.as;
  
  const VkAccelerationStructureBuildRangeInfoKHR *as_build_range_info =
    &(VkAccelerationStructureBuildRangeInfoKHR){
    .primitiveCount = primitive_count,
  };

  VkCommandBuffer cmd = vkw_immediate_begin(device, immediate);
  vkCmdBuildAccelerationStructuresKHRp(cmd, 1, &geom_info,
				       &as_build_range_info);
  vkw_immediate_end(device, immediate, scratch_queue);

  VkAccelerationStructureDeviceAddressInfoKHR device_addr_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    .accelerationStructure = as.as,
  };
  as.handle = vkGetAccelerationStructureDeviceAddressKHRp(device, &device_addr_info);

  vkrt_memory_free(allocator, as_scratch_buffer);

  return as;
}

vkrt_as vkrt_create_as2(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
			vkw_immediate_submit_buffer immediate, vkrt_as_level lvl,
			VkAccelerationStructureBuildGeometryInfoKHR geom_info,
			uint32_t build_range_info_count, uint32_t *primitive_counts,
			const VkAccelerationStructureBuildRangeInfoKHR **pp_build_range_infos) {
  vkrt_as as;
  VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };
  
  vkGetAccelerationStructureBuildSizesKHRp(device,
					   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
					   &geom_info, primitive_counts,
					   &as_build_sizes_info); 
  as.memory =
    vkrt_allocate_memory(device, allocator,
			 as_build_sizes_info.accelerationStructureSize, NULL,
			 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as.memory.buffer,
    .size = as_build_sizes_info.accelerationStructureSize,
    .type = lvl,
  };

  VK_CHECK(vkCreateAccelerationStructureKHRp(device, &as_info, NULL, &as.as));
  
  vkrt_memory as_scratch_buffer =
    vkrt_allocate_memory(device, allocator, as_build_sizes_info.buildScratchSize, NULL,
			 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  
  geom_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  geom_info.scratchData.deviceAddress = as_scratch_buffer.device_address;
  geom_info.dstAccelerationStructure = as.as;

  VkCommandBuffer cmd = vkw_immediate_begin(device, immediate);
  vkCmdBuildAccelerationStructuresKHRp(cmd, 1, &geom_info,
				       pp_build_range_infos);
  vkw_immediate_end(device, immediate, scratch_queue);

  VkAccelerationStructureDeviceAddressInfoKHR device_addr_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    .accelerationStructure = as.as,
  };
  as.handle = vkGetAccelerationStructureDeviceAddressKHRp(device, &device_addr_info);

  vkrt_memory_free(allocator, as_scratch_buffer);

  return as;
}

void vkrt_destroy_as(VkDevice device, VmaAllocator allocator, vkrt_as as) {
  vkrt_memory_free(allocator, as.memory);
  vkDestroyAccelerationStructureKHRp(device, as.as, NULL);
}

// TODO: not like this
typedef HMM_Vec3 v3;

typedef struct {
  v3 pos;
  v3 norm;
  float uv[2];
} vkrt_vertex_t;

// temporary
typedef struct {
  float color[3];
  uint32_t base_texture_index;
  uint32_t norm_texture_index;
  uint32_t _pad[3]; // this is needed because the gpu will align this structure
} vkrt_material;

typedef struct {
  vkrt_memory vertex_buffer;
  vkrt_memory index_buffer;

  uint32_t vertex_count;
  uint32_t primitive_count;

  uint32_t material_index;
} vkrt_primitive;

typedef struct {
  size_t primitive_count;
  vkrt_primitive *primitives;
  vkrt_memory transform_buffer;
} vkrt_mesh;

typedef struct {
  size_t mesh_count;
  vkrt_mesh *meshes;
  size_t texture_count;
  vkw_image *textures;
  vkrt_memory materials_buffer;
} vkrt_model;

vkrt_primitive vkrt_load_gltf_primitive(VkDevice device, VmaAllocator allocator,
					cgltf_primitive p, cgltf_data *data) {
  // get the number of indices
  size_t index_count = cgltf_accessor_unpack_indices(p.indices, NULL, 1, 1);  
  // get the number of vertices in the primitive
  size_t vertex_count = 0;
  for (size_t i = 0; i < p.attributes_count; ++i) {
    if (p.attributes[i].type == cgltf_attribute_type_position) {
      vertex_count += p.attributes[i].data->count;
    }
  }
  uint32_t *indices = calloc(sizeof(*indices), index_count);
  vkrt_vertex_t *vertices = calloc(sizeof(*vertices), vertex_count);
  assert(indices && vertices); // TODO: proper error check or something

  cgltf_accessor_unpack_indices(p.indices, indices, 4, p.indices->count);

  for (size_t i = 0; i < p.attributes_count; ++i) {
    cgltf_accessor *attr = p.attributes[i].data;
    if (p.attributes[i].type == cgltf_attribute_type_position) {
      float *p_positions = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].pos = (v3) {
	  p_positions[3*j + 0],
	  p_positions[3*j + 1],
	  p_positions[3*j + 2],
	};
      }
    } else if (p.attributes[i].type == cgltf_attribute_type_normal) {
      float *p_normals = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].norm = (v3) {
	  p_normals[3*j + 0],
	  p_normals[3*j + 1],
	  p_normals[3*j + 2],
	};
      }
    } else if (p.attributes[i].type == cgltf_attribute_type_texcoord) {
      float *p_uvs = (float *)attr->buffer_view->buffer->data +
	attr->buffer_view->offset/sizeof(float) + attr->offset/sizeof(float);
      for (size_t j = 0; j < attr->count; ++j) {
	vertices[j].uv[0] = p_uvs[2*j];
	vertices[j].uv[1] = p_uvs[2*j + 1];
      }
    }
  }

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  vkrt_memory vertex_buffer =
    vkrt_allocate_memory(device, allocator, vertex_count * sizeof(vkrt_vertex_t),
			 vertices, usage);
  vkrt_memory index_buffer =
    vkrt_allocate_memory(device, allocator, index_count * sizeof(uint32_t),
			 indices, usage);
  
  free(vertices);
  free(indices);
  size_t material_idx = 0;
  if (p.material) {
    material_idx = cgltf_material_index(data, p.material);
  }
  
  return (vkrt_primitive) {
    .vertex_buffer = vertex_buffer,
    .index_buffer = index_buffer,
    .material_index = material_idx,
    .vertex_count = vertex_count,
    .primitive_count = index_count / 3
  };
}

vkrt_mesh
vkrt_load_gltf_mesh(VkDevice device, VmaAllocator allocator, cgltf_mesh mesh,
		    cgltf_node node, cgltf_data *data) {
  vkrt_mesh res = {
    .primitive_count = mesh.primitives_count,
    .primitives = calloc(sizeof(vkrt_primitive), mesh.primitives_count),
  };
  VkTransformMatrixKHR transform = {};
  HMM_Mat4 transform4 = {};
  cgltf_node_transform_world(&node, (float*)transform4.Elements);
  transform4 = HMM_TransposeM4(transform4);
  memcpy(&transform, &transform4.Elements, 12 * sizeof(float));

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  res.transform_buffer = vkrt_allocate_memory(device, allocator, sizeof(transform),
					      &transform, usage);
  
  for (size_t i = 0; i < mesh.primitives_count; ++i) {
    res.primitives[i] = vkrt_load_gltf_primitive(device, allocator, mesh.primitives[i], data);
  }

  return res;
}

vkrt_model
vkrt_load_gltf_model(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		     vkw_immediate_submit_buffer immediate, const char *fp) {
  cgltf_options options = {};
  cgltf_data *data = NULL;
  cgltf_result res = cgltf_parse_file(&options, fp, &data);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to open gltf file %s (code: %d)\n", fp, res);
    exit(1);
  }
  res = cgltf_load_buffers(&options, data, fp);
  if (res != cgltf_result_success) {
    fprintf(stderr, "Failed to load gltf file %s (code: %d)\n", fp, res);
    exit(1);
  }
  
  vkrt_model model = { .mesh_count = data->meshes_count };
  model.meshes = calloc(sizeof(*model.meshes), data->meshes_count);
  
  // TODO: INCOMPLETE (need to populate materials and textures)
  vkrt_material *materials = calloc(sizeof(*materials), data->materials_count);
  size_t material_count = data->materials_count;
  for (size_t i = 0; i < material_count; ++i) {
    cgltf_material matt = data->materials[i];
    cgltf_pbr_metallic_roughness mat = matt.pbr_metallic_roughness;
    materials[i].color[0] = mat.base_color_factor[0];
    materials[i].color[1] = mat.base_color_factor[1];
    materials[i].color[2] = mat.base_color_factor[2];
    // TODO:
    if (mat.base_color_texture.texture) {
      materials[i].base_texture_index = cgltf_texture_index(data, mat.base_color_texture.texture);
    } else {
      // TODO: this is not a real value and could contain a valid texture
      // in more complex scenes
      materials[i].base_texture_index = 0xFFFFFFFF;
      //printf("material at index %lu has no texture\n", i);
    }

    if (matt.normal_texture.texture) {
      materials[i].norm_texture_index = cgltf_texture_index(data, matt.normal_texture.texture);
    } else {
      materials[i].norm_texture_index = 0xFFFFFFFF;
    }
  }

  model.texture_count = data->textures_count;
  model.textures = calloc(sizeof(*model.textures), model.texture_count);
  for (size_t i = 0; i < data->textures_count; ++i) {
    cgltf_texture tex = data->textures[i];
    // TODO: if the file is a gltf, then the images need to be loaded via their URI
    // which is relative to the path of the gltf model, so some work reconstructing
    // the location relative to the exe is needed
    if (tex.image && tex.image->buffer_view) {
      cgltf_buffer_view *view = tex.image->buffer_view;      
      int w, h, c;
      uint8_t *data = stbi_load_from_memory(view->buffer->data + view->offset, view->size,
					    &w, &h, &c, 4);
      VkExtent3D dims = { w, h, 1 };
      VkImageUsageFlagBits usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      model.textures[i] =
	vkw_image_create_data(device, allocator, immediate, scratch_queue, dims,
			      VK_FORMAT_R8G8B8A8_UNORM, usage, false, data);
      //printf("Loaded image with dimensions: %d %d %d\n", w, h, 4);
      free(data);
    } else {
      fprintf(stderr, "Failed to load texture at index %lu\n", i);
      if (tex.image->uri) {
	fprintf(stderr, "This image should have been loaded from %s\n", tex.image->uri);
      }
      exit(1);
    }
  }

  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  
  model.materials_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(vkrt_material) * material_count,
			 materials, usage);

  free(materials);
  
  for (size_t i = 0; i < model.mesh_count; ++i) {
    model.meshes[i] = vkrt_load_gltf_mesh(device, allocator, data->meshes[i],
					  data->nodes[i], data);
  }

  cgltf_free(data);
  
  return model;
}

void vkrt_free_model(VkDevice device, VmaAllocator allocator, vkrt_model model) {
  for (size_t i = 0; i < model.mesh_count; ++i) {
    vkrt_mesh mesh = model.meshes[i];
    for (size_t j = 0; j < mesh.primitive_count; ++j) {
      vkrt_primitive p = mesh.primitives[j];
      vkrt_memory_free(allocator, p.vertex_buffer);
      vkrt_memory_free(allocator, p.index_buffer);
    }
    vkrt_memory_free(allocator, mesh.transform_buffer);
    free(mesh.primitives);
  }
  for (size_t i = 0; i < model.texture_count; ++i) {
    vkw_image_destroy(device, allocator, model.textures[i]);
  }
  vkrt_memory_free(allocator, model.materials_buffer);
  free(model.textures);
  free(model.meshes);
}

vkrt_as
vkrt_create_blas(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
		 vkw_immediate_submit_buffer immediate, vkrt_memory vertex_buffer,
		 vkrt_memory index_buffer, uint32_t primitive_count,
		 uint32_t vertex_count, vkrt_memory transform_buffer) {
  VkAccelerationStructureGeometryKHR as_geom_info = {};

  VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};

  const VkAccelerationStructureBuildRangeInfoKHR *p_build_range_info =
    &build_range_info;
  

  as_geom_info = (VkAccelerationStructureGeometryKHR) {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
    .geometry.triangles.sType =
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
    .geometry.triangles.vertexData = vertex_buffer.device_address,
    .geometry.triangles.indexData = index_buffer.device_address,
    .geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
    .geometry.triangles.maxVertex = vertex_count,
    .geometry.triangles.vertexStride = sizeof(vkrt_vertex_t),
    .geometry.triangles.indexType = VK_INDEX_TYPE_UINT32,
    .geometry.triangles.transformData = transform_buffer.device_address,
  };
  build_range_info = (VkAccelerationStructureBuildRangeInfoKHR) {
    .firstVertex = 0,
    .primitiveOffset = 0,
    .primitiveCount = primitive_count,
    .transformOffset = 0,
  };
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_bottom, 1, &as_geom_info);

  vkrt_as blas = vkrt_create_as2(device, allocator, scratch_queue, immediate,
				 vkrt_as_bottom, as_build_geom_info, 1,
				 &primitive_count, &p_build_range_info);

  return blas;
}

vkrt_as vkrt_create_tlas(VkDevice device, VmaAllocator allocator, VkQueue scratch_queue,
			 vkw_immediate_submit_buffer immediate, uint64_t blas_cnt,
			 vkrt_as *blases, VkTransformMatrixKHR transform) {

  VkAccelerationStructureInstanceKHR *as_instances =
    calloc(sizeof(*as_instances), blas_cnt);
  for (uint64_t i = 0; i < blas_cnt; ++i) {
    as_instances[i] = (VkAccelerationStructureInstanceKHR) {
      .transform = transform,
      .instanceCustomIndex = 0,
      .mask = 0xFF,
      .instanceShaderBindingTableRecordOffset = 0,
      .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
      .accelerationStructureReference = blases[i].handle
    };
  }
  VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

  vkrt_memory instance_buffer =
    vkrt_allocate_memory(device, allocator, sizeof(*as_instances) * blas_cnt,
			 as_instances, usage);  
  free(as_instances);

  VkAccelerationStructureGeometryKHR as_geom_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
    .geometry.instances.sType =
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
    .geometry.instances.arrayOfPointers = VK_FALSE,
    .geometry.instances.data = instance_buffer.device_address,
  };
  
  VkAccelerationStructureBuildGeometryInfoKHR as_build_geom_info =
    vkrt_as_build_geometry_info(vkrt_as_top, 1, &as_geom_info);
  
  vkrt_as tlas = vkrt_create_as(device, allocator, scratch_queue, immediate,
				vkrt_as_top, as_build_geom_info,
				blas_cnt);

  vkrt_memory_free(allocator, instance_buffer);
  return tlas;
}

typedef struct {
  uint32_t ds_count;
  uint32_t ds_capacity;
  VkWriteDescriptorSet *write_ds;
  VkDescriptorSet dst_set;
  void *p_next; // ds_count pNext pointers to make sure they are valid
  VkDescriptorBufferInfo *buffer_infos; // make sure pointers are valid
  VkDescriptorImageInfo *image_infos;
  size_t p_next_count;
  size_t image_info_idx;
  size_t buffer_info_idx;
} vkrt_ds_writer;

// max_auxilliary_space determines the size of the extra buffers alloced
// to store buffer infos or image infos
vkrt_ds_writer
vkrt_ds_writer_create(VkDescriptorSet dst_set, uint32_t max_auxilliary_space) {
  VkWriteDescriptorSet *ds = vkh_context.calloc(sizeof(*ds), 1);
  // all (valid) pNext values are the same size, so we allocate based on that fact
  void *pnext = vkh_context.calloc(sizeof(VkWriteDescriptorSetAccelerationStructureKHR),
				   max_auxilliary_space);
  
  VkDescriptorBufferInfo *buffs = vkh_context.calloc(sizeof(VkDescriptorBufferInfo),
						     max_auxilliary_space);
  VkDescriptorImageInfo *imgs = vkh_context.calloc(sizeof(VkDescriptorImageInfo),
						   max_auxilliary_space);

  return (vkrt_ds_writer) {
    .ds_count = 0,
    .ds_capacity = 1,
    .write_ds = ds,
    .dst_set = dst_set,
    .p_next = pnext,
    .buffer_infos = buffs,
    .image_infos = imgs,
  };
}

void vkrt_ds_writer_add(vkrt_ds_writer *writer, VkWriteDescriptorSet set) {
  // dynamic growing array
  if (writer->ds_count + 1 > writer->ds_capacity) {
    writer->ds_capacity *= 2;
    writer->write_ds = vkh_context.realloc(writer->write_ds, sizeof(set)*writer->ds_capacity);
    assert(writer->write_ds);
  }
  writer->write_ds[writer->ds_count++] = set;
}

void vkrt_ds_writer_add_as(vkrt_ds_writer *writer, uint32_t binding,
			   VkAccelerationStructureKHR *as) {
  ((VkWriteDescriptorSetAccelerationStructureKHR*)writer->p_next)[writer->p_next_count] =
    (VkWriteDescriptorSetAccelerationStructureKHR) {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
    .accelerationStructureCount = 1,
    .pAccelerationStructures = as,
  };
  vkrt_ds_writer_add(writer, (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = &((VkWriteDescriptorSetAccelerationStructureKHR*)
		 writer->p_next)[writer->p_next_count],
      .dstSet = writer->dst_set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    });
  writer->p_next_count += 1;
}

void vkrt_ds_writer_add_image(vkrt_ds_writer *writer, uint32_t binding,
			       VkImageView view) {
  writer->image_infos[writer->image_info_idx] = (VkDescriptorImageInfo) {
    .sampler = VK_NULL_HANDLE,
    .imageView = view,
    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
  vkrt_ds_writer_add(writer, (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pImageInfo = &writer->image_infos[writer->image_info_idx],
      .dstSet = writer->dst_set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    });
  writer->image_info_idx += 1;
}

void vkrt_ds_writer_add_sampled_images(vkrt_ds_writer *writer, uint32_t binding,
				       uint32_t image_count, vkw_image *images) {
  size_t start = writer->image_info_idx;
  for (size_t i = 0; i < image_count; ++i) {
    writer->image_infos[writer->image_info_idx++] = (VkDescriptorImageInfo) {
      .sampler = images[i].sampler,
      .imageView = images[i].view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
  }

  vkrt_ds_writer_add(writer, (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pImageInfo = &writer->image_infos[start],
      .dstSet = writer->dst_set,
      .dstBinding = binding,
      .descriptorCount = image_count,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    });
}

void vkrt_ds_writer_add_buffer(vkrt_ds_writer *writer, uint32_t binding,
				VkBuffer buffer, uint64_t offset, uint64_t range) {
  writer->buffer_infos[writer->buffer_info_idx] = (VkDescriptorBufferInfo) {
    .buffer = buffer,
    .offset = offset,
    .range = range
  };
  vkrt_ds_writer_add(writer, (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pBufferInfo = &writer->buffer_infos[writer->buffer_info_idx],
      .dstSet = writer->dst_set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    });

  writer->buffer_info_idx += 1;
}

void vkrt_ds_writer_write(VkDevice device, vkrt_ds_writer writer) {
  vkUpdateDescriptorSets(device, writer.ds_count, writer.write_ds, 0, NULL);
}

void vkrt_ds_writer_free(vkrt_ds_writer *writer) {
  vkh_context.free(writer->write_ds);
  vkh_context.free(writer->p_next);
  vkh_context.free(writer->image_infos);
  vkh_context.free(writer->buffer_infos);
  *writer = (vkrt_ds_writer) {};
}
#endif // VK_RT_HELP_H_
