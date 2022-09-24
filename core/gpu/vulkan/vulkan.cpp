//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/gpu/vulkan/vulkan.h"

#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/fixed_array.inl"
#include "core/linear_allocator.inl"
#include "core/os.h"
#include "core/log.h"
#include "core/string.inl"
#include "core/utils.h"

#define M_vk_check(condition) { \
  VkResult vk_result = condition; \
  M_check(vk_result == VK_SUCCESS); \
}

#define M_vk_check_return(condition) { \
  VkResult vk_result = condition; \
  M_check_return(vk_result == VK_SUCCESS); \
}

#define M_vk_check_return_false(condition) { \
  VkResult vk_result = condition; \
  M_check_return_val(vk_result == VK_SUCCESS, false); \
}

#define M_vk_check_return_val(condition, val) { \
  VkResult vk_result = condition; \
  M_check_return_val(vk_result == VK_SUCCESS, val); \
}

struct Vk_sub_buffer_t_ {
  VkDescriptorBufferInfo bi;
  U8* cpu_p = NULL;
};

struct Vulkan_texture_t : Texture_t {
  VkImage image;
  VkDeviceMemory memory;
};

struct Vulkan_sampler_t : Sampler_t {
  VkSampler sampler;
};

struct Vulkan_uniform_buffer_t : Uniform_buffer_t {
  Vk_sub_buffer_t_ sub_buffer;
};

struct Vulkan_vertex_buffer_t : Vertex_buffer_t {
  Vk_sub_buffer_t_ sub_buffer;
};

struct Vulkan_index_buffer_t : Index_buffer_t {
  Vk_sub_buffer_t_ sub_buffer;
};

struct Vulkan_shader_t : Shader_t {
  VkShaderModule shader;
};

struct Vulkan_resources_set_t : Resources_set_t {
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
  U8 binding;
};

struct Vulkan_render_target_t : Render_target_t {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView image_view;
};

struct Vulkan_render_pass_t : Render_pass_t {
  VkRenderPass render_pass;
  VkFramebuffer framebuffers[4] = {};
  Fixed_array_t<VkClearValue, 4> clear_values;
};

struct Vulkan_pipeline_layout_t : Pipeline_layout_t {
  VkPipelineLayout pipeline_layout;
};

struct Vulkan_pipeline_state_object_t : Pipeline_state_object_t {
  VkPipeline pso;
};

struct Vulkan_image_view_t : Image_view_t {
  VkImageView image_view;
};

static VkFormat convert_format_to_vk_format(E_format format) {
  switch (format) {
    case e_format_r32g32b32a32_float:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
    case e_format_r32g32b32a32_uint:
      return VK_FORMAT_R32G32B32A32_UINT;
    case e_format_r32g32b32_float:
      return VK_FORMAT_R32G32B32_SFLOAT;
    case e_format_r32g32_float:
      return VK_FORMAT_R32G32_SFLOAT;
    case e_format_r8_uint:
      return VK_FORMAT_R8_UINT;
    case e_format_r8_unorm:
      return VK_FORMAT_R8_UNORM;
    case e_format_r8g8b8a8_uint:
      return VK_FORMAT_R8G8B8A8_UINT;
    case e_format_r8g8b8a8_unorm:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case e_format_r16_uint:
      return VK_FORMAT_R16_UINT;
    case e_format_r16_unorm:
      return VK_FORMAT_R16_UNORM;
    case e_format_r16g16b16a16_uint:
      return VK_FORMAT_R16G16B16A16_UINT;
    case e_format_r16g16b16a16_unorm:
      return VK_FORMAT_R16G16B16A16_UNORM;
    case e_format_r24_unorm_x8_typeless:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    case e_format_bc7_unorm:
      return VK_FORMAT_BC7_UNORM_BLOCK;
    default:
      M_unimplemented();
  }
  return (VkFormat)0;
}

static VkImageLayout convert_resource_state_to_image_layout_(E_resource_state state) {
  switch(state) {
    case e_resource_state_undefined:
      return VK_IMAGE_LAYOUT_UNDEFINED;
    case e_resource_state_render_target:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case e_resource_state_depth_write:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case e_resource_state_depth_read:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case e_resource_state_present:
      return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case e_resource_state_pixel_shader_resource:
      return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    default:
      M_unimplemented();
  }
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

static VkShaderStageFlags convert_shader_stage_to_state_flags_(E_shader_stage visibility) {
  VkShaderStageFlags rv = 0;
  if (e_shader_stage_vertex & visibility) {
    rv |= VK_SHADER_STAGE_VERTEX_BIT;
  }
  if (e_shader_stage_fragment & visibility) {
    rv |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  return rv;
}

static VkBool32 debug_cb_(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, U64 src_obj, Sz location, S32 msg_code, const char* layer_prefix, const char* msg, void* user_data) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    M_logf("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    M_logw("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	}
	return VK_FALSE;
}

bool Vulkan_t::init(Window_t* w) {
  m_window = w;
  M_check_return_false(vulkan_loader_init());
  Linear_allocator_t<> temp_allocator("vulkan_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ngen";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "ngen";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    const char* instance_exts[] = {
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
      "VK_KHR_surface",
#if M_os_is_win()
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif M_os_is_linux()
      VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#else
#error "?"
#endif
    };
    const char* layers[] = {
      "VK_LAYER_KHRONOS_validation",
    };
    VkInstanceCreateInfo instance_ci = {};
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pApplicationInfo = &app_info;
    instance_ci.enabledExtensionCount = static_array_size(instance_exts);
    instance_ci.ppEnabledExtensionNames = instance_exts;
    instance_ci.enabledLayerCount = static_array_size(layers);
    instance_ci.ppEnabledLayerNames = layers;
    M_vk_check(vkCreateInstance(&instance_ci, NULL, &m_instance));
  }

  {
#if M_os_is_win()
    VkWin32SurfaceCreateInfoKHR win32_surface_ci = {};
    win32_surface_ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_ci.hwnd = m_window->m_platform_data.hwnd;
    win32_surface_ci.hinstance = GetModuleHandle(NULL);
    M_vk_check(vkCreateWin32SurfaceKHR(m_instance, &win32_surface_ci, NULL, &m_surface));
#elif M_os_is_linux()
    VkXcbSurfaceCreateInfoKHR xcb_surface_ci = {};
    xcb_surface_ci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_surface_ci.connection = m_window->m_platform_data.xcb_connection;
    xcb_surface_ci.window = m_window->m_platform_data.xcb_window_id;
    M_vk_check(vkCreateXcbSurfaceKHR(m_instance, &xcb_surface_ci, NULL, &m_surface));
#else
#error "?"
#endif
  }

  {
    U32 gpu_count = 0;
    M_vk_check(vkEnumeratePhysicalDevices(m_instance, &gpu_count, NULL));
    Dynamic_array_t<VkPhysicalDevice> physical_devices(&temp_allocator);
    M_scope_exit(physical_devices.destroy());
    physical_devices.resize(gpu_count);
    M_vk_check(vkEnumeratePhysicalDevices(m_instance, &gpu_count, physical_devices.m_p));

    VkPhysicalDeviceProperties device_props;
    m_chosen_device = physical_devices[0];
    for (int i = 0; i < gpu_count; i++) {
      vkGetPhysicalDeviceProperties(physical_devices[i], &device_props);
      if (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        m_chosen_device = physical_devices[i];
        break;
      }
    }
    vkGetPhysicalDeviceMemoryProperties(m_chosen_device, &m_mem_props);
  }

  {
    vkGetPhysicalDeviceProperties(m_chosen_device, &m_device_props);
    m_min_alignment = m_device_props.limits.minMemoryMapAlignment;
    M_logi("Chosen GPU: %s", m_device_props.deviceName);
  }

  int m_transfer_queue_idx = -1;
  {
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_chosen_device, &queue_family_count, NULL);
    Dynamic_array_t<VkQueueFamilyProperties> queue_properties_array(&temp_allocator);
    M_scope_exit(queue_properties_array.destroy());
    queue_properties_array.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_chosen_device, &queue_family_count, queue_properties_array.m_p);
    for (int i = 0; i < queue_family_count; ++i) {
      VkBool32 can_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(m_chosen_device, i, m_surface, &can_present);

      if (m_graphics_q_idx == -1 && queue_properties_array[i].queueCount > 0 && queue_properties_array[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        m_graphics_q_idx = i;
      }
      if (m_present_q_idx == -1 && can_present) {
        m_present_q_idx = i;
      }
      if (m_graphics_q_idx >= 0 && m_present_q_idx >= 0) {
        break;
      }
    }
    for (int i = 0; i < queue_family_count; ++i) {
      VkBool32 can_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(m_chosen_device, i, m_surface, &can_present);

      if (queue_properties_array[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
        m_transfer_queue_idx = i;
        break;
      }
    }
  }

  {
    float q_priority = 1.0f;

    Fixed_array_t<VkDeviceQueueCreateInfo, 4> q_cis;
    VkDeviceQueueCreateInfo q_ci = {};
    q_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci.queueFamilyIndex = m_graphics_q_idx;
    q_ci.queueCount = 1;
    q_ci.pQueuePriorities = &q_priority;
    q_cis.append(q_ci);

    if (m_present_q_idx != m_graphics_q_idx) {
      q_ci.queueFamilyIndex = m_present_q_idx;
      q_cis.append(q_ci);
    }

    if (m_transfer_queue_idx != m_graphics_q_idx) {
      q_ci.queueFamilyIndex = m_transfer_queue_idx;
      q_cis.append(q_ci);
    }

    const char* device_exts[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const char* layers[] = {
      "VK_LAYER_KHRONOS_validation",
    };
    VkDeviceCreateInfo device_ci = {};
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.queueCreateInfoCount = q_cis.len();
    device_ci.pQueueCreateInfos = q_cis.m_p;
    device_ci.enabledExtensionCount = static_array_size(device_exts);
    device_ci.ppEnabledExtensionNames = device_exts;
    device_ci.pEnabledFeatures = NULL;
    device_ci.enabledLayerCount = static_array_size(layers);
    device_ci.ppEnabledLayerNames = layers;
    M_vk_check(vkCreateDevice(m_chosen_device, &device_ci, NULL, &m_device));
    vkGetDeviceQueue(m_device, m_graphics_q_idx, 0, &m_graphics_q);
    vkGetDeviceQueue(m_device, m_present_q_idx, 0, &m_present_q);
    vkGetDeviceQueue(m_device, m_transfer_queue_idx, 0, &m_transfer_q);
  }

	VkDebugReportCallbackEXT callback;
  {
    VkDebugReportCallbackCreateInfoEXT dbg_report_ci = {};
    dbg_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dbg_report_ci.pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_cb_;
    dbg_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");

    M_vk_check(vkCreateDebugReportCallbackEXT(m_instance, &dbg_report_ci, NULL, &callback));
  }

  {
    VkCommandPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.queueFamilyIndex = m_graphics_q_idx;
    pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    M_vk_check(vkCreateCommandPool(m_device, &pool_ci, NULL, &m_graphics_cmd_pool));

    if (m_transfer_queue_idx != m_graphics_q_idx) {
      pool_ci.queueFamilyIndex = m_transfer_queue_idx;
      M_vk_check(vkCreateCommandPool(m_device, &pool_ci, NULL, &m_transfer_cmd_pool));
    } else {
      m_transfer_cmd_pool = m_graphics_cmd_pool;
    }
  }

  M_scope_exit(m_swapchain_images.destroy());
  // Select swap chain size
  VkExtent2D swapchain_extent = { (U32)m_window->m_width, (U32)m_window->m_height };
  {
    // // Find m_surface capabilities
    VkSurfaceCapabilitiesKHR surface_capabilities;
    M_vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_chosen_device, m_surface, &surface_capabilities));
    // Find supported m_surface formats
    U32 format_count;
    M_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_chosen_device, m_surface, &format_count, NULL));

    VkSurfaceFormatKHR surface_format = {};
    {
      Dynamic_array_t<VkSurfaceFormatKHR> surface_formats(&temp_allocator);
      M_scope_exit(surface_formats.destroy());
      surface_formats.resize(format_count);
      M_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_chosen_device, m_surface, &format_count, surface_formats.m_p));
      surface_format = surface_formats[0];
      for (int i = 0; i < surface_formats.len(); ++i) {
        if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
          surface_format = surface_formats[i];
          break;
        }
      }
    }
    m_swapchain_format = surface_format.format;

    // Determine number of images for swap chain
    U32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount != 0 && image_count > surface_capabilities.maxImageCount) {
      image_count = surface_capabilities.maxImageCount;
    }

    // Determine transformation to use (preferring no transform)
    VkSurfaceTransformFlagBitsKHR surface_transform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
      surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
      surface_transform = surface_capabilities.currentTransform;
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR ;

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = m_surface;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = surface_format.format;
    swapchain_ci.imageColorSpace = surface_format.colorSpace;
    swapchain_ci.imageExtent = swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = NULL;
    swapchain_ci.preTransform = surface_transform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;

    M_vk_check(vkCreateSwapchainKHR(m_device, &swapchain_ci, NULL, &m_swapchain));

    U32 swapchain_image_count = 0;
    M_vk_check(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchain_image_count, NULL));

    m_swapchain_images.resize(swapchain_image_count);

    M_vk_check(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchain_image_count, m_swapchain_images.m_p));
    m_swapchain_image_views.resize(m_swapchain_images.len());
    {
      for (int i = 0; i < m_swapchain_images.len(); ++i) {
        m_swapchain_image_views[i] = create_image_view_(m_swapchain_images[i], VK_IMAGE_VIEW_TYPE_2D, m_swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT);
      }
    }
  }

  {
    VkDescriptorPoolSize pool_sizes[3];
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 10;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pool_sizes[1].descriptorCount = 10;
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pool_sizes[2].descriptorCount = 10;

    VkDescriptorPoolCreateInfo descriptor_pool_ci = {};
    descriptor_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_ci.poolSizeCount = 3;
    descriptor_pool_ci.pPoolSizes = pool_sizes;
    descriptor_pool_ci.maxSets = 30;
    M_vk_check(vkCreateDescriptorPool(m_device, &descriptor_pool_ci, NULL, &m_descriptors_pool));
  }
  create_buffer_(&m_uniform_buffer, 16*1024*1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  create_buffer_(&m_vertex_buffer, 100*1024*1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  create_buffer_(&m_upload_buffer, 100*1024*1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
  m_graphics_cmd_buffers.resize(m_swapchain_images.len());
  {
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = m_graphics_cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = (uint32_t) m_swapchain_images.len();
    M_vk_check_return_false(vkAllocateCommandBuffers(m_device, &cmd_buffer_alloc_info, m_graphics_cmd_buffers.m_p));

    cmd_buffer_alloc_info.commandPool = m_transfer_cmd_pool;
    cmd_buffer_alloc_info.commandBufferCount = 1;
    M_vk_check_return_false(vkAllocateCommandBuffers(m_device, &cmd_buffer_alloc_info, &m_transfer_cmd_buffer));
  }
  {
    m_fences.resize(m_swapchain_images.len());
    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      vkCreateFence(m_device, &fence_ci, NULL, &m_fences[i]);
    }
  }
  {
    VkSemaphoreCreateInfo semaphore_ci = {};
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    M_vk_check(vkCreateSemaphore(m_device, &semaphore_ci, NULL, &m_image_available_semaphore));
    M_vk_check(vkCreateSemaphore(m_device, &semaphore_ci, NULL, &m_rendering_finished_semaphore));
  }

  VkFormat depth_formats[] = {
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D24_UNORM_S8_UINT,
  };
  for (auto depth_format : depth_formats) {
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(m_chosen_device, depth_format, &format_props);
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      m_depth_format = depth_format;
      break;
    }
  }
  return true;
}

void Vulkan_t::destroy() {
}

Texture_t* Vulkan_t::create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) {
  Sz texture_size = ci.row_count * ci.row_pitch;
  memcpy(m_upload_buffer.cpu_p, ci.data, texture_size);
  VkBufferImageCopy copy_region = {};
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.mipLevel = 0;
  copy_region.imageSubresource.baseArrayLayer = 0;
  copy_region.imageSubresource.layerCount = 1;
  copy_region.imageExtent.width = ci.width;
  copy_region.imageExtent.height = ci.height;
  copy_region.imageExtent.depth = 1;
  copy_region.bufferOffset = 0;

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  vkBeginCommandBuffer(m_transfer_cmd_buffer, &cmd_buffer_begin_info);
  VkImage image;
  VkDeviceMemory memory;
  create_image_(&image, &memory, ci.width, ci.height, convert_format_to_vk_format(ci.format), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT);
  VkImageSubresourceRange subresource_range = {};
  subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresource_range.baseMipLevel = 0;
  subresource_range.levelCount = 1;
  subresource_range.layerCount = 1;
  VkImageMemoryBarrier image_barrier = {};
  image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_barrier.image = image;
  image_barrier.subresourceRange = subresource_range;
  image_barrier.srcAccessMask = 0;
  image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  vkCmdPipelineBarrier(m_transfer_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
  vkCmdCopyBufferToImage(m_transfer_cmd_buffer, m_upload_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

  image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  vkCmdPipelineBarrier(m_transfer_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
  // m_device->flushCommandBuffer(m_transfer_cmd_buffer, m_transfer_queue, true);
  {
    vkEndCommandBuffer(m_transfer_cmd_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_transfer_cmd_buffer;

    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = 0;
    VkFence fence;
    vkCreateFence(m_device, &fence_ci, nullptr, &fence);

    vkQueueSubmit(m_transfer_q, 1, &submit_info, fence);
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, (U64)(-1));

    vkDestroyFence(m_device, fence, nullptr);
  }
  auto rv = allocator->construct<Vulkan_texture_t>();
  rv->image = image;
  rv->memory = memory;
  return rv;
}

Texture_t* Vulkan_t::create_texture_cube(Allocator_t* allocator, const Texture_create_info_t& ci) {
  M_check(ci.width == ci.height);
  Sz texture_size = ci.row_count * ci.row_pitch;
  memcpy(m_upload_buffer.cpu_p, ci.data, 6*texture_size);
  VkBufferImageCopy copy_region = {};
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.mipLevel = 0;
  copy_region.imageSubresource.baseArrayLayer = 0;
  copy_region.imageSubresource.layerCount = 6;
  copy_region.imageExtent.width = ci.width;
  copy_region.imageExtent.height = ci.height;
  copy_region.imageExtent.depth = 1;
  copy_region.bufferOffset = 0;

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  vkBeginCommandBuffer(m_transfer_cmd_buffer, &cmd_buffer_begin_info);
  VkImage image;
  VkDeviceMemory memory;
  create_image_(&image, &memory, ci.width, ci.height, convert_format_to_vk_format(ci.format), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
  VkImageSubresourceRange subresource_range = {};
  subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresource_range.baseMipLevel = 0;
  subresource_range.levelCount = 1;
  subresource_range.layerCount = 6;
  VkImageMemoryBarrier image_barrier = {};
  image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_barrier.image = image;
  image_barrier.subresourceRange = subresource_range;
  image_barrier.srcAccessMask = 0;
  image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  vkCmdPipelineBarrier(m_transfer_cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
  vkCmdCopyBufferToImage(m_transfer_cmd_buffer, m_upload_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

  image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  vkCmdPipelineBarrier(m_transfer_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
  // m_device->flushCommandBuffer(m_transfer_cmd_buffer, m_transfer_queue, true);
  {
    vkEndCommandBuffer(m_transfer_cmd_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_transfer_cmd_buffer;

    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = 0;
    VkFence fence;
    vkCreateFence(m_device, &fence_ci, nullptr, &fence);

    vkQueueSubmit(m_transfer_q, 1, &submit_info, fence);
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, (U64)(-1));

    vkDestroyFence(m_device, fence, nullptr);
  }
  auto rv = allocator->construct<Vulkan_texture_t>();
  rv->image = image;
  rv->memory = memory;
  rv->is_cube = true;
  return rv;
  return NULL;
}

Resources_set_t* Vulkan_t::create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci) {
  auto rv = allocator->construct<Vulkan_resources_set_t>();
  rv->binding = ci.binding;
  Fixed_array_t<VkDescriptorSetLayoutBinding, 8> layout_bindings;
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.stageFlags = convert_shader_stage_to_state_flags_(ci.visibility);
  layout_binding.descriptorCount = 1;
  int descriptor_count = 0;
  if (ci.uniform_buffer_count) {
    descriptor_count = ci.uniform_buffer_count;
    layout_binding.binding = GPU_VK_UNIFORM_BINDING_OFFSET + ci.binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  }

  if (ci.sampler_count) {
    descriptor_count = ci.sampler_count;
    layout_binding.binding = GPU_VK_SAMPLER_BINDING_OFFSET + ci.binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  }

  if (ci.image_count) {
    descriptor_count = ci.image_count;
    layout_binding.binding = GPU_VK_TEXTURE_BINDING_OFFSET + ci.binding;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  }
  for (int i = 0; i < descriptor_count; ++i) {
    layout_bindings.append(layout_binding);
    ++layout_binding.binding;
  }

  VkDescriptorSetLayoutCreateInfo set_layout_ci = {};
  set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set_layout_ci.bindingCount = layout_bindings.len();
  set_layout_ci.pBindings = layout_bindings.m_p;
  M_vk_check_return_val(vkCreateDescriptorSetLayout(m_device, &set_layout_ci, NULL, &rv->layout), NULL);

  VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
  descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_alloc_info.descriptorPool = m_descriptors_pool;
  descriptor_set_alloc_info.descriptorSetCount = 1;
  descriptor_set_alloc_info.pSetLayouts = &rv->layout;
  M_vk_check(vkAllocateDescriptorSets(m_device, &descriptor_set_alloc_info, &rv->set));

  return rv;
}

Pipeline_layout_t* Vulkan_t::create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci) {
  M_check_return_val(ci.set_count, NULL);
  Linear_allocator_t<> temp_allocator("vulkan_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  Dynamic_array_t<VkDescriptorSetLayout> layouts(&temp_allocator);
  layouts.reserve(ci.set_count);
  for (int i = 0; i < ci.set_count; ++i) {
    layouts.append(((Vulkan_resources_set_t*)ci.sets[i])->layout);
  }

  auto rv = allocator->construct<Vulkan_pipeline_layout_t>();
  VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.setLayoutCount = ci.set_count;
  pipeline_layout_ci.pSetLayouts = layouts.m_p;
  M_vk_check_return_val(vkCreatePipelineLayout(m_device, &pipeline_layout_ci, NULL, &rv->pipeline_layout), NULL);
  return rv;
}

Vertex_buffer_t* Vulkan_t::create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) {
  auto rv = allocator->construct<Vulkan_vertex_buffer_t>();
  allocate_sub_buffer_(&rv->sub_buffer, &m_vertex_buffer, ci.size, ci.alignment);
  rv->p = rv->sub_buffer.cpu_p;
  return rv;
}

Index_buffer_t* Vulkan_t::create_index_buffer(Allocator_t* allocator, const Index_buffer_create_info_t& ci) {
  auto rv = allocator->construct<Vulkan_index_buffer_t>();
  allocate_sub_buffer_(&rv->sub_buffer, &m_vertex_buffer, ci.size, 256);
  rv->p = rv->sub_buffer.cpu_p;
  return rv;
}

Render_target_t* Vulkan_t::create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) {
  VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (ci.can_be_sampled) {
    usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  auto rv = allocator->construct<Vulkan_render_target_t>();
  create_image_(&rv->image, &rv->memory, (U32)m_window->m_width, (U32)m_window->m_height, m_depth_format, usage, 0);
  VkImageView image_view = create_image_view_(rv->image, VK_IMAGE_VIEW_TYPE_2D, m_depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
  rv->state = e_resource_state_undefined;
  rv->image_view = image_view;
  rv->type = e_render_target_type_depth_stencil;
  return rv;
}

Render_pass_t* Vulkan_t::create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) {
  Linear_allocator_t<> temp_allocator("vulkan_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  auto rv = allocator->construct<Vulkan_render_pass_t>();
  Fixed_array_t<VkAttachmentDescription, 8> attachment_descs;
  Dynamic_array_t<VkAttachmentReference> color_refs(&temp_allocator);
  color_refs.reserve(ci.render_target_count);
  VkAttachmentReference depth_stencil_ref;
  int depth_stencil_ref_count = 0;
  Fixed_array_t<VkImageView, 8> attachments;
  for (int i = 0; i < ci.render_target_count; ++i) {
    VkAttachmentDescription desc = {};
    const Render_target_description_t& rt_desc = ci.descs[i];
    VkAttachmentReference ref;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = convert_resource_state_to_image_layout_(rt_desc.render_target->state);
    desc.finalLayout = convert_resource_state_to_image_layout_(rt_desc.state_after);
    ref.attachment = i;
    ref.layout = convert_resource_state_to_image_layout_(rt_desc.render_pass_state);
    if (rt_desc.render_target->type == e_render_target_type_depth_stencil) {
      desc.format = m_depth_format;
      depth_stencil_ref = ref;
      ++depth_stencil_ref_count;
      VkClearValue clear_value = {};
      clear_value.color = { 1.0f, 1.0f, 1.0f, 1.0f };
      rv->clear_values.append(clear_value);
    } else {
      desc.format = m_swapchain_format;
      color_refs.append(ref);
      VkClearValue clear_value = {};
      clear_value.depthStencil = { 1.0f, 0 };
      rv->clear_values.append(clear_value);
    }
    attachment_descs.append(desc);
    attachments.append(((Vulkan_render_target_t*)ci.descs[i].render_target)->image_view);
    rv->rt_descs.append(rt_desc);
  }
  if (ci.use_swapchain_render_target || ci.is_last) {
    VkAttachmentDescription desc = {};
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    if (ci.should_clear_render_target) {
      desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      VkClearValue clear_value = {};
      clear_value.color = { 1.0f, 1.0f, 1.0f, 1.0f };
      rv->clear_values.append(clear_value);
    } else {
      desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    if (ci.is_last) {
      desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
      desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    desc.format = m_swapchain_format;
    attachment_descs.append(desc);
    VkAttachmentReference ref;
    ref.attachment = attachment_descs.len() - 1;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_refs.append(ref);
  }
  M_check_return_val(depth_stencil_ref_count <= 1, NULL);
  VkSubpassDescription subpass_desc = {};
  subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_desc.colorAttachmentCount = color_refs.len();
  subpass_desc.pColorAttachments = color_refs.m_p;
  if (depth_stencil_ref_count) {
    subpass_desc.pDepthStencilAttachment = &depth_stencil_ref;
  }
  Fixed_array_t<VkSubpassDependency, 8> subpass_deps;
  if (ci.hint == e_render_pass_hint_shadow) {
    subpass_deps.resize(2);
    subpass_deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_deps[0].dstSubpass = 0;
    subpass_deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpass_deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpass_deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_deps[1].srcSubpass = 0;
    subpass_deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_deps[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpass_deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpass_deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  }
  VkRenderPassCreateInfo vk_ci = {};
  vk_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  vk_ci.attachmentCount = attachment_descs.len();
  vk_ci.pAttachments = attachment_descs.m_p;
  vk_ci.subpassCount = 1;
  vk_ci.pSubpasses = &subpass_desc;
  vk_ci.dependencyCount = subpass_deps.len();
  vk_ci.pDependencies = subpass_deps.m_p;
  VkRenderPass render_pass;
  M_vk_check_return_val(vkCreateRenderPass(m_device, &vk_ci, NULL, &render_pass), NULL);
  rv->render_pass = render_pass;
  rv->use_swapchain_render_target = ci.use_swapchain_render_target;
  rv->should_clear_render_target = ci.should_clear_render_target;
  rv->is_last = ci.is_last;
  if (ci.use_swapchain_render_target || ci.is_last) {
    attachments.append(m_swapchain_image_views[0]);
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      attachments[attachments.len() - 1] = m_swapchain_image_views[i];
      VkFramebufferCreateInfo framebuffer_ci = {};
      framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_ci.renderPass = render_pass;
      framebuffer_ci.attachmentCount = attachments.len();
      framebuffer_ci.pAttachments = attachments.m_p;
      framebuffer_ci.width = m_window->m_width;
      framebuffer_ci.height = m_window->m_height;
      framebuffer_ci.layers = 1;
      M_vk_check(vkCreateFramebuffer(m_device, &framebuffer_ci, NULL, &rv->framebuffers[i]));
    }
  } else {
    VkFramebufferCreateInfo framebuffer_ci = {};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.renderPass = render_pass;
    framebuffer_ci.attachmentCount = attachments.len();
    framebuffer_ci.pAttachments = attachments.m_p;
    framebuffer_ci.width = m_window->m_width;
    framebuffer_ci.height = m_window->m_height;
    framebuffer_ci.layers = 1;
    M_vk_check(vkCreateFramebuffer(m_device, &framebuffer_ci, NULL, &rv->framebuffers[0]));
  }
  return rv;
}

Resource_t Vulkan_t::create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) {
  auto ub = allocator->construct<Vulkan_uniform_buffer_t>();
  allocate_sub_buffer_(&ub->sub_buffer, &m_uniform_buffer, ci.size, ci.alignment);
  ub->p = ub->sub_buffer.cpu_p;

  Resource_t rv;
  rv.type = e_resource_type_uniform_buffer;
  rv.uniform_buffer = ub;
  return rv;
}

Resource_t Vulkan_t::create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) {
  VkSampler sampler;
  VkSamplerCreateInfo sampler_ci = {};
  sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_ci.magFilter = VK_FILTER_LINEAR;
  sampler_ci.minFilter = VK_FILTER_LINEAR;
  sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_ci.minLod = 0.0f;
  sampler_ci.maxLod = 0.0f;
  sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  M_vk_check(vkCreateSampler(m_device, &sampler_ci, NULL, &sampler));

  auto vk_sampler = allocator->construct<Vulkan_sampler_t>();
  vk_sampler->sampler = sampler;
  Resource_t rv;
  rv.type = e_resource_type_sampler;
  rv.sampler = vk_sampler;
  return rv;
}

Resource_t Vulkan_t::create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) {
  Resource_t rv;
  auto image_view = allocator->construct<Vulkan_image_view_t>();
  if (ci.render_target) {
    auto vk_rt = (Vulkan_render_target_t*)ci.render_target;
    image_view->image_view = vk_rt->image_view;
  } else if (ci.texture) {
    auto vk_texture = (Vulkan_texture_t*)ci.texture;
    if (vk_texture->is_cube) {
      image_view->image_view = create_image_view_(vk_texture->image, VK_IMAGE_VIEW_TYPE_CUBE, convert_format_to_vk_format(ci.format), VK_IMAGE_ASPECT_COLOR_BIT);
    } else {
      image_view->image_view = create_image_view_(vk_texture->image, VK_IMAGE_VIEW_TYPE_2D, convert_format_to_vk_format(ci.format), VK_IMAGE_ASPECT_COLOR_BIT);
    }
  } else {
    M_logf_return_val(rv, "One of |ci.render_target| or |ci.texture| has to have a valid value");
  }

  rv.type = e_resource_type_image_view;
  rv.image_view = image_view;
  return rv;
}

void Vulkan_t::bind_resource_to_set(const Resource_t& resource, const Resources_set_t* set, int binding) {
  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = ((Vulkan_resources_set_t*)set)->set;
  write_descriptor_set.descriptorCount = 1;
  VkDescriptorImageInfo descriptor_image_info = {};
  switch(resource.type) {
    case e_resource_type_uniform_buffer:
      write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write_descriptor_set.pBufferInfo = &((Vulkan_uniform_buffer_t*)(resource.uniform_buffer))->sub_buffer.bi;
      write_descriptor_set.dstBinding = GPU_VK_UNIFORM_BINDING_OFFSET + binding;
      break;
    case e_resource_type_sampler:
      descriptor_image_info.sampler = ((Vulkan_sampler_t*)resource.sampler)->sampler;
      write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
      write_descriptor_set.dstBinding = GPU_VK_SAMPLER_BINDING_OFFSET + binding;
      write_descriptor_set.pImageInfo = &descriptor_image_info;
      break;
    case e_resource_type_image_view:
      descriptor_image_info.imageView = ((Vulkan_image_view_t*)resource.image_view)->image_view;
      descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
      write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      write_descriptor_set.dstBinding = GPU_VK_TEXTURE_BINDING_OFFSET + binding;
      write_descriptor_set.pImageInfo = &descriptor_image_info;
      break;
    default:
      M_unimplemented();
  };
  vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, NULL);
}

Shader_t* Vulkan_t::compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci) {
  VkShaderModule shader;
  {
    Linear_allocator_t<16*1024> temp_allocator("shader_allocator");
    Path_t path_with_ext = ci.path;
    path_with_ext.m_path_str.append(M_txt(".spv"));
    M_scope_exit(temp_allocator.destroy());
    Dynamic_array_t<U8> shader_file = File_t::read_whole_file_as_binary(&temp_allocator, path_with_ext.m_path);
    VkShaderModuleCreateInfo shader_ci = {};
    shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_ci.codeSize = shader_file.len();
    shader_ci.pCode = (U32*)shader_file.m_p;
    M_vk_check_return_val(vkCreateShaderModule(m_device, &shader_ci, NULL, &shader), NULL);
  }
  auto rv = allocator->construct<Vulkan_shader_t>();
  rv->shader = shader;
  return rv;
}

Pipeline_state_object_t* Vulkan_t::create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci) {
  Linear_allocator_t<16*1024> temp_allocator("shader_allocator");
  M_scope_exit(temp_allocator.destroy());
  Dynamic_array_t<VkPipelineShaderStageCreateInfo> shader_stage_cis(&temp_allocator);
  if (ci.vs) {
    VkPipelineShaderStageCreateInfo shader_stage_ci = {};
    shader_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_ci.module = ((Vulkan_shader_t*)ci.vs)->shader;
    shader_stage_ci.pName = "VSMain";
    shader_stage_cis.append(shader_stage_ci);
  }

  if (ci.ps) {
    VkPipelineShaderStageCreateInfo shader_stage_ci = {};
    shader_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_ci.module = ((Vulkan_shader_t*)ci.ps)->shader;
    shader_stage_ci.pName = "PSMain";
    shader_stage_cis.append(shader_stage_ci);
  }

  Fixed_array_t<VkVertexInputBindingDescription, 16> binding_descs;
  Fixed_array_t<VkVertexInputAttributeDescription, 16> attribute_descs;
  int location = 0;
  for (int i = 0; i < ci.input_slot_count; ++i) {
    const auto& slot = ci.input_slots[i];
    VkVertexInputBindingDescription binding = {};
    binding.binding = slot.slot_num;
    binding.stride = slot.stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_descs.append(binding);
    int offset = 0;
    for (int j = 0; j < slot.input_element_count; ++j) {
      const auto& ci_elem = slot.input_elements[j];
      VkVertexInputAttributeDescription attribute = {};
      int matrix_row_count = ci_elem.matrix_row_count ? ci_elem.matrix_row_count : 1;
      attribute.binding = slot.slot_num;
      attribute.format = convert_format_to_vk_format(ci_elem.format);
      for (int k = 0; k < matrix_row_count; ++k) {
        attribute.location = location++;
        attribute_descs.append(attribute);
        attribute.offset = offset;
        offset += convert_format_to_size_(ci_elem.format);
      }
    }
  }
  VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
  vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_ci.vertexBindingDescriptionCount = binding_descs.len();
  vertex_input_ci.pVertexBindingDescriptions = binding_descs.m_p;
  vertex_input_ci.vertexAttributeDescriptionCount = attribute_descs.len();
  vertex_input_ci.pVertexAttributeDescriptions = attribute_descs.m_p;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
  input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_ci.primitiveRestartEnable = VK_FALSE;

  // Flipped viewport
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = m_window->m_height;
  viewport.width = (float)m_window->m_width;
  viewport.height = -(float)m_window->m_height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = { (U32)m_window->m_width, (U32)m_window->m_height };

  VkPipelineViewportStateCreateInfo viewport_ci = {};
  viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_ci.viewportCount = 1;
  viewport_ci.pViewports = &viewport;
  viewport_ci.scissorCount = 1;
  viewport_ci.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterization_ci = {};
  rasterization_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_ci.depthClampEnable = VK_FALSE;
  rasterization_ci.rasterizerDiscardEnable = VK_FALSE;
  rasterization_ci.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_ci.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_ci.depthBiasEnable = VK_FALSE;
  rasterization_ci.depthBiasConstantFactor = 0.0f;
  rasterization_ci.depthBiasClamp = 0.0f;
  rasterization_ci.depthBiasSlopeFactor = 0.0f;
  rasterization_ci.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisample_ci = {};
  multisample_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_ci.sampleShadingEnable = VK_FALSE;
  multisample_ci.minSampleShading = 1.0f;
  multisample_ci.alphaToCoverageEnable = VK_FALSE;
  multisample_ci.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
  color_blend_attachment_state.blendEnable = VK_FALSE;
  color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo color_blend_state_ci = {};
  color_blend_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_state_ci.logicOpEnable = VK_FALSE;
  color_blend_state_ci.logicOp = VK_LOGIC_OP_COPY;
  color_blend_state_ci.attachmentCount = 1;
  color_blend_state_ci.pAttachments = &color_blend_attachment_state;
  color_blend_state_ci.blendConstants[0] = 0.0f;
  color_blend_state_ci.blendConstants[1] = 0.0f;
  color_blend_state_ci.blendConstants[2] = 0.0f;
  color_blend_state_ci.blendConstants[3] = 0.0f;

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci = {};
  depth_stencil_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  if (ci.enable_depth) {
    depth_stencil_state_ci.depthTestEnable = VK_TRUE;
  }
  depth_stencil_state_ci.depthWriteEnable = VK_TRUE;
  depth_stencil_state_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_state_ci.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_state_ci.stencilTestEnable = VK_FALSE;

  VkGraphicsPipelineCreateInfo pipeline_ci = {};
  pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_ci.stageCount = shader_stage_cis.len();
  pipeline_ci.pStages = shader_stage_cis.m_p;
  pipeline_ci.pVertexInputState = &vertex_input_ci;
  pipeline_ci.pInputAssemblyState = &input_assembly_ci;
  pipeline_ci.pViewportState = &viewport_ci;
  pipeline_ci.pRasterizationState = &rasterization_ci;
  pipeline_ci.pMultisampleState = &multisample_ci;
  pipeline_ci.pDepthStencilState = &depth_stencil_state_ci;
  pipeline_ci.pColorBlendState = &color_blend_state_ci;
  pipeline_ci.layout = ((Vulkan_pipeline_layout_t*)ci.pipeline_layout)->pipeline_layout;
  pipeline_ci.renderPass = ((Vulkan_render_pass_t*)ci.render_pass)->render_pass;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_ci.basePipelineIndex = -1;

  VkPipeline pipeline;
  M_vk_check_return_val(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &pipeline), NULL);
  auto rv = allocator->construct<Vulkan_pipeline_state_object_t>();
  rv->pso = pipeline;
  return rv;
}

void Vulkan_t::get_back_buffer() {
  M_vk_check(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_image_available_semaphore, VK_NULL_HANDLE, &m_next_swapchain_image_idx));
}

void Vulkan_t::cmd_begin() {
  vkWaitForFences(m_device, 1, &m_fences[m_next_swapchain_image_idx], true, UINT64_MAX);
  vkResetFences(m_device, 1, &m_fences[m_next_swapchain_image_idx]);
  vkResetCommandBuffer(get_active_cmd_buffer_(), 0);
  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  vkBeginCommandBuffer(get_active_cmd_buffer_(), &cmd_buffer_begin_info);
}

void Vulkan_t::cmd_begin_render_pass(Render_pass_t* render_pass) {
  VkClearValue clear_values[2] = {};
  clear_values[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
  clear_values[1].depthStencil.depth = 1.0f;

  auto vk_render_pass = (Vulkan_render_pass_t*)render_pass;
  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = vk_render_pass->render_pass;
  if (vk_render_pass->use_swapchain_render_target || vk_render_pass->is_last) {
    render_pass_begin_info.framebuffer = vk_render_pass->framebuffers[m_next_swapchain_image_idx];
  } else {
    render_pass_begin_info.framebuffer = vk_render_pass->framebuffers[0];
  }
  render_pass_begin_info.renderArea.offset = { 0, 0 };
  render_pass_begin_info.renderArea.extent = { (U32)m_window->m_width, (U32)m_window->m_height};
  render_pass_begin_info.clearValueCount = vk_render_pass->clear_values.len();
  render_pass_begin_info.pClearValues = vk_render_pass->clear_values.m_p;
  vkCmdBeginRenderPass(get_active_cmd_buffer_(), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void Vulkan_t::cmd_end_render_pass(Render_pass_t* render_pass) {
  vkCmdEndRenderPass(get_active_cmd_buffer_());
  for (auto& desc : render_pass->rt_descs) {
    desc.render_target->state = desc.state_after;
  }
}

void Vulkan_t::cmd_set_pipeline_state(Pipeline_state_object_t* pso) {
  vkCmdBindPipeline(get_active_cmd_buffer_(), VK_PIPELINE_BIND_POINT_GRAPHICS, ((Vulkan_pipeline_state_object_t*)pso)->pso);
}

void Vulkan_t::cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding) {
  auto vulkan_vb = (Vulkan_vertex_buffer_t*)vb;
  VkDeviceSize offset1 = vulkan_vb->sub_buffer.bi.offset;
  vkCmdBindVertexBuffers(get_active_cmd_buffer_(), binding, 1, &m_vertex_buffer.buffer, &offset1);
}

void Vulkan_t::cmd_set_index_buffer(Index_buffer_t* ib) {
  auto vulkan_ib = (Vulkan_index_buffer_t*)ib;
  VkDeviceSize offset = vulkan_ib->sub_buffer.bi.offset;
  vkCmdBindIndexBuffer(get_active_cmd_buffer_(), m_vertex_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

void Vulkan_t::cmd_set_resource(const Resource_t& resource, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  auto vk_pipeline_layout = (Vulkan_pipeline_layout_t*)pipeline_layout;
  auto vk_set = (Vulkan_resources_set_t*)set;
  vkCmdBindDescriptorSets(get_active_cmd_buffer_(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_layout->pipeline_layout, index, 1, &vk_set->set, 0, NULL);
}

void Vulkan_t::cmd_draw(int vertex_count, int first_vertex) {
  vkCmdDraw(get_active_cmd_buffer_(), vertex_count, 1, first_vertex, 0);
}

void Vulkan_t::cmd_draw_index(int index_count, int instance_count, int first_index, int vertex_offset, int first_instance) {
  vkCmdDrawIndexed(get_active_cmd_buffer_(), index_count, instance_count, first_index, vertex_offset, first_index);
}

void Vulkan_t::cmd_end() {
  vkEndCommandBuffer(get_active_cmd_buffer_());
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &m_image_available_semaphore;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &m_rendering_finished_semaphore;
  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &m_graphics_cmd_buffers[m_next_swapchain_image_idx];
  M_vk_check(vkQueueSubmit(m_graphics_q, 1, &submit_info, m_fences[m_next_swapchain_image_idx]));

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &m_rendering_finished_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &m_swapchain;
  present_info.pImageIndices = &m_next_swapchain_image_idx;
  M_vk_check(vkQueuePresentKHR(m_present_q, &present_info));
}

int Vulkan_t::get_mem_type_idx_(U32 mem_type_bits, VkFlags mem_flags) {
  for (int i = 0; i < m_mem_props.memoryTypeCount; ++i) {
    if ((mem_type_bits & 1 << i) && (m_mem_props.memoryTypes[i].propertyFlags & mem_flags)) {
      return i;
    }
  }
  M_logf("Can't find a suitable memory type");
  return -1;
}

VkImageView Vulkan_t::create_image_view_(VkImage image, VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo image_view_ci = {};
  image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_ci.image = image;
  image_view_ci.viewType = view_type;
  image_view_ci.format = format;
  image_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.subresourceRange.aspectMask = aspect_flags;
  image_view_ci.subresourceRange.baseMipLevel = 0;
  image_view_ci.subresourceRange.levelCount = 1;
  image_view_ci.subresourceRange.baseArrayLayer = 0;
  if (view_type == VK_IMAGE_VIEW_TYPE_CUBE) {
    image_view_ci.subresourceRange.layerCount = 6;
  } else {
    image_view_ci.subresourceRange.layerCount = 1;
  }
  VkImageView image_view;
  M_vk_check(vkCreateImageView(m_device, &image_view_ci, NULL, &image_view));
  return image_view;
}

bool Vulkan_t::create_buffer_(Vk_buffer_t_* buffer, Sz size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlagBits mem_prop_flags) {
  VkBufferCreateInfo buffer_ci = {};
  buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_ci.size = size;
  buffer_ci.usage = usage_flags;
  buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  M_vk_check_return_false(vkCreateBuffer(m_device, &buffer_ci, NULL, &buffer->buffer));

  VkMemoryRequirements mem_reqs = {};
  vkGetBufferMemoryRequirements(m_device, buffer->buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = get_mem_type_idx_(mem_reqs.memoryTypeBits, mem_prop_flags);
  M_vk_check_return_false(vkAllocateMemory(m_device, &alloc_info, NULL, &buffer->memory));
  vkBindBufferMemory(m_device, buffer->buffer, buffer->memory, 0);
  vkMapMemory(m_device, buffer->memory, 0, size, 0, &buffer->cpu_p);
  buffer->size = size;
  return true;
}

void Vulkan_t::allocate_sub_buffer_(Vk_sub_buffer_t_* sub_buffer, Vk_buffer_t_* buffer, Sip size, int alignment) {
  Sip aligned_offset = (buffer->offset_for_sub_buffer + alignment - 1) & ~(alignment - 1);
  M_check_log_return(aligned_offset + size <= buffer->size, "Out of memory");
  sub_buffer->cpu_p = (U8*)buffer->cpu_p + aligned_offset;
  sub_buffer->bi.buffer = buffer->buffer;
  sub_buffer->bi.offset = aligned_offset;
  sub_buffer->bi.range = size;
  buffer->offset_for_sub_buffer = aligned_offset + size;
}

VkCommandBuffer Vulkan_t::get_active_cmd_buffer_() const {
  return m_graphics_cmd_buffers[m_next_swapchain_image_idx];
}

void Vulkan_t::create_image_(VkImage* image, VkDeviceMemory* memory, U32 width, U32 height, VkFormat format, VkImageUsageFlags usage, VkImageCreateFlags flags) {
  VkImageCreateInfo image_ci = {};
  image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_ci.imageType = VK_IMAGE_TYPE_2D;
  image_ci.format = format;
  image_ci.extent = {width, height, 1};
  image_ci.mipLevels = 1;
  if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
    image_ci.arrayLayers = 6;
  } else {
    image_ci.arrayLayers = 1;
  }
  image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
  image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_ci.usage = usage;
  image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_ci.flags = flags;
  M_vk_check_return(vkCreateImage(m_device, &image_ci, NULL, image));

  VkMemoryRequirements image_mem_reqs;
  vkGetImageMemoryRequirements(m_device, *image, &image_mem_reqs);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = image_mem_reqs.size;
  alloc_info.memoryTypeIndex = get_mem_type_idx_(image_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  M_vk_check_return(vkAllocateMemory(m_device, &alloc_info, nullptr, memory));
  vkBindImageMemory(m_device, *image, *memory, 0);
}
