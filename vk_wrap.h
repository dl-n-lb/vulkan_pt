#ifndef VK_WRAP_H_
#define VK_WRAP_H_
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#include "vk_help.h"

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  VkFence fence;
  VkCommandBuffer cmd_buf;
  VkCommandPool cmd_pool;
} vkw_immediate_submit_buffer;

vkw_immediate_submit_buffer
vkw_immediate_submit_buffer_create(VkDevice device, uint32_t graphics_family);

typedef void(*_imm_func)(VkCommandBuffer);

VkCommandBuffer
vkw_immediate_begin(VkDevice device, vkw_immediate_submit_buffer imm);

void
vkw_immediate_end(VkDevice device, vkw_immediate_submit_buffer imm, VkQueue queue);

void vkw_immediate_submit_buffer_destroy(VkDevice device,
					 vkw_immediate_submit_buffer buf);

typedef struct {
  VkImage image;
  VkImageView view;
  VmaAllocation allocation;
  VkExtent3D extent;
  VkFormat format;
  VkSampler sampler;
} vkw_image;

vkw_image vkw_image_create(VkDevice device, VmaAllocator alloc, VkExtent3D dims,
			   VkFormat format, VkImageUsageFlags usage, bool mipmap);

vkw_image vkw_image_create_data(VkDevice device, VmaAllocator allocator,
				vkw_immediate_submit_buffer immediate,
				VkQueue imm_queue, VkExtent3D dims, VkFormat fmt,
				VkImageUsageFlags flags, bool mipmap, void *data);

void vkw_image_destroy(VkDevice device, VmaAllocator alloc, vkw_image image);

#define vkw_da_push(p_arr, val) \
  do {									\
    if ((p_arr)->cap < (p_arr)->len + 1) {				\
      if ((p_arr)->cap == 0) {						\
	(p_arr)->cap = 1;						\
      }									\
      (p_arr)->cap *= 2;						\
      (p_arr)->data = vkh_context.realloc((p_arr)->data,		\
					  (p_arr)->cap * sizeof(*(p_arr)->data)); \
    }									\
    (p_arr)->data[(p_arr)->len++] = (val);				\
  } while(0)

#define vkw_da_free(p_arr)			\
  do {						\
    vkh_context.free((p_arr)->data);		\
    (p_arr)->cap = 0;				\
    (p_arr)->len = 0;				\
  } while(0)

typedef struct {
  VkDescriptorType type;
  float ratio;
} vkw_pool_size_ratio;

typedef struct {
  struct {
    uint32_t len;
    uint32_t cap;
    VkDescriptorPool *data;
  } full_pools;
  struct {
    uint32_t len;
    uint32_t cap;
    VkDescriptorPool *data;
  } ready_pools;
  struct {
    uint32_t len;
    uint32_t cap;
    vkw_pool_size_ratio *data;
  } ratios;
  uint32_t sets_per_pool;
} vkw_descriptor_allocator;

VkDescriptorPool
vkw_descriptor_pool_create(VkDevice device, uint32_t sets, uint32_t cnt,
			   vkw_pool_size_ratio ratios[restrict cnt]);

vkw_descriptor_allocator
vkw_descriptor_allocator_init(VkDevice device, uint32_t sets, uint32_t cnt,
			      vkw_pool_size_ratio ratios[restrict cnt]);

VkDescriptorPool
vkw_descriptor_allocator_get(vkw_descriptor_allocator *a, VkDevice device);

VkDescriptorSet
vkw_descriptor_allocator_alloc(vkw_descriptor_allocator *alloc,
			       VkDevice device, VkDescriptorSetLayout layout);
					       

void vkw_descriptor_allocator_clear(VkDevice device,
				    vkw_descriptor_allocator *alloc);

void vkw_descriptor_allocator_destroy(VkDevice device,
				      vkw_descriptor_allocator *alloc);

typedef struct {
  uint32_t len;
  uint32_t cap;
  VkDescriptorSetLayoutBinding *data;
} vkw_descriptor_layout_builder;

void vkw_descriptor_layout_builder_add(vkw_descriptor_layout_builder *b,
				       uint32_t bind, VkDescriptorType type);

VkDescriptorSetLayout
vkw_descriptor_layout_build(vkw_descriptor_layout_builder *b, VkDevice device,
			    VkShaderStageFlags stages);

typedef struct {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  size_t sizeof_push_constants;
} vkw_compute_pipeline;

vkw_compute_pipeline vkw_compute_pipeline_create(VkDevice device,
						 VkDescriptorSetLayout layout,
						 VkShaderModule shader,
						 size_t sizeof_push_constants);

void vkw_compute_pipeline_bind(VkCommandBuffer cmd,
			       vkw_compute_pipeline pipeline,
			       VkDescriptorSet descriptor_set);

void vkw_compute_pipeline_push_constants(VkCommandBuffer cmd,
					 vkw_compute_pipeline pipeline,
					 void *push_constants_data);

void vkw_compute_pipeline_destroy(VkDevice device,
				  vkw_compute_pipeline pipeline);

typedef struct {
  VkCommandPool pool;
  VkCommandBuffer buf;

  VkSemaphore swap_sem, render_sem;
  VkFence render_fence;
} vkw_frame_data;

vkw_frame_data vkw_frame_data_create(VkDevice device, uint32_t queue_idx);


void vkw_frame_data_destroy(VkDevice device, vkw_frame_data data);

void vkw_frame_cmd_begin(VkDevice device, vkw_frame_data data,
			 uint32_t timeout);

void vkw_frame_end_and_submit(VkDevice device, VkQueue queue,
			      vkw_frame_data data);

#endif
#ifdef VK_WRAP_IMPL
//#define VK_HELP_IMPL

vkw_immediate_submit_buffer
vkw_immediate_submit_buffer_create(VkDevice device, uint32_t graphics_family) {
  vkw_immediate_submit_buffer res = {};

  VkCommandPoolCreateInfo cmd_pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = graphics_family,
  };
  VK_CHECK(vkCreateCommandPool(device, &cmd_pool_info, NULL, &res.cmd_pool));

  VkCommandBufferAllocateInfo cmd_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = res.cmd_pool,
    .commandBufferCount = 1,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  };
  VK_CHECK(vkAllocateCommandBuffers(device, &cmd_alloc_info, &res.cmd_buf));

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  VK_CHECK(vkCreateFence(device, &fence_info, NULL, &res.fence));

  return res;
}

VkCommandBuffer
vkw_immediate_begin(VkDevice device, vkw_immediate_submit_buffer imm) {
  VK_CHECK(vkResetFences(device, 1, &imm.fence));
  VK_CHECK(vkResetCommandBuffer(imm.cmd_buf, 0));

  VkCommandBuffer cmd = imm.cmd_buf;
  VkCommandBufferBeginInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = NULL,
    .pInheritanceInfo = NULL,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  VK_CHECK(vkBeginCommandBuffer(cmd, &info));
  return cmd;
}

void
vkw_immediate_end(VkDevice device, vkw_immediate_submit_buffer imm, VkQueue queue) {
  VkCommandBuffer cmd = imm.cmd_buf;
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo sinfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = cmd,
  };
  VkSubmitInfo2 submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &sinfo,
  };
  VK_CHECK(vkQueueSubmit2(queue, 1, &submit, imm.fence));

  VK_CHECK(vkWaitForFences(device, 1, &imm.fence, true, 99999999));
}

void vkw_immediate_submit_buffer_destroy(VkDevice device,
					 vkw_immediate_submit_buffer buf) {
  // destroying the pool will deallocate the buffer
  vkDestroyCommandPool(device, buf.cmd_pool, NULL);
  vkDestroyFence(device, buf.fence, NULL);  
}


vkw_image vkw_image_create(VkDevice device, VmaAllocator alloc, VkExtent3D dims,
		       VkFormat format, VkImageUsageFlags usage, bool mipmap) {
  vkw_image result = { .format = format, .extent = dims };

  VkImageCreateInfo info = vkh_image_create_info(format, dims, usage);
  if (mipmap) {
    uint32_t larger = (dims.width > dims.height) ? dims.width : dims.height;
    info.mipLevels = floorf(log2f((float)larger));
  }
  VmaAllocationCreateInfo alloc_info = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  };
  VK_CHECK(vmaCreateImage(alloc, &info, &alloc_info, &result.image,
			  &result.allocation, NULL));

  // assume image is either depth or colour image (standard usage)
  VkImageAspectFlags flag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT) {
    flag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  VkImageViewCreateInfo vinfo = vkh_image_view_create_info(format,
							   result.image,
							   flag);
  vinfo.subresourceRange.levelCount = info.mipLevels;
  VK_CHECK(vkCreateImageView(device, &vinfo, NULL, &result.view));

  return result;
}

// will assume the data is rgba8 for colour or 32bit fp for depth
vkw_image vkw_image_create_data(VkDevice device, VmaAllocator allocator,
				vkw_immediate_submit_buffer immediate,
				VkQueue imm_queue, VkExtent3D dims, VkFormat fmt,
				VkImageUsageFlags flags, bool mipmap, void *data) {
  flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;  
  size_t data_size = dims.depth * dims.width * dims.height * 4; // r,g,b,a

  VkBuffer buffer;
  VmaAllocationInfo alloc_info;
  VmaAllocation allocation;
  {
    VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = data_size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    VmaAllocationCreateInfo vma_info = {
      .usage = VMA_MEMORY_USAGE_AUTO,
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
      | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
    };

    VK_CHECK(vmaCreateBuffer(allocator, &info, &vma_info, &buffer, &allocation,
			     &alloc_info));
    VK_CHECK(vmaCopyMemoryToAllocation(allocator, data, allocation, 0, data_size));
  }
  
  vkw_image res = vkw_image_create(device, allocator, dims, fmt, flags, mipmap);
  VkCommandBuffer cmd = vkw_immediate_begin(device, immediate);
  
  vkh_transition_image(cmd, res.image, VK_IMAGE_LAYOUT_UNDEFINED,
		       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy copy = {
    .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount = 1,
    .imageExtent = dims
  };

  vkCmdCopyBufferToImage(cmd, buffer, res.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			 1, &copy);

  // NOTE: this might not be a universal thing
  vkh_transition_image(cmd, res.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkw_immediate_end(device, immediate, imm_queue);
  vmaDestroyBuffer(allocator, buffer, allocation);

  // from sascha willems
  VkSamplerCreateInfo sampler_info = {};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  sampler_info.compareOp = VK_COMPARE_OP_NEVER;
  sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_info.maxAnisotropy = 1.0;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxLod = 1;
  //sampler_info.maxAnisotropy = 8.0f;
  //sampler_info.anisotropyEnable = VK_TRUE;

  VK_CHECK(vkCreateSampler(device, &sampler_info, NULL, &res.sampler));
  
  return res;
}

void vkw_image_destroy(VkDevice device, VmaAllocator alloc, vkw_image image) {
  vkDestroyImageView(device, image.view, NULL);
  vmaDestroyImage(alloc, image.image, image.allocation);
  if (image.sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, image.sampler, NULL);
  }
}

VkDescriptorPool
vkw_descriptor_pool_create(VkDevice device, uint32_t sets, uint32_t cnt,
			   vkw_pool_size_ratio ratios[restrict cnt]) {
  VkDescriptorPoolSize *sizes = vkh_context.calloc(cnt, sizeof(*sizes));
  for (uint32_t i = 0; i < cnt; ++i) {
    sizes[i] = (VkDescriptorPoolSize) {
      .type = ratios[i].type,
      .descriptorCount = (uint32_t)(ratios[i].ratio * sets),
    };
  }

  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = sets,
    .poolSizeCount = cnt,
    .pPoolSizes = sizes,
  };
  
  VkDescriptorPool out;
  vkCreateDescriptorPool(device, &info, NULL, &out);
  
  vkh_context.free(sizes);

  return out;
}

vkw_descriptor_allocator
vkw_descriptor_allocator_init(VkDevice device, uint32_t sets, uint32_t cnt,
			      vkw_pool_size_ratio ratios[restrict cnt]) {
  vkw_descriptor_allocator out = {};
  
  for (uint32_t i = 0; i < cnt; ++i) {
    vkw_da_push(&out.ratios, ratios[i]);
  }
  VkDescriptorPool p = vkw_descriptor_pool_create(device, sets, cnt, ratios);
  out.sets_per_pool = sets * 1.5;

  vkw_da_push(&out.ready_pools, p);

  return out;
}

VkDescriptorPool
vkw_descriptor_allocator_get(vkw_descriptor_allocator *a, VkDevice device) {
  if (a->ready_pools.len > 0) {
    return a->ready_pools.data[--a->ready_pools.len];
  } else {
    VkDescriptorPool p = vkw_descriptor_pool_create(device, a->sets_per_pool,
						    a->ratios.len,
						    a->ratios.data);
    a->sets_per_pool *= 1.5;
    if (a->sets_per_pool > 4096) { a->sets_per_pool = 4096; }
    return p;
  }
}

VkDescriptorSet
vkw_descriptor_allocator_alloc(vkw_descriptor_allocator *alloc,
			       VkDevice device, VkDescriptorSetLayout layout) {
  VkDescriptorPool pool = vkw_descriptor_allocator_get(alloc, device);
  VkDescriptorSetAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &layout,
  };

  VkDescriptorSet ds;
  VkResult res = vkAllocateDescriptorSets(device, &info, &ds);
  if (res == VK_ERROR_OUT_OF_POOL_MEMORY ||
      res == VK_ERROR_FRAGMENTED_POOL) {
    vkw_da_push(&alloc->full_pools, pool);
    pool = vkw_descriptor_allocator_get(alloc, device);
    info.descriptorPool = pool;
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &ds));
  } else if (res != VK_SUCCESS) {
    VK_CHECK(res);
  }
  vkw_da_push(&alloc->ready_pools, pool);
  return ds;
}
					       

void vkw_descriptor_allocator_clear(VkDevice device,
				    vkw_descriptor_allocator *alloc) {
  for (uint32_t i = 0; i < alloc->ready_pools.len; ++i) {
    vkResetDescriptorPool(device, alloc->ready_pools.data[i], 0);
  }
  for (uint32_t i = 0; i < alloc->full_pools.len; ++i) {
    vkResetDescriptorPool(device, alloc->full_pools.data[i], 0);
    vkw_da_push(&alloc->ready_pools, alloc->full_pools.data[i]);
  }
  alloc->full_pools.len = 0; // reset the full pools
}

void vkw_descriptor_allocator_destroy(VkDevice device,
				      vkw_descriptor_allocator *alloc) {
  for (uint32_t i = 0; i < alloc->ready_pools.len; ++i) {
    vkDestroyDescriptorPool(device, alloc->ready_pools.data[i], NULL);
  }
  for (uint32_t i = 0; i < alloc->full_pools.len; ++i) {
    vkDestroyDescriptorPool(device, alloc->full_pools.data[i], NULL);
  }
  vkw_da_free(&alloc->ready_pools);
  vkw_da_free(&alloc->full_pools);
  vkw_da_free(&alloc->ratios);
}

void vkw_descriptor_layout_builder_add(vkw_descriptor_layout_builder *b,
				       uint32_t bind, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding binding = {
    .binding = bind,
    .descriptorCount = 1,
    .descriptorType = type,
  };
  vkw_da_push(b, binding);
}

void vkw_descriptor_layout_builder_add2(vkw_descriptor_layout_builder *b,
					uint32_t bind, VkDescriptorType type,
					uint32_t count) {
  VkDescriptorSetLayoutBinding binding = {
    .binding = bind,
    .descriptorCount = count,
    .descriptorType = type,
  };
  vkw_da_push(b, binding);
}

VkDescriptorSetLayout
vkw_descriptor_layout_build(vkw_descriptor_layout_builder *b, VkDevice device,
			    VkShaderStageFlags stages) {
  for (uint32_t i = 0; i < b->len; ++i) {
    b->data[i].stageFlags |= stages;
  }
  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pBindings = b->data,
    .bindingCount = b->len,
  };
  VkDescriptorSetLayout out;
  VK_CHECK(vkCreateDescriptorSetLayout(device, &info, NULL, &out));

  vkw_da_free(b);
  
  return out;
}

vkw_compute_pipeline vkw_compute_pipeline_create(VkDevice device,
						 VkDescriptorSetLayout layout,
						 VkShaderModule shader,
						 size_t sizeof_push_constants) {
  vkw_compute_pipeline res = {
    .sizeof_push_constants = sizeof_push_constants,
  };

  VkPipelineLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pSetLayouts = &layout,
    .setLayoutCount = 1,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &(VkPushConstantRange) {
      .offset = 0,
      .size = sizeof_push_constants,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    },
  };
  
  VK_CHECK(vkCreatePipelineLayout(device, &layout_info, NULL, &res.layout));

  VkPipelineShaderStageCreateInfo stage_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = shader,
    .pName = "main",
  };
  
  VkComputePipelineCreateInfo compute_info = {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .layout = res.layout,
    .stage = stage_info,
  };
  
  VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_info,
				    NULL, &res.pipeline));
  return res;
}

void vkw_compute_pipeline_bind(VkCommandBuffer cmd,
			       vkw_compute_pipeline pipeline,
			       VkDescriptorSet descriptor_set) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout,
			  0, 1, &descriptor_set, 0, NULL);
}

void vkw_compute_pipeline_push_constants(VkCommandBuffer cmd,
					 vkw_compute_pipeline pipeline,
					 void *push_constants_data) {
  vkCmdPushConstants(cmd, pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
		     pipeline.sizeof_push_constants,
		     push_constants_data);
}

void vkw_compute_pipeline_destroy(VkDevice device, vkw_compute_pipeline pipeline) {
  vkDestroyPipeline(device, pipeline.pipeline, NULL);
  vkDestroyPipelineLayout(device, pipeline.layout, NULL);
}

vkw_frame_data vkw_frame_data_create(VkDevice device, uint32_t queue_idx) {
  vkw_frame_data res;
  VkCommandPoolCreateInfo cpci = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_idx,
  };
  VkFenceCreateInfo fi = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  VkSemaphoreCreateInfo si = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VK_CHECK(vkCreateCommandPool(device, &cpci, NULL, &res.pool));

  VkCommandBufferAllocateInfo cbai = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = res.pool,
    .commandBufferCount = 1,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
  };
  VK_CHECK(vkAllocateCommandBuffers(device, &cbai, &res.buf));

  VK_CHECK(vkCreateFence(device, &fi, NULL, &res.render_fence));
  VK_CHECK(vkCreateSemaphore(device, &si, NULL, &res.swap_sem));
  VK_CHECK(vkCreateSemaphore(device, &si, NULL, &res.render_sem));

  return res;
}


void vkw_frame_data_destroy(VkDevice device, vkw_frame_data data) {
    vkDestroyCommandPool(device, data.pool, NULL);
    vkDestroyFence(device, data.render_fence, NULL);
    vkDestroySemaphore(device, data.swap_sem, NULL);
    vkDestroySemaphore(device, data.render_sem, NULL);
}

void vkw_frame_cmd_begin(VkDevice device, vkw_frame_data data,
			 uint32_t timeout) {

  VK_CHECK(vkWaitForFences(device, 1, &data.render_fence, true, timeout));
  VK_CHECK(vkResetFences(device, 1, &data.render_fence)); 

  VK_CHECK(vkResetCommandBuffer(data.buf, 0));
  VkCommandBufferBeginInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  VK_CHECK(vkBeginCommandBuffer(data.buf, &info));
}

void vkw_frame_end_and_submit(VkDevice device, VkQueue queue,
			      vkw_frame_data data) {
    VK_CHECK(vkEndCommandBuffer(data.buf));

    VkCommandBufferSubmitInfo cmd_submit_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = data.buf,
    };

    VkSemaphoreSubmitInfo wait_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = data.swap_sem,
      .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      .value = 1
    };
    VkSemaphoreSubmitInfo sig_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = data.render_sem,
      .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR,
      .value = 1
    };

    VkSubmitInfo2 submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount = 1,
      .pWaitSemaphoreInfos = &wait_info,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos = &sig_info,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &cmd_submit_info,
    };

    VK_CHECK(vkQueueSubmit2(queue, 1, &submit_info,
			    data.render_fence));
}
#endif
