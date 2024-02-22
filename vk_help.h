#ifndef VK_HELP_H_
#define VK_HELP_H_

#include "vk_mem_alloc.h"
#include "vulkan/vulkan.h"

// TODO: remove these dependencies for some log function
// perhaps of the form log(FILE *, fmt, ...)
#include <stdio.h>
#include <stdbool.h>

const char* string_VkResult(VkResult input_value);

#define VKH_RESOLVE_DEVICE_PFN(device, pfn)			\
  do {								\
    pfn##p = (PFN_##pfn)vkGetDeviceProcAddr(device, #pfn);	\
    if (!( pfn##p )) {					      	\
      fprintf(stderr, "Failed to load %s\n", #pfn);             \
      exit(1);                                                  \
    }		                                                \
  } while(0);

#define VK_CHECK(x)							\
  do {									\
    VkResult err = x;							\
    if (err) {								\
      fprintf(stderr, "Vulkan Error: %s", string_VkResult(err));	\
      exit(1);								\
    }									\
  } while(0);

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT
					*create_info,
					const VkAllocationCallbacks *alloc,
					VkDebugUtilsMessengerEXT *out_messenger);

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
				     VkDebugUtilsMessengerEXT messenger,
				     const VkAllocationCallbacks *alloc);


// global context struct contains the allocator you want to use
typedef void *(*vkh_malloc_pfn)(size_t);
typedef void *(*vkh_calloc_pfn)(size_t, size_t);
typedef void *(*vkh_realloc_pfn)(void *, size_t);
typedef void  (*vkh_free_pfn)(void *);
struct {
  vkh_malloc_pfn malloc;
  vkh_calloc_pfn calloc;
  vkh_realloc_pfn realloc;
  vkh_free_pfn free;
  void *ctx_ptr; // any user data can go here and be referenced by the other fns i suppose..
} vkh_context;

typedef struct {
  char **data;
  size_t len;
  size_t cap;
} vkh_darr_str_t;

vkh_darr_str_t vkh_darr_str_init(size_t initial_cap);

void vkh_darr_str_free(vkh_darr_str_t *arr);

// push a string into a dynamic array of strings
// the string is not owned by the array
void vkh_darr_str_push(vkh_darr_str_t *arr, char *new);

typedef struct {
  VkApplicationInfo app_info;
  vkh_darr_str_t extension_names;
  vkh_darr_str_t layer_names;

  bool enable_debug_messenger;
  VkDebugUtilsMessengerCreateInfoEXT dbg_create_info;
} vkh_instance_builder;

typedef struct {
  VkInstance instance;
  VkDebugUtilsMessengerEXT messenger;
} vkh_instance;

VkApplicationInfo vkh_default_app_info();

vkh_instance_builder vkh_new_instance_builder();

void vkh_set_api_version(vkh_instance_builder *b, uint32_t api_version);

VKAPI_ATTR VkBool32 VKAPI_CALL
vkh_default_debug_messenger_cb(VkDebugUtilsMessageSeverityFlagBitsEXT msg_sev,
			       VkDebugUtilsMessageTypeFlagsEXT msg_type,
			       const VkDebugUtilsMessengerCallbackDataEXT *d,
			       void *user_data);

void vkh_enable_debug_messenger(vkh_instance_builder *b,
				VkDebugUtilsMessengerCreateInfoEXT *info);

void vkh_enable_layer(vkh_instance_builder *b, char *name);

void vkh_enable_layers(vkh_instance_builder *b, size_t count,
		       char *names[restrict count]);

void vkh_enable_validation(vkh_instance_builder *b);

void vkh_enable_extension(vkh_instance_builder *b, char *name);

void vkh_enable_extensions(vkh_instance_builder *b, size_t count,
			   char *names[restrict count]);

vkh_instance vkh_instance_build(vkh_instance_builder b);

typedef struct {
  VkInstance instance;
  VkSurfaceKHR surface;
  
  uint32_t min_version;
  VkPhysicalDeviceFeatures2 features;
  VkPhysicalDeviceVulkan11Features features11;
  VkPhysicalDeviceVulkan12Features features12;
  VkPhysicalDeviceVulkan13Features features13;
  
  vkh_darr_str_t extensions;
  void *pnext;

  bool selected;
  VkPhysicalDevice physical_device;  
} vkh_physical_device;

typedef struct {
  VkDevice device;
  VkPhysicalDevice physical_device;
  
  VkQueueFamilyProperties *queue_families;
  uint32_t queue_family_count;
} vkh_device;

vkh_physical_device vkh_physical_device_init(VkInstance instance,
					     uint32_t vulkan_version);

void vkh_enable_device_extension(vkh_physical_device *s, char *name);

void vkh_set_surface(vkh_physical_device *s, VkSurfaceKHR surface);

void vkh_set_features13(vkh_physical_device *s,
			VkPhysicalDeviceVulkan13Features features);

void vkh_set_features12(vkh_physical_device *s,
			VkPhysicalDeviceVulkan12Features features);

void vkh_set_features11(vkh_physical_device *s,
			VkPhysicalDeviceVulkan11Features features);

void vkh_set_features(vkh_physical_device *s,
		      VkPhysicalDeviceFeatures2 features);

void vkh_enable_features_pnext(vkh_physical_device *s, void *pnext);

void vkh_physical_device_select(vkh_physical_device *s);

vkh_device vkh_device_create(vkh_physical_device pdev);

VkQueue vkh_device_get_queue(vkh_device dev, uint32_t type, uint32_t *family);

void vkh_device_cleanup(vkh_device dev);

typedef struct {
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkSurfaceKHR surface;
  uint32_t desired_width, desired_height;

  VkSurfaceFormatKHR desired_format;
  uint32_t desired_present_mode;

  uint32_t image_usage_flags;
} vkh_swapchain_builder;

typedef struct {
  VkSwapchainKHR swapchain;
  VkImage *images;
  VkImageView *image_views;
  uint32_t image_count;
  VkExtent2D extent;
} vkh_swapchain;

void vkh_set_desired_format(vkh_swapchain_builder *b, VkSurfaceFormatKHR f);

void vkh_set_desired_present_mode(vkh_swapchain_builder *b, uint32_t mode);

void vkh_add_image_usage_flags(vkh_swapchain_builder *b, uint32_t flags);

vkh_swapchain vkh_swapchain_build(vkh_swapchain_builder b);

void vkh_destroy_swapchain(VkDevice device, vkh_swapchain swapchain);

VkImageCreateInfo vkh_image_create_info(VkFormat format, VkExtent3D extent,
					VkImageUsageFlags usage);

VkImageViewCreateInfo vkh_image_view_create_info(VkFormat format,
						 VkImage image,
						 VkImageAspectFlags flags);

bool vkh_load_shader_module(const char *path, VkDevice device,
			    VkShaderModule *out_module);

VkImageSubresourceRange
vkh_image_subresource_range(VkImageAspectFlags flags);

void vkh_transition_image(VkCommandBuffer cmd, VkImage img,
			  VkImageLayout now, VkImageLayout next);

void vkh_copy_image_to_image(VkCommandBuffer cmd, VkImage src,
			     VkImage dst, VkExtent2D srce,
			     VkExtent2D dste);

#endif
#ifdef VK_HELP_IMPL
// function below copied from this file
//#include <vulkan/vk_enum_string_helper.h>
// since the file itself is not c compatible (would have to be compiled with c++)
// since it includes <string> 
const char* string_VkResult(VkResult input_value)
{
    switch (input_value)
    {
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
#endif // VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        default:
            return "Unhandled VkResult";
    }
}

#define VK_CHECK(x)							\
  do {									\
    VkResult err = x;							\
    if (err) {								\
      fprintf(stderr, "Vulkan Error: %s", string_VkResult(err));	\
      exit(1);								\
    }									\
  } while(0);

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT
					*create_info,
					const VkAllocationCallbacks *alloc,
					VkDebugUtilsMessengerEXT *out_messenger)
{
  PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (fn) {
    return fn(instance, create_info, alloc, out_messenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
				     VkDebugUtilsMessengerEXT messenger,
				     const VkAllocationCallbacks *alloc) {
  PFN_vkDestroyDebugUtilsMessengerEXT fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (fn) {
    fn(instance, messenger, alloc);
  }
}

vkh_darr_str_t vkh_darr_str_init(size_t initial_cap) {
  return (vkh_darr_str_t) {
    .data = vkh_context.calloc(initial_cap, sizeof(char *)),
    .len = 0,
    .cap = initial_cap,
  };
}

void vkh_darr_str_free(vkh_darr_str_t *arr) {
  vkh_context.free(arr->data);
  *arr = (vkh_darr_str_t){};
}

// we dont own the string!
void vkh_darr_str_push(vkh_darr_str_t *arr, char *new) {
  if (arr->len + 1 > arr->cap) {
    if (arr->cap == 0) { arr->cap = 1; }
    arr->cap *= 2;
    arr->data = vkh_context.realloc(arr->data, arr->cap * sizeof(char *));
  }
  arr->data[arr->len++] = new;
}

VkApplicationInfo vkh_default_app_info() {
  return (VkApplicationInfo) {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Vulkan App",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "",
    .engineVersion = VK_MAKE_VERSION(0, 0, 0),
    .apiVersion = VK_API_VERSION_1_3,
  };
}

vkh_instance_builder vkh_new_instance_builder() {
  return (vkh_instance_builder) {
    .app_info = vkh_default_app_info(),
    .layer_names = vkh_darr_str_init(1),
    .extension_names = vkh_darr_str_init(1),
  };
}

void vkh_set_api_version(vkh_instance_builder *b, uint32_t api_version) {
  b->app_info.apiVersion = api_version;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vkh_default_debug_messenger_cb(VkDebugUtilsMessageSeverityFlagBitsEXT msg_sev,
			       VkDebugUtilsMessageTypeFlagsEXT msg_type,
			       const VkDebugUtilsMessengerCallbackDataEXT *d,
			       void *user_data) {
  (void) msg_sev;
  (void) msg_type;
  (void) user_data;
  fprintf(stderr, "Validation Layer: %s\n", d->pMessage);

  return VK_FALSE;
}

void vkh_enable_debug_messenger(vkh_instance_builder *b,
				VkDebugUtilsMessengerCreateInfoEXT *info) {
  b->enable_debug_messenger = true;
  if (info != NULL) {
    info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    b->dbg_create_info = *info;
    return;
  }  
  b->dbg_create_info = (VkDebugUtilsMessengerCreateInfoEXT) {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = vkh_default_debug_messenger_cb,
  };
}

void vkh_enable_layer(vkh_instance_builder *b, char *name) {
  vkh_darr_str_push(&b->layer_names, name);
}

void vkh_enable_layers(vkh_instance_builder *b, size_t count, char *names[restrict count]) {
  for (size_t i = 0; i < count; ++i) {
    vkh_darr_str_push(&b->layer_names, names[i]);
  }
}

void vkh_enable_validation(vkh_instance_builder *b) {
  vkh_enable_layer(b, "VK_LAYER_KHRONOS_validation");
}

void vkh_enable_extension(vkh_instance_builder *b, char *name) {
  vkh_darr_str_push(&b->extension_names, name);
}

void vkh_enable_extensions(vkh_instance_builder *b, size_t count, char *names[restrict count]) {
  for (size_t i = 0; i < count; ++i) {
    vkh_darr_str_push(&b->extension_names, names[i]);
  }
}

vkh_instance vkh_instance_build(vkh_instance_builder b) {
  VkInstanceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &b.app_info,
    .enabledExtensionCount = b.extension_names.len,
    .ppEnabledExtensionNames = (const char * const *)b.extension_names.data,
    .enabledLayerCount = b.layer_names.len,
    .ppEnabledLayerNames = (const char * const *)b.layer_names.data,    
  };
  vkh_instance res;
  VK_CHECK(vkCreateInstance(&info, NULL, &res.instance));
  if (b.enable_debug_messenger) {
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(res.instance, &b.dbg_create_info,
					    NULL, &res.messenger));
  }
  vkh_darr_str_free(&b.layer_names);
  vkh_darr_str_free(&b.extension_names);
  return res;
}

vkh_physical_device vkh_physical_device_init(VkInstance instance,
					     uint32_t vulkan_version) {
  return (vkh_physical_device) {
    .instance = instance,
    .min_version = vulkan_version,
  };
}

void vkh_enable_device_extension(vkh_physical_device *s, char *name) {
  vkh_darr_str_push(&s->extensions, name);
}

void vkh_set_surface(vkh_physical_device *s, VkSurfaceKHR surface) {
  s->surface = surface;
  // if we want to draw to a surface, we need a swapchain too
  vkh_enable_device_extension(s, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void vkh_set_features13(vkh_physical_device *s,
			VkPhysicalDeviceVulkan13Features features) {
  s->features13 = features;
}

void vkh_set_features12(vkh_physical_device *s,
			VkPhysicalDeviceVulkan12Features features) {
  s->features12 = features;
}

void vkh_set_features11(vkh_physical_device *s,
			VkPhysicalDeviceVulkan11Features features) {
  s->features11 = features;
}

void vkh_set_features(vkh_physical_device *s,
			VkPhysicalDeviceFeatures2 features) {
  s->features = features;
}

void vkh_enable_features_pnext(vkh_physical_device *s, void *pnext) {
  s->pnext = pnext;
}

void vkh_physical_device_select(vkh_physical_device *s) {
  // TODO: any real selection process
  // right now just grab the first device that matches the api version :3
  uint32_t dev_cnt;
  vkEnumeratePhysicalDevices(s->instance, &dev_cnt, NULL);
  VkPhysicalDevice *devs = vkh_context.calloc(dev_cnt, sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(s->instance, &dev_cnt, devs);

  for (uint32_t i = 0; i < dev_cnt; ++i) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(devs[i], &props);
    if (props.apiVersion >= s->min_version) {
      s->physical_device = devs[i];
      break;
    }
  }
  vkh_context.free(devs);
  if (s->physical_device != VK_NULL_HANDLE) {
    s->selected = true;
  } else {
    fprintf(stderr, "Failed to find a physical device that works\n");
    exit(1);
  }
}

vkh_device vkh_device_create(vkh_physical_device pdev) {
  pdev.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  pdev.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  pdev.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  pdev.features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

  pdev.features.pNext = &pdev.features11;
  pdev.features11.pNext = &pdev.features12;
  pdev.features12.pNext = &pdev.features13;
  pdev.features13.pNext = pdev.pnext;

  // create queues for every queue family
  uint32_t qf_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(pdev.physical_device,
					   &qf_count, NULL);
  VkQueueFamilyProperties *qf_props = vkh_context.calloc(qf_count, sizeof(*qf_props));
  vkGetPhysicalDeviceQueueFamilyProperties(pdev.physical_device,
					   &qf_count, qf_props);
  VkDeviceQueueCreateInfo *queue_infos = vkh_context.calloc(qf_count,
							    sizeof(*queue_infos));
  
  float queue_priorities[32];
  for(size_t i = 0; i < 32; ++i) {
    queue_priorities[i] = 1.f;
  }
  for (uint32_t i = 0; i < qf_count; ++i) {
    queue_infos[i] = (VkDeviceQueueCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = i,
      .queueCount = qf_props[i].queueCount,
      .pQueuePriorities = queue_priorities,
    };
  }
  
  VkDeviceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &pdev.features,
    .enabledExtensionCount = pdev.extensions.len,
    .ppEnabledExtensionNames = (const char* const*)pdev.extensions.data,
    .queueCreateInfoCount = qf_count,
    .pQueueCreateInfos = queue_infos,
  };

  VkDevice device;

  VK_CHECK(vkCreateDevice(pdev.physical_device, &info, NULL, &device));
  
  vkh_context.free(queue_infos);
  vkh_darr_str_free(&pdev.extensions);
  return (vkh_device) {
    .device = device,
    .physical_device = pdev.physical_device,
    .queue_families = qf_props,
    .queue_family_count = qf_count,
  };
}

VkQueue vkh_device_get_queue(vkh_device dev, uint32_t type, uint32_t *family) {
  *family = dev.queue_family_count;
  for (uint32_t i = 0; i < dev.queue_family_count; ++i) {
    if (dev.queue_families[i].queueFlags & type) {
      *family = i;
      break;
    }
  }
  if (*family >= dev.queue_family_count) {
    // error
    return VK_NULL_HANDLE;
  }
  VkQueue res;
  vkGetDeviceQueue(dev.device, *family, 0, &res);
  return res;
}

void vkh_device_cleanup(vkh_device dev) {
  vkh_context.free(dev.queue_families);
}

void vkh_set_desired_format(vkh_swapchain_builder *b, VkSurfaceFormatKHR f) {
  b->desired_format = f;
}

void vkh_set_desired_present_mode(vkh_swapchain_builder *b, uint32_t mode) {
  b->desired_present_mode = mode;
}

void vkh_add_image_usage_flags(vkh_swapchain_builder *b, uint32_t flags) {
  b->image_usage_flags |= flags;
}

vkh_swapchain vkh_swapchain_build(vkh_swapchain_builder b) {
  VkSurfaceCapabilitiesKHR abilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(b.physical_device,
					    b.surface, &abilities);
  // clamp dimensions
  if (b.desired_width < abilities.minImageExtent.width) {
    /* fprintf(stderr, "Width adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_width, */
    /* 	    abilities.minImageExtent.width);	     */
    b.desired_width = abilities.minImageExtent.width;
  }
  if (b.desired_width > abilities.maxImageExtent.width) {
    /* fprintf(stderr, "Width adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_width, */
    /* 	    abilities.maxImageExtent.width); */
    b.desired_width = abilities.maxImageExtent.width;
  }
  if (b.desired_height < abilities.minImageExtent.height) {
    /* fprintf(stderr, "Height adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_height, */
    /* 	    abilities.minImageExtent.height); */
    b.desired_height = abilities.minImageExtent.height;
  }
  if (b.desired_height > abilities.maxImageExtent.height) {
    /* fprintf(stderr, "Height adjusted from %d to %d due to device constraints\n", */
    /* 	    b.desired_height, */
    /* 	    abilities.maxImageExtent.height); */
    b.desired_height = abilities.maxImageExtent.height;
  }

  uint32_t pm_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(b.physical_device, b.surface,
					    &pm_count, NULL);
  if (pm_count == 0) {
    fprintf(stderr, "Device does not support presenting to this surface!");
    exit(1);
  }
  VkPresentModeKHR *modes = vkh_context.calloc(pm_count, sizeof(*modes));
  vkGetPhysicalDeviceSurfacePresentModesKHR(b.physical_device, b.surface,
					    &pm_count, modes);
  bool mode_valid = false;
  for (uint32_t i = 0; i < pm_count; ++i) {
    if (modes[i] == b.desired_present_mode) {
      mode_valid = true;
      break;
    }
  }
  vkh_context.free(modes);
  if (!mode_valid) {
    fprintf(stderr, "Device does not support desired present mode");
    exit(1);
  }

  VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = b.surface,
    .minImageCount = abilities.minImageCount,
    .imageFormat = b.desired_format.format,
    .imageColorSpace = b.desired_format.colorSpace,
    .imageExtent = (VkExtent2D) {b.desired_width, b.desired_height},
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | b.image_usage_flags,
    .preTransform = abilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = b.desired_present_mode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  vkh_swapchain res;
  VK_CHECK(vkCreateSwapchainKHR(b.device, &info, NULL, &res.swapchain));

  vkGetSwapchainImagesKHR(b.device, res.swapchain, &res.image_count, NULL);
  res.images = vkh_context.calloc(res.image_count, sizeof(*res.images));
  vkGetSwapchainImagesKHR(b.device, res.swapchain,
			  &res.image_count, res.images);
  res.image_views = vkh_context.calloc(res.image_count, sizeof(*res.image_views));
  for (uint32_t i = 0; i < res.image_count; ++i) {
    VkImageViewCreateInfo image_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = res.images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = b.desired_format.format,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.levelCount = 1,
      .subresourceRange.layerCount = 1,
    };
    VK_CHECK(vkCreateImageView(b.device, &image_view_info, NULL,
			       &res.image_views[i]));
  }
  res.extent = (VkExtent2D) {b.desired_width, b.desired_height};
  return res;
}

void vkh_destroy_swapchain(VkDevice device, vkh_swapchain swapchain) {
  vkDestroySwapchainKHR(device, swapchain.swapchain, NULL);
  for (uint32_t i = 0; i < swapchain.image_count; ++i) {
    vkDestroyImageView(device, swapchain.image_views[i], NULL);
  }
  vkh_context.free(swapchain.images);
  vkh_context.free(swapchain.image_views);
}

VkImageCreateInfo vkh_image_create_info(VkFormat format, VkExtent3D extent,
					VkImageUsageFlags usage) {
  return (VkImageCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = extent,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = usage
  };
}

VkImageViewCreateInfo vkh_image_view_create_info(VkFormat format,
						 VkImage image,
						 VkImageAspectFlags flags) {
  return (VkImageViewCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .image = image,
    .format = format,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
    .subresourceRange.aspectMask = flags
  };
}

bool vkh_load_shader_module(const char *path, VkDevice device,
			    VkShaderModule *out_module) {
  FILE *file = fopen(path, "rb");
  if (!file) { return false; }

  fseek(file, 0, SEEK_END);
  uint32_t file_size = (uint32_t)ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = vkh_context.malloc(file_size);
  size_t sz = fread(buffer, file_size, 1, file);
  if (!sz) {
    fprintf(stderr, "Failed to read file %s\n", path);
    return false;
  }
  fclose(file);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = file_size,
    .pCode = (uint32_t *)buffer,
  };

  if (vkCreateShaderModule(device, &info, NULL, out_module)
      != VK_SUCCESS) {
    return false;
  }
  return true;
}

VkImageSubresourceRange
vkh_image_subresource_range(VkImageAspectFlags flags) {
  return (VkImageSubresourceRange) {
    .aspectMask = flags,
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
}

void vkh_transition_image(VkCommandBuffer cmd, VkImage img,
			  VkImageLayout now, VkImageLayout next) {
  VkImageMemoryBarrier2 barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
 .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT |
                     VK_ACCESS_2_MEMORY_READ_BIT,
    
    .oldLayout = now,
    .newLayout = next
  };

  VkImageAspectFlags flags =
    (next == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
    VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageSubresourceRange rng = vkh_image_subresource_range(flags);
  barrier.subresourceRange = rng;
  barrier.image = img;

  VkDependencyInfo dep_info = {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

void vkh_copy_image_to_image(VkCommandBuffer cmd, VkImage src,
			     VkImage dst, VkExtent2D srce,
			     VkExtent2D dste) {
  VkImageBlit2 blit_region = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
  
    .srcOffsets[1].x = srce.width,
    .srcOffsets[1].y = srce.height,
    .srcOffsets[1].z = 1,

    .dstOffsets[1].x = dste.width,
    .dstOffsets[1].y = dste.height,
    .dstOffsets[1].z = 1,

    .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .srcSubresource.layerCount = 1,  
    .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .dstSubresource.layerCount = 1,
  };

  VkBlitImageInfo2 blit_info = {
    .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
    .dstImage = dst,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .srcImage = src,
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .filter = VK_FILTER_LINEAR,
    .regionCount = 1,
    .pRegions = &blit_region,
  };

  vkCmdBlitImage2(cmd, &blit_info);
}
#endif
