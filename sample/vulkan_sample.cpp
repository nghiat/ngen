//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/linear_allocator.inl"
#include "core/loader/obj.h"
#include "core/log.h"
#include "core/os.h"
#include "core/path.h"
#include "core/path_utils.h"
#include "core/utils.h"
#include "core/window/window.h"
#include "sample/vulkan_loader.h"

#define M_vk_check(condition) { \
  VkResult vk_result = condition; \
  M_check(vk_result == VK_SUCCESS); \
}


static VkBool32 debug_cb(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, U64 src_obj, Sz location, S32 msg_code, const char* layer_prefix, const char* msg, void* user_data) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    M_logf("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    M_logw("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	}
	return VK_FALSE;
}


static VkShaderModule load_shader_(VkDevice m_device, const Path_t& path) {
  Linear_allocator_t<16*1024> allocator("shader_allocator");
  allocator.init();
  M_scope_exit(allocator.destroy());
  Dynamic_array_t<U8> shader_file = File_t::read_whole_file_as_binary(&allocator, path.m_path);
  VkShaderModuleCreateInfo shader_ci = {};
  shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_ci.codeSize = shader_file.len();
  shader_ci.pCode = (U32*)shader_file.m_p;
  VkShaderModule shader;
  M_vk_check(vkCreateShaderModule(m_device, &shader_ci, NULL, &shader));
  return shader;
}

static int get_mem_type_idx(VkPhysicalDeviceMemoryProperties m_mem_props, U32 mem_type_bits, VkFlags mem_flags) {
  for (int i = 0; i < m_mem_props.memoryTypeCount; ++i) {
    if ((mem_type_bits & 1 << i) && (m_mem_props.memoryTypes[i].propertyFlags & mem_flags)) {
      return i;
    }
  }
  M_logf("Can't find a suitable memory type");
  return -1;
}

class Vk_window_t : public Window_t {
public:
  Vk_window_t(const Os_char* title, int w, int h) : Window_t(title, w, h), m_vk_allocator("vk_allocator") {}

  bool init();
  void destroy() override;
  void loop() override;

  Linear_allocator_t<> m_vk_allocator;
  VkInstance m_instance;
  VkSurfaceKHR m_surface;
  VkPhysicalDevice m_chosen_device = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties m_mem_props = {};
  int m_graphics_q_idx = -1;
  int m_present_q_idx = -1;
  VkDevice m_device;
  VkQueue m_graphics_q;
  VkQueue m_present_q;
  VkCommandPool m_cmd_pool;
  VkSwapchainKHR m_swapchain;
  Dynamic_array_t<VkImage> m_swapchain_images;
  VkRenderPass m_render_pass;
  Dynamic_array_t<VkImageView> m_swapchain_image_views;
  Dynamic_array_t<VkFramebuffer> m_framebuffers;
  VkBuffer m_vertex_buffer;
  VkBuffer m_uniform_buffer;
  VkDescriptorSetLayout m_descriptor_set_layout;
  VkPipelineLayout m_pipeline_layout;
  VkPipeline m_graphics_pipeline;
  VkDescriptorPool m_descriptor_pool;
  VkDescriptorSet m_descriptor_set;
  Dynamic_array_t<VkCommandBuffer> m_graphics_cmd_buffers;
};

bool Vk_window_t::init() {
  Window_t::init();
  m_vk_allocator.init();

  vulkan_loader_init();
  Linear_allocator_t<> temp_vk_allocator("temp_vk_allocator");
  temp_vk_allocator.init();
  M_scope_exit(temp_vk_allocator.destroy());
  {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vulkan_sample";
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
    win32_surface_ci.hwnd = m_platform_data.hwnd;
    win32_surface_ci.hinstance = GetModuleHandle(NULL);
    M_vk_check(vkCreateWin32SurfaceKHR(m_instance, &win32_surface_ci, NULL, &m_surface));
#elif M_os_is_linux()
    VkXCBSurfaceCreateInfoKHR xcb_surface_ci = {};
    xcb_surface_ci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_surface_ci.connect = m_platform_data.hwnd;
    xcb_surface_ci.window = GetModuleHandle(NULL);
    M_vk_check(vkCreateWin32SurfaceKHR(m_instance, &win32_surface_ci, NULL, &m_surface));
#else
#error "?"
#endif
  }

  {
    U32 gpu_count = 0;
    M_vk_check(vkEnumeratePhysicalDevices(m_instance, &gpu_count, NULL));
    Dynamic_array_t<VkPhysicalDevice> physical_devices;
    physical_devices.init(&temp_vk_allocator);
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
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(m_chosen_device, &device_props);
    M_logi("Chosen GPU: %s", device_props.deviceName);
  }

  {
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_chosen_device, &queue_family_count, NULL);
    Dynamic_array_t<VkQueueFamilyProperties> queue_properties_array;
    queue_properties_array.init(&temp_vk_allocator);
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
  }

  {
    float q_priority = 1.0f;

    VkDeviceQueueCreateInfo q_ci[2] = {};

    q_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci[0].queueFamilyIndex = m_graphics_q_idx;
    q_ci[0].queueCount = 1;
    q_ci[0].pQueuePriorities = &q_priority;

    q_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci[0].queueFamilyIndex = m_present_q_idx;
    q_ci[0].queueCount = 1;
    q_ci[0].pQueuePriorities = &q_priority;

    const char* device_exts[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const char* layers[] = {
      "VK_LAYER_KHRONOS_validation",
    };
    VkDeviceCreateInfo device_ci = {};
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.pQueueCreateInfos = q_ci;

    if (m_graphics_q_idx == m_present_q_idx) {
      device_ci.queueCreateInfoCount = 1;
    } else {
      device_ci.queueCreateInfoCount = 2;
    }
    device_ci.enabledExtensionCount = static_array_size(device_exts);
    device_ci.ppEnabledExtensionNames = device_exts;
    device_ci.pEnabledFeatures = NULL;
    device_ci.enabledLayerCount = static_array_size(layers);
    device_ci.ppEnabledLayerNames = layers;
    M_vk_check(vkCreateDevice(m_chosen_device, &device_ci, NULL, &m_device));
    vkGetDeviceQueue(m_device, m_graphics_q_idx, 0, &m_graphics_q);
    vkGetDeviceQueue(m_device, m_present_q_idx, 0, &m_present_q);
  }

	VkDebugReportCallbackEXT callback;
  {
    VkDebugReportCallbackCreateInfoEXT dbg_report_ci = {};
    dbg_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dbg_report_ci.pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_cb;
    dbg_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");

    M_vk_check(vkCreateDebugReportCallbackEXT(m_instance, &dbg_report_ci, NULL, &callback));
  }

  {
    VkCommandPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.queueFamilyIndex = m_graphics_q_idx;

    M_vk_check(vkCreateCommandPool(m_device, &pool_ci, NULL, &m_cmd_pool));
  }

  m_swapchain_images.init(&m_vk_allocator);
  M_scope_exit(m_swapchain_images.destroy());
  VkFormat swapchain_format;
  // Select swap chain size
  VkExtent2D swapchain_extent = { (U32)m_width, (U32)m_height };
  {
    // // Find m_surface capabilities
    VkSurfaceCapabilitiesKHR surface_capabilities;
    M_vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_chosen_device, m_surface, &surface_capabilities));
    // Find supported m_surface formats
    U32 format_count;
    M_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_chosen_device, m_surface, &format_count, NULL));

    VkSurfaceFormatKHR surface_format = {};
    {
      Dynamic_array_t<VkSurfaceFormatKHR> surface_formats;
      surface_formats.init(&temp_vk_allocator);
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
    swapchain_format = surface_format.format;

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

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAILBOX_KHR;

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
  }

  {
    VkAttachmentDescription attachment_desc = {};
    attachment_desc.format = swapchain_format;
    attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_desc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachment_ref = {};
    attachment_ref.attachment = 0;
    attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &attachment_ref;

    // Create the render pass
    VkRenderPassCreateInfo m_render_pass_ci = {};
    m_render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    m_render_pass_ci.attachmentCount = 1;
    m_render_pass_ci.pAttachments = &attachment_desc;
    m_render_pass_ci.subpassCount = 1;
    m_render_pass_ci.pSubpasses = &subpass_desc;

    M_vk_check(vkCreateRenderPass(m_device, &m_render_pass_ci, NULL, &m_render_pass));
  }

  m_swapchain_image_views.init(&m_vk_allocator);
  m_swapchain_image_views.resize(m_swapchain_images.len());
  {
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      VkImageViewCreateInfo image_view_ci = {};
      image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      image_view_ci.image = m_swapchain_images[i];
      image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      image_view_ci.format = swapchain_format;
      image_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_view_ci.subresourceRange.baseMipLevel = 0;
      image_view_ci.subresourceRange.levelCount = 1;
      image_view_ci.subresourceRange.baseArrayLayer = 0;
      image_view_ci.subresourceRange.layerCount = 1;
      M_vk_check(vkCreateImageView(m_device, &image_view_ci, NULL, &m_swapchain_image_views[i]));
    }
  }

  m_framebuffers.init(&m_vk_allocator);
  m_framebuffers.resize(m_swapchain_images.len());
  {
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      VkFramebufferCreateInfo framebuffer_ci = {};
      framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_ci.renderPass = m_render_pass;
      framebuffer_ci.attachmentCount = 1;
      framebuffer_ci.pAttachments = &m_swapchain_image_views[i];
      framebuffer_ci.width = swapchain_extent.width;
      framebuffer_ci.height = swapchain_extent.height;
      framebuffer_ci.layers = 1;
      M_vk_check(vkCreateFramebuffer(m_device, &framebuffer_ci, NULL, &m_framebuffers[i]));
    }
  }

  VkVertexInputBindingDescription vertex_input_binding_descs[2] = {};
  VkVertexInputAttributeDescription vertex_input_attr_descs[2] = {};
  {
    Obj_loader_t obj;
    Path_t full_obj_path = g_exe_dir.join(M_txt("wolf.obj"));
    obj.init(&temp_vk_allocator, full_obj_path.m_path);
    M_scope_exit(obj.destroy());
    int vertices_size = obj.m_vertices.len() * sizeof(obj.m_vertices[0]);
    int normals_size = obj.m_normals.len() * sizeof(obj.m_normals[0]);
    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.size = 128*1024*1024;
    buffer_ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    M_vk_check(vkCreateBuffer(m_device, &buffer_ci, NULL, &m_vertex_buffer));

    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(m_device, m_vertex_buffer, &mem_reqs);

    VkDeviceMemory m_vertex_buffer_mem;
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = get_mem_type_idx(m_mem_props, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    M_vk_check(vkAllocateMemory(m_device, &alloc_info, NULL, &m_vertex_buffer_mem));
    vkBindBufferMemory(m_device, m_vertex_buffer, m_vertex_buffer_mem, 0);
    void* data;
    vkMapMemory(m_device, m_vertex_buffer_mem, 0, buffer_ci.size, 0, &data);
    memcpy(data, obj.m_vertices.m_p, vertices_size);
    memcpy((void*)((U8*)data + vertices_size), obj.m_normals.m_p, normals_size);
    vkUnmapMemory(m_device, m_vertex_buffer_mem);

    vertex_input_binding_descs[0].binding = 0;
    vertex_input_binding_descs[0].stride = sizeof(obj.m_vertices[0]);
    vertex_input_binding_descs[0].binding = 0;
    vertex_input_attr_descs[0].binding = 0;
    vertex_input_attr_descs[0].location = 0;
    vertex_input_attr_descs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;

    vertex_input_binding_descs[1].binding = 0;
    vertex_input_binding_descs[1].stride = sizeof(obj.m_normals[0]);
    vertex_input_binding_descs[1].binding = 0;
    vertex_input_attr_descs[1].binding = 0;
    vertex_input_attr_descs[1].location = 1;
    vertex_input_attr_descs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  }

  {
    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.size = 16*1024*1024;
    buffer_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    M_vk_check(vkCreateBuffer(m_device, &buffer_ci, NULL, &m_uniform_buffer));

    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(m_device, m_uniform_buffer, &mem_reqs);

    VkDeviceMemory m_uniform_buffer_mem;
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = get_mem_type_idx(m_mem_props, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    M_vk_check(vkAllocateMemory(m_device, &alloc_info, NULL, &m_uniform_buffer_mem));
    vkBindBufferMemory(m_device, m_uniform_buffer, m_uniform_buffer_mem, 0);
    void* data;
    vkMapMemory(m_device, m_uniform_buffer_mem, 0, buffer_ci.size, 0, &data);
    // memcpy(data, obj.m_vertices.m_p, obj.m_vertices.len() * sizeof(obj.m_vertices[0]));
    vkUnmapMemory(m_device, m_uniform_buffer_mem);
  }

  {
    VkShaderModule vertex_shader = load_shader_(m_device, g_exe_dir.join(M_txt("shader_vs.spv")));
    VkShaderModule fragment_shader = load_shader_(m_device, g_exe_dir.join(M_txt("shader_ps.spv")));

    VkPipelineShaderStageCreateInfo shader_stage_cis[2] = {};
    shader_stage_cis[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_cis[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_cis[0].module = vertex_shader;
    shader_stage_cis[0].pName = "VSMain";

    shader_stage_cis[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_cis[1].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_cis[1].module = fragment_shader;
    shader_stage_cis[1].pName = "PSMain";

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
    vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_ci.vertexBindingDescriptionCount = 2;
    vertex_input_ci.pVertexBindingDescriptions = vertex_input_binding_descs;
    vertex_input_ci.vertexAttributeDescriptionCount = 2;
    vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attr_descs;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
    input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_ci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent.width;
    viewport.height = (float)swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_extent;

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

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo m_descriptor_set_layout_ci = {};
    m_descriptor_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    m_descriptor_set_layout_ci.bindingCount = 1;
    m_descriptor_set_layout_ci.pBindings = &layout_binding;
    M_vk_check(vkCreateDescriptorSetLayout(m_device, &m_descriptor_set_layout_ci, NULL, &m_descriptor_set_layout));

    VkPipelineLayoutCreateInfo m_pipeline_layout_ci = {};
    m_pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    m_pipeline_layout_ci.setLayoutCount = 1;
    m_pipeline_layout_ci.pSetLayouts = &m_descriptor_set_layout;

    M_vk_check(vkCreatePipelineLayout(m_device, &m_pipeline_layout_ci, NULL, &m_pipeline_layout));

    VkGraphicsPipelineCreateInfo m_graphics_pipeline_ci = {};
    m_graphics_pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    m_graphics_pipeline_ci.stageCount = 2;
    m_graphics_pipeline_ci.pStages = shader_stage_cis;
    m_graphics_pipeline_ci.pVertexInputState = &vertex_input_ci;
    m_graphics_pipeline_ci.pInputAssemblyState = &input_assembly_ci;
    m_graphics_pipeline_ci.pViewportState = &viewport_ci;
    m_graphics_pipeline_ci.pRasterizationState = &rasterization_ci;
    m_graphics_pipeline_ci.pMultisampleState = &multisample_ci;
    m_graphics_pipeline_ci.pColorBlendState = &color_blend_state_ci;
    m_graphics_pipeline_ci.layout = m_pipeline_layout;
    m_graphics_pipeline_ci.renderPass = m_render_pass;
    m_graphics_pipeline_ci.subpass = 0;
    m_graphics_pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    m_graphics_pipeline_ci.basePipelineIndex = -1;

    M_vk_check(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &m_graphics_pipeline_ci, NULL, &m_graphics_pipeline));
  }

  {
    VkDescriptorPoolSize pool_size;
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &pool_size;
    createInfo.maxSets = 1;
    M_vk_check(vkCreateDescriptorPool(m_device, &createInfo, NULL, &m_descriptor_pool));
  }

  {
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = m_descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &m_descriptor_set_layout;

    M_vk_check(vkAllocateDescriptorSets(m_device, &descriptor_set_alloc_info, &m_descriptor_set));

    // Update descriptor set with uniform binding
    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = m_uniform_buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = 128;

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = m_descriptor_set;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.dstBinding = 0;

    vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, NULL);
  }

  {
    m_graphics_cmd_buffers.init(&m_vk_allocator);
    m_graphics_cmd_buffers.resize(m_swapchain_images.len());

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = m_cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = (uint32_t) m_swapchain_images.len();

    M_vk_check(vkAllocateCommandBuffers(m_device, &cmd_buffer_alloc_info, m_graphics_cmd_buffers.m_p));

    // Prepare data for recording command buffers
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = 1;

    VkClearValue clear_color = {
      { 0.1f, 0.1f, 0.1f, 1.0f } // R, G, B, A
    };

    // Record command buffer for each swap image
    for (size_t i = 0; i < m_swapchain_images.len(); i++) {
      vkBeginCommandBuffer(m_graphics_cmd_buffers[i], &cmd_buffer_begin_info);

      // If present queue family and graphics queue family are different, then a barrier is necessary
      // The barrier is also needed initially to transition the image to the present layout
      VkImageMemoryBarrier present_to_draw_barrier = {};
      present_to_draw_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      present_to_draw_barrier.srcAccessMask = 0;
      present_to_draw_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      present_to_draw_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      present_to_draw_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      if (m_present_q_idx != m_graphics_q_idx) {
        present_to_draw_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        present_to_draw_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      } else {
        present_to_draw_barrier.srcQueueFamilyIndex = m_present_q_idx;
        present_to_draw_barrier.dstQueueFamilyIndex = m_graphics_q_idx;
      }

      present_to_draw_barrier.image = m_swapchain_images[i];
      present_to_draw_barrier.subresourceRange = image_subresource_range;

      vkCmdPipelineBarrier(m_graphics_cmd_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &present_to_draw_barrier);

      VkRenderPassBeginInfo m_render_pass_begin_info = {};
      m_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      m_render_pass_begin_info.renderPass = m_render_pass;
      m_render_pass_begin_info.framebuffer = m_framebuffers[i];
      m_render_pass_begin_info.renderArea.offset = { 0, 0 };
      m_render_pass_begin_info.renderArea.extent = swapchain_extent;
      m_render_pass_begin_info.clearValueCount = 1;
      m_render_pass_begin_info.pClearValues = &clear_color;

      vkCmdBeginRenderPass(m_graphics_cmd_buffers[i], &m_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindDescriptorSets(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, NULL);

      vkCmdBindPipeline(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(m_graphics_cmd_buffers[i], 0, 1, &m_vertex_buffer, &offset);

      // vkCmdBindIndexBuffer(m_graphics_cmd_buffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // vkCmdDrawIndexed(m_graphics_cmd_buffers[i], 3, 1, 0, 0, 0);

      vkCmdEndRenderPass(m_graphics_cmd_buffers[i]);

      // If present and graphics queue families differ, then another barrier is required
      if (m_present_q_idx != m_graphics_q_idx) {
        VkImageMemoryBarrier draw_to_present_barrier = {};
        draw_to_present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        draw_to_present_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        draw_to_present_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        draw_to_present_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        draw_to_present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        draw_to_present_barrier.srcQueueFamilyIndex = m_graphics_q_idx;
        draw_to_present_barrier.dstQueueFamilyIndex = m_present_q_idx;
        draw_to_present_barrier.image = m_swapchain_images[i];
        draw_to_present_barrier.subresourceRange = image_subresource_range;

        vkCmdPipelineBarrier(m_graphics_cmd_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &draw_to_present_barrier);
      }

      M_vk_check(vkEndCommandBuffer(m_graphics_cmd_buffers[i]));
    }

    // No longer needed
    vkDestroyPipelineLayout(m_device, m_pipeline_layout, NULL);
  }
  return true;
}

void Vk_window_t::destroy() {
  m_vk_allocator.destroy();
}

void Vk_window_t::loop() {
}

int main() {
  core_init(M_txt("vulkan_sample.log"));
  Vk_window_t w(M_txt("vulkan_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}
