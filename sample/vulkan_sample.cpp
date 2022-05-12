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
#include "core/math/float.inl"
#include "core/math/mat4.inl"
#include "core/math/transform.inl"
#include "core/os.h"
#include "core/path.h"
#include "core/path_utils.h"
#include "core/utils.h"
#include "core/window/window.h"
#include "sample/cam.h"
#include "sample/vulkan_loader.h"

#define M_vk_check(condition) { \
  VkResult vk_result = condition; \
  M_check(vk_result == VK_SUCCESS); \
}

#define M_vk_check_return_false(condition) { \
  VkResult vk_result = condition; \
  M_check_return_val(vk_result == VK_SUCCESS, false); \
}

struct Per_obj_cb_t_ {
  M4_t world;
};

struct Shadow_shared_cb_t_ {
  M4_t light_view;
  M4_t light_proj;
};

struct Final_shared_cb_t_ {
  M4_t view;
  M4_t proj;
  M4_t light_view;
  M4_t light_proj;
  V4_t eye_pos;
  V4_t obj_color;
  V4_t light_pos;
  V4_t light_color;
};

struct Vk_buffer_t_ {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void* cpu_p = NULL;
  Sip size = 0;
  Sip offset_for_subbuffer = 0;
};

struct Vk_subbuffer_t_ {
  VkDescriptorBufferInfo bi;
  U8* cpu_p = NULL;
};

static VkBool32 debug_cb(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, U64 src_obj, Sz location, S32 msg_code, const char* layer_prefix, const char* msg, void* user_data) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    M_logf("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    M_logw("layer: %s\ncode: %d\nmessage: %s", layer_prefix, msg_code, msg);
	}
	return VK_FALSE;
}

class Vk_window_t : public Window_t {
public:
  Vk_window_t(const Os_char* title, int w, int h) : Window_t(title, w, h), m_vk_allocator("vk_allocator") {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(E_mouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;

  Linear_allocator_t<> m_vk_allocator;
  VkInstance m_instance;
  VkSurfaceKHR m_surface;
  VkPhysicalDevice m_chosen_device = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties m_device_props;
  Sz m_min_alignment = 0;
  VkPhysicalDeviceMemoryProperties m_mem_props = {};
  int m_graphics_q_idx = -1;
  int m_present_q_idx = -1;
  VkDevice m_device;
  VkQueue m_graphics_q;
  VkQueue m_present_q;
  VkCommandPool m_cmd_pool;
  VkSwapchainKHR m_swapchain;
  Dynamic_array_t<VkImage> m_swapchain_images;
  VkRenderPass m_final_render_pass;
  VkRenderPass m_shadow_render_pass;
  Dynamic_array_t<VkImageView> m_swapchain_image_views;
  Dynamic_array_t<VkFramebuffer> m_framebuffers;
  VkFramebuffer m_shadow_fb;
  Vk_buffer_t_ m_vertex_buffer;
  Vk_subbuffer_t_ m_vertices_subbuffer;
  Vk_subbuffer_t_ m_normals_subbuffer;
  Vk_buffer_t_ m_uniform_buffer;
  VkDescriptorSetLayout m_shadow_shared_ds_layout;
  VkDescriptorSetLayout m_per_obj_ds_layout;
  VkPipelineLayout m_shadow_pipeline_layout;
  VkPipeline m_shadow_gfx_pipeline;
  VkDescriptorPool m_uniform_descriptor_pool;
  Dynamic_array_t<VkCommandBuffer> m_graphics_cmd_buffers;
  VkSemaphore m_image_available_semaphore;
  VkSemaphore m_rendering_finished_semaphore;

  VkDescriptorSetLayout m_final_shared_ds_layout;
  VkDescriptorSet m_final_shared_ds;
  Vk_subbuffer_t_ m_final_shared_subbuffer;
  VkPipelineLayout m_final_pipeline_layout;
  VkPipeline m_final_gfx_pipeline;
  VkSampler m_shadow_sampler;

  U32 m_obj_vertices_counts[10] = {};
  Vk_subbuffer_t_ m_per_obj_cb_subbuffers[10];
  Per_obj_cb_t_ m_per_obj_cbs[10];
  VkDescriptorSet m_per_obj_cb_dss[10];
  Vk_subbuffer_t_ m_final_shared_cb_subbuffer;

  Shadow_shared_cb_t_ m_shadow_shared_cb;
  Vk_subbuffer_t_ m_shadow_shared_subbuffer;
  VkDescriptorSet m_shadow_shared_ds;
  Final_shared_cb_t_ m_final_shared_cb;
  int m_obj_count;

  VkShaderModule m_shadow_vs;
  VkShaderModule m_shadow_ps;
  VkShaderModule m_final_vs;
  VkShaderModule m_final_ps;
  VkImage m_depth_stencil_image;
  VkDeviceMemory m_depth_stencil_mem;
  VkImageView m_depth_stencil_image_view;

  VkImage m_shadow_depth_stencil_image;
  VkDeviceMemory m_shadow_depth_stencil_mem;
  VkImageView m_shadow_depth_stencil_image_view;

  Cam_t m_cam;

private:
  VkShaderModule load_shader_(const Path_t& path);
  int get_mem_type_idx_(U32 mem_type_bits, VkFlags mem_flags);
  bool create_buffer_(Vk_buffer_t_* buffer, Sz size, VkBufferUsageFlagBits usage_flags, VkMemoryPropertyFlagBits mem_prop_flags);
  Vk_subbuffer_t_ allocate_subbuffer_(Vk_buffer_t_* buffer, Sip size, int alignment);
  VkDescriptorPool create_descriptor_pool_(VkDescriptorType type, int descriptor_count);
  VkDescriptorSetLayout create_descriptor_set_layout_(const VkDescriptorSetLayoutBinding* bindings, int count);
  VkDescriptorSet allocate_descriptor_sets_(VkDescriptorPool pool, VkDescriptorSetLayout* layouts);
  void update_uniform_descriptor_sets_(VkDescriptorSet descriptor_set, int binding, int buffer_count, VkDescriptorBufferInfo* buffer_info);
  VkPipeline create_graphics_pipeline_(VkShaderModule vs,
                                       VkShaderModule ps,
                                       VkPipelineLayout pipeline_layout,
                                       const VkVertexInputBindingDescription* bindings,
                                       int binding_count,
                                       const VkVertexInputAttributeDescription* attributes,
                                       int attribute_count,
                                       VkRenderPass render_pass);
  VkImageView create_image_view_(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
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
    vkGetPhysicalDeviceProperties(m_chosen_device, &m_device_props);
    m_min_alignment = m_device_props.limits.minMemoryMapAlignment;
    M_logi("Chosen GPU: %s", m_device_props.deviceName);
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

    attachment_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_ref = {};
    depth_ref.attachment = 0;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.pDepthStencilAttachment = &depth_ref;

    // Create the render pass
    VkSubpassDependency subpass_deps[2] = {};
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

    VkRenderPassCreateInfo shadow_render_pass_ci = {};
    shadow_render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    shadow_render_pass_ci.attachmentCount = 1;
    shadow_render_pass_ci.pAttachments = &attachment_desc;
    shadow_render_pass_ci.subpassCount = 1;
    shadow_render_pass_ci.pSubpasses = &subpass_desc;
    shadow_render_pass_ci.dependencyCount = 2;
    shadow_render_pass_ci.pDependencies = subpass_deps;

    M_vk_check(vkCreateRenderPass(m_device, &shadow_render_pass_ci, NULL, &m_shadow_render_pass));
  }

  {
    VkAttachmentDescription attachment_descs[2] = {};
    attachment_descs[0].format = swapchain_format;
    attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descs[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
    attachment_descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref = {};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref = {};
    depth_ref.attachment = 1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_ref;
    subpass_desc.pDepthStencilAttachment = &depth_ref;

    // Create the render pass
    VkRenderPassCreateInfo final_render_pass_ci = {};
    final_render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    final_render_pass_ci.attachmentCount = static_array_size(attachment_descs);
    final_render_pass_ci.pAttachments = attachment_descs;
    final_render_pass_ci.subpassCount = 1;
    final_render_pass_ci.pSubpasses = &subpass_desc;

    M_vk_check(vkCreateRenderPass(m_device, &final_render_pass_ci, NULL, &m_final_render_pass));
  }

  m_swapchain_image_views.init(&m_vk_allocator);
  m_swapchain_image_views.resize(m_swapchain_images.len());
  {
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      m_swapchain_image_views[i] = create_image_view_(m_swapchain_images[i], swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
  }

  {
    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_ci.extent = {(U32)m_width, (U32)m_height, 1};
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    M_vk_check(vkCreateImage(m_device, &image_ci, NULL, &m_depth_stencil_image));
    image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    M_vk_check(vkCreateImage(m_device, &image_ci, NULL, &m_shadow_depth_stencil_image));

    {
      VkMemoryRequirements image_mem_reqs;
      vkGetImageMemoryRequirements(m_device, m_depth_stencil_image, &image_mem_reqs);

      VkMemoryAllocateInfo alloc_info{};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = image_mem_reqs.size;
      alloc_info.memoryTypeIndex = get_mem_type_idx_(image_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      M_vk_check(vkAllocateMemory(m_device, &alloc_info, nullptr, &m_depth_stencil_mem));
      vkBindImageMemory(m_device, m_depth_stencil_image, m_depth_stencil_mem, 0);

      m_depth_stencil_image_view = create_image_view_(m_depth_stencil_image, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    {
      VkMemoryRequirements image_mem_reqs;
      vkGetImageMemoryRequirements(m_device, m_shadow_depth_stencil_image, &image_mem_reqs);

      VkMemoryAllocateInfo alloc_info{};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = image_mem_reqs.size;
      alloc_info.memoryTypeIndex = get_mem_type_idx_(image_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      M_vk_check(vkAllocateMemory(m_device, &alloc_info, nullptr, &m_shadow_depth_stencil_mem));
      vkBindImageMemory(m_device, m_shadow_depth_stencil_image, m_shadow_depth_stencil_mem, 0);

      m_shadow_depth_stencil_image_view = create_image_view_(m_shadow_depth_stencil_image, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
  }

  m_framebuffers.init(&m_vk_allocator);
  m_framebuffers.resize(m_swapchain_images.len());
  {
    for (int i = 0; i < m_swapchain_images.len(); ++i) {
      VkImageView attachments[] = { m_swapchain_image_views[i], m_depth_stencil_image_view };
      VkFramebufferCreateInfo framebuffer_ci = {};
      framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_ci.renderPass = m_final_render_pass;
      framebuffer_ci.attachmentCount = static_array_size(attachments);
      framebuffer_ci.pAttachments = attachments;
      framebuffer_ci.width = swapchain_extent.width;
      framebuffer_ci.height = swapchain_extent.height;
      framebuffer_ci.layers = 1;
      M_vk_check(vkCreateFramebuffer(m_device, &framebuffer_ci, NULL, &m_framebuffers[i]));
    }
  }

  {
      VkFramebufferCreateInfo framebuffer_ci = {};
      framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_ci.renderPass = m_shadow_render_pass;
      framebuffer_ci.attachmentCount = 1;
      framebuffer_ci.pAttachments = &m_shadow_depth_stencil_image_view;
      framebuffer_ci.width = swapchain_extent.width;
      framebuffer_ci.height = swapchain_extent.height;
      framebuffer_ci.layers = 1;
      M_vk_check(vkCreateFramebuffer(m_device, &framebuffer_ci, NULL, &m_shadow_fb));
  }

  {
    M_check_return_false(create_buffer_(&m_uniform_buffer, 16*1024*1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
  }

  {
    m_uniform_descriptor_pool = create_descriptor_pool_(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10);
  }

  {
    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    m_per_obj_ds_layout = create_descriptor_set_layout_(&layout_binding, 1);

    m_shadow_shared_ds_layout = create_descriptor_set_layout_(&layout_binding, 1);
  }

  {
    VkDescriptorSetLayoutBinding layout_bindings[3] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[2].binding = 2;
    layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layout_bindings[2].descriptorCount = 1;
    layout_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_final_shared_ds_layout = create_descriptor_set_layout_(layout_bindings, 3);
  }

  // The light is static for now.
  Cam_t light_cam;
  light_cam.init({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, this);

  // TODO: ortho?
  M4_t perspective_m4 = perspective(degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);

  {

    m_shadow_shared_cb.light_view = light_cam.m_view_mat;
    m_shadow_shared_cb.light_proj = perspective_m4;
    m_shadow_shared_ds = allocate_descriptor_sets_(m_uniform_descriptor_pool, &m_shadow_shared_ds_layout);
    m_shadow_shared_subbuffer = allocate_subbuffer_(&m_uniform_buffer, sizeof(Shadow_shared_cb_t_), 256);
    memcpy(m_shadow_shared_subbuffer.cpu_p, &m_shadow_shared_cb, sizeof(Shadow_shared_cb_t_));
    update_uniform_descriptor_sets_(m_shadow_shared_ds, 0, 1, &m_shadow_shared_subbuffer.bi);
  }

  {
    m_cam.init({5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
    m_final_shared_cb.eye_pos = V3o_v4(m_cam.m_eye, 1.0f);
    m_final_shared_cb.obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
    m_final_shared_cb.light_pos = {10.0f, 10.0f, 10.0f, 1.0f};
    m_final_shared_cb.light_color = {1.0f, 1.0f, 1.0f, 1.0f};

    m_final_shared_cb.proj = perspective_m4;
    m_final_shared_cb.light_view = light_cam.m_view_mat;
    m_final_shared_cb.light_proj = perspective_m4;

    m_final_shared_ds = allocate_descriptor_sets_(m_uniform_descriptor_pool, &m_final_shared_ds_layout);
    m_final_shared_subbuffer = allocate_subbuffer_(&m_uniform_buffer, sizeof(Shadow_shared_cb_t_), 256);
    memcpy(m_final_shared_subbuffer.cpu_p, &m_final_shared_cb, sizeof(m_final_shared_cb));
    update_uniform_descriptor_sets_(m_final_shared_ds, 0, 1, &m_final_shared_subbuffer.bi);
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
    M_vk_check(vkCreateSampler(m_device, &sampler_ci, NULL, &m_shadow_sampler));

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = m_shadow_sampler;
    descriptor_image_info.imageView = m_shadow_depth_stencil_image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write_descriptor_sets[2] = {};
    write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[0].dstSet = m_final_shared_ds;
    write_descriptor_sets[0].descriptorCount = 1;
    write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write_descriptor_sets[0].dstBinding = 1;
    write_descriptor_sets[0].pImageInfo = &descriptor_image_info;
    write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[1].dstSet = m_final_shared_ds;
    write_descriptor_sets[1].descriptorCount = 1;
    write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write_descriptor_sets[1].dstBinding = 2;
    write_descriptor_sets[1].pImageInfo = &descriptor_image_info;

    vkUpdateDescriptorSets(m_device, 2, write_descriptor_sets, 0, NULL);
  }

  VkVertexInputBindingDescription vertex_input_binding_descs[2] = {};
  VkVertexInputAttributeDescription vertex_input_attr_descs[2] = {};
  {
    const Os_char* obj_paths[] = {
        M_txt("assets/wolf.obj"),
        M_txt("assets/plane.obj"),
    };
    Sip vertices_offset = 0;
    Sip normals_offset = 0;
    m_obj_count = static_array_size(obj_paths);

    M_check_return_false(create_buffer_(&m_vertex_buffer, 128*1024*1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
    m_vertices_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 16 * 1024 * 1024, m_device_props.limits.minMemoryMapAlignment);
    m_normals_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 16 * 1024 * 1024, m_device_props.limits.minMemoryMapAlignment);
    {
      for (int i = 0; i < m_obj_count; ++i) {
        m_per_obj_cb_subbuffers[i] = allocate_subbuffer_(&m_uniform_buffer, sizeof(Per_obj_cb_t_), 256);
        m_per_obj_cb_dss[i] = allocate_descriptor_sets_(m_uniform_descriptor_pool, &m_per_obj_ds_layout);
        update_uniform_descriptor_sets_(m_per_obj_cb_dss[i], 0, 1, &m_per_obj_cb_subbuffers[i].bi);
        m_per_obj_cbs[i].world = m4_identity();
        m_per_obj_cbs[i].world.a[1][1] = -1.0f;
        memcpy(m_per_obj_cb_subbuffers[i].cpu_p, &m_per_obj_cbs[i], sizeof(Per_obj_cb_t_));

        Obj_loader_t obj;
        Path_t full_obj_path = g_exe_dir.join(obj_paths[i]);
        obj.init(&temp_vk_allocator, full_obj_path.m_path);
        M_scope_exit(obj.destroy());
        m_obj_vertices_counts[i] = obj.m_vertices.len();
        int vertices_size = m_obj_vertices_counts[i] * sizeof(obj.m_vertices[0]);
        int normals_size = m_obj_vertices_counts[i] * sizeof(obj.m_normals[0]);
        M_check_return_false(vertices_offset + vertices_size <= m_vertices_subbuffer.bi.range);
        M_check_return_false(normals_offset + normals_size <= m_normals_subbuffer.bi.range);
        memcpy(m_vertices_subbuffer.cpu_p + vertices_offset, &obj.m_vertices[0], vertices_size);
        memcpy(m_normals_subbuffer.cpu_p + normals_offset, &obj.m_normals[0], normals_size);
        vertices_offset += vertices_size;
        normals_offset += normals_size;
      }
    }

    vertex_input_binding_descs[0].binding = 0;
    vertex_input_binding_descs[0].stride = sizeof(decltype(Obj_loader_t::m_vertices)::T_value);
    vertex_input_binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_attr_descs[0].binding = 0;
    vertex_input_attr_descs[0].location = 0;
    vertex_input_attr_descs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attr_descs[0].offset = 0;

    vertex_input_binding_descs[1].binding = 1;
    vertex_input_binding_descs[1].stride = sizeof(decltype(Obj_loader_t::m_normals)::T_value);
    vertex_input_binding_descs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_attr_descs[1].binding = 1;
    vertex_input_attr_descs[1].location = 1;
    vertex_input_attr_descs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attr_descs[1].offset = 0;
  }

  {

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 2;
    {
      VkDescriptorSetLayout layouts[] = { m_per_obj_ds_layout, m_shadow_shared_ds_layout };
      pipeline_layout_ci.pSetLayouts = layouts;
      M_vk_check(vkCreatePipelineLayout(m_device, &pipeline_layout_ci, NULL, &m_shadow_pipeline_layout));
    }

    {
      VkDescriptorSetLayout layouts[] = { m_per_obj_ds_layout, m_final_shared_ds_layout };
      pipeline_layout_ci.pSetLayouts = layouts;
      M_vk_check(vkCreatePipelineLayout(m_device, &pipeline_layout_ci, NULL, &m_final_pipeline_layout));
    }

    m_shadow_vs = load_shader_(g_exe_dir.join(M_txt("shadow_vs.spv")));
    m_final_vs = load_shader_(g_exe_dir.join(M_txt("shader_vs.spv")));
    m_final_ps = load_shader_(g_exe_dir.join(M_txt("shader_ps.spv")));

    m_shadow_gfx_pipeline = create_graphics_pipeline_(m_shadow_vs, VK_NULL_HANDLE, m_shadow_pipeline_layout, vertex_input_binding_descs, 1, vertex_input_attr_descs, 1, m_shadow_render_pass);
    m_final_gfx_pipeline = create_graphics_pipeline_(m_final_vs, m_final_ps, m_final_pipeline_layout, vertex_input_binding_descs, 2, vertex_input_attr_descs, 2, m_final_render_pass);
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

    VkClearValue clear_values[2] = {};
    clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clear_values[1].depthStencil.depth = 1.0f;

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

      VkRenderPassBeginInfo render_pass_begin_info = {};
      render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_begin_info.renderPass = m_shadow_render_pass;
      render_pass_begin_info.framebuffer = m_shadow_fb;
      render_pass_begin_info.renderArea.offset = { 0, 0 };
      render_pass_begin_info.renderArea.extent = swapchain_extent;
      render_pass_begin_info.clearValueCount = static_array_size(clear_values);
      render_pass_begin_info.pClearValues = clear_values;

      vkCmdBeginRenderPass(m_graphics_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadow_gfx_pipeline);
      vkCmdBindDescriptorSets(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadow_pipeline_layout, 1, 1, &m_shadow_shared_ds, 0, NULL);
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(m_graphics_cmd_buffers[i], 0, 1, &m_vertex_buffer.buffer, &offset);

      for (int obj_i = 0; obj_i < m_obj_count; ++obj_i) {
        vkCmdBindDescriptorSets(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadow_pipeline_layout, 0, 1, &m_per_obj_cb_dss[obj_i], 0, NULL);
        if (obj_i == 0) {
          vkCmdDraw(m_graphics_cmd_buffers[i], m_obj_vertices_counts[obj_i], 1, 0, 0);
        } else {
          vkCmdDraw(m_graphics_cmd_buffers[i], m_obj_vertices_counts[obj_i], 1, m_obj_vertices_counts[obj_i - 1], 0);
        }
      }
      vkCmdEndRenderPass(m_graphics_cmd_buffers[i]);

      render_pass_begin_info.renderPass = m_final_render_pass;
      render_pass_begin_info.framebuffer = m_framebuffers[i];

      vkCmdBeginRenderPass(m_graphics_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_final_gfx_pipeline);
      vkCmdBindDescriptorSets(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_final_pipeline_layout, 1, 1, &m_final_shared_ds, 0, NULL);
      VkDeviceSize offset0 = m_vertices_subbuffer.bi.offset;
      vkCmdBindVertexBuffers(m_graphics_cmd_buffers[i], 0, 1, &m_vertex_buffer.buffer, &offset0);
      VkDeviceSize offset1 = m_normals_subbuffer.bi.offset;
      vkCmdBindVertexBuffers(m_graphics_cmd_buffers[i], 1, 1, &m_vertex_buffer.buffer, &offset1);
      for (int obj_i = 0; obj_i < m_obj_count; ++obj_i) {
        vkCmdBindDescriptorSets(m_graphics_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_final_pipeline_layout, 0, 1, &m_per_obj_cb_dss[obj_i], 0, NULL);
        if (obj_i == 0) {
          vkCmdDraw(m_graphics_cmd_buffers[i], m_obj_vertices_counts[obj_i], 1, 0, 0);
        } else {
          vkCmdDraw(m_graphics_cmd_buffers[i], m_obj_vertices_counts[obj_i], 1, m_obj_vertices_counts[obj_i - 1], 0);
        }
      }
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
    vkDestroyPipelineLayout(m_device, m_shadow_pipeline_layout, NULL);
  }
  {
    VkSemaphoreCreateInfo semaphore_ci = {};
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    M_vk_check(vkCreateSemaphore(m_device, &semaphore_ci, NULL, &m_image_available_semaphore));
    M_vk_check(vkCreateSemaphore(m_device, &semaphore_ci, NULL, &m_rendering_finished_semaphore));
  }
  return true;
}

void Vk_window_t::destroy() {
  m_vk_allocator.destroy();
}

void Vk_window_t::loop() {
  m_cam.update();
  m_final_shared_cb.view = m_cam.m_view_mat;
  // memcpy(m_final_shared_cb_subbuffer.cpu_p, &m_final_shared_cb, sizeof(m_final_shared_cb));

  U32 image_idx;
  M_vk_check(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_image_available_semaphore, VK_NULL_HANDLE, &image_idx));
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &m_image_available_semaphore;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &m_rendering_finished_semaphore;
  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &m_graphics_cmd_buffers[image_idx];
  M_vk_check(vkQueueSubmit(m_graphics_q, 1, &submit_info, VK_NULL_HANDLE));

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &m_rendering_finished_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &m_swapchain;
  present_info.pImageIndices = &image_idx;
  M_vk_check(vkQueuePresentKHR(m_present_q, &present_info));
}

void Vk_window_t::on_mouse_event(E_mouse mouse, int x, int y, bool is_down) {
  m_cam.mouse_event(mouse, x, y, is_down);
}

void Vk_window_t::on_mouse_move(int x, int y) {
  m_cam.mouse_move(x, y);
}

VkShaderModule Vk_window_t::load_shader_(const Path_t& path) {
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

int Vk_window_t::get_mem_type_idx_(U32 mem_type_bits, VkFlags mem_flags) {
  for (int i = 0; i < m_mem_props.memoryTypeCount; ++i) {
    if ((mem_type_bits & 1 << i) && (m_mem_props.memoryTypes[i].propertyFlags & mem_flags)) {
      return i;
    }
  }
  M_logf("Can't find a suitable memory type");
  return -1;
}

bool Vk_window_t::create_buffer_(Vk_buffer_t_* buffer, Sz size, VkBufferUsageFlagBits usage_flags, VkMemoryPropertyFlagBits mem_prop_flags) {
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

Vk_subbuffer_t_ Vk_window_t::allocate_subbuffer_(Vk_buffer_t_* buffer, Sip size, int alignment) {
  Vk_subbuffer_t_ subbuffer = {};
  Sip aligned_offset = (buffer->offset_for_subbuffer + alignment - 1) & ~(alignment - 1);
  M_check_log_return_val(aligned_offset + size <= buffer->size, subbuffer, "Out of memory");
  subbuffer.cpu_p = (U8*)buffer->cpu_p + aligned_offset;
  subbuffer.bi.buffer = buffer->buffer;
  subbuffer.bi.offset = aligned_offset;
  subbuffer.bi.range = size;
  buffer->offset_for_subbuffer = aligned_offset + size;
  return subbuffer;
}

VkDescriptorPool Vk_window_t::create_descriptor_pool_(VkDescriptorType type, int descriptor_count) {
  VkDescriptorPoolSize pool_size;
  pool_size.type = type;
  pool_size.descriptorCount = descriptor_count;

  VkDescriptorPoolCreateInfo descriptor_pool_ci = {};
  descriptor_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_ci.poolSizeCount = 1;
  descriptor_pool_ci.pPoolSizes = &pool_size;
  descriptor_pool_ci.maxSets = descriptor_count;
  VkDescriptorPool pool;
  M_vk_check(vkCreateDescriptorPool(m_device, &descriptor_pool_ci, NULL, &pool));
  return pool;
}

VkDescriptorSetLayout Vk_window_t::create_descriptor_set_layout_(const VkDescriptorSetLayoutBinding* bindings, int count) {
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci = {};
  descriptor_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_ci.bindingCount = count;
  descriptor_set_layout_ci.pBindings = bindings;
  M_vk_check(vkCreateDescriptorSetLayout(m_device, &descriptor_set_layout_ci, NULL, &descriptor_set_layout));
  return descriptor_set_layout;
}

VkDescriptorSet Vk_window_t::allocate_descriptor_sets_(VkDescriptorPool pool, VkDescriptorSetLayout* layouts) {
  VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
  descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_alloc_info.descriptorPool = pool;
  descriptor_set_alloc_info.descriptorSetCount = 1;
  descriptor_set_alloc_info.pSetLayouts = layouts;

  VkDescriptorSet descriptor_set;
  M_vk_check(vkAllocateDescriptorSets(m_device, &descriptor_set_alloc_info, &descriptor_set));
  return descriptor_set;
}

void Vk_window_t::update_uniform_descriptor_sets_(VkDescriptorSet descriptor_set, int binding, int buffer_count, VkDescriptorBufferInfo* buffer_info) {
  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = descriptor_set;
  write_descriptor_set.descriptorCount = buffer_count;
  write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write_descriptor_set.pBufferInfo = buffer_info;
  write_descriptor_set.dstBinding = binding;

  vkUpdateDescriptorSets(m_device, 1, &write_descriptor_set, 0, NULL);
}

VkPipeline Vk_window_t::create_graphics_pipeline_(VkShaderModule vs,
                                                  VkShaderModule ps,
                                                  VkPipelineLayout pipeline_layout,
                                                  const VkVertexInputBindingDescription* bindings,
                                                  int binding_count,
                                                  const VkVertexInputAttributeDescription* attributes,
                                                  int attribute_count,
                                                  VkRenderPass render_pass) {
  VkPipelineShaderStageCreateInfo shader_stage_cis[2] = {};
  shader_stage_cis[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_cis[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stage_cis[0].module = vs;
  shader_stage_cis[0].pName = "VSMain";

  if (ps != VK_NULL_HANDLE) {
    shader_stage_cis[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_cis[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_cis[1].module = ps;
    shader_stage_cis[1].pName = "PSMain";
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_ci = {};
  vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_ci.vertexBindingDescriptionCount = binding_count;
  vertex_input_ci.pVertexBindingDescriptions = bindings;
  vertex_input_ci.vertexAttributeDescriptionCount = attribute_count;
  vertex_input_ci.pVertexAttributeDescriptions = attributes;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {};
  input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_ci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)m_width;
  viewport.height = (float)m_height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = { (U32)m_width, (U32)m_height };

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
  depth_stencil_state_ci.depthTestEnable = VK_TRUE;
  depth_stencil_state_ci.depthWriteEnable = VK_TRUE;
  depth_stencil_state_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_state_ci.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_state_ci.stencilTestEnable = VK_FALSE;

  VkGraphicsPipelineCreateInfo pipeline_ci = {};
  pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_ci.stageCount = ps == VK_NULL_HANDLE ? 1 : 2;
  pipeline_ci.pStages = shader_stage_cis;
  pipeline_ci.pVertexInputState = &vertex_input_ci;
  pipeline_ci.pInputAssemblyState = &input_assembly_ci;
  pipeline_ci.pViewportState = &viewport_ci;
  pipeline_ci.pRasterizationState = &rasterization_ci;
  pipeline_ci.pMultisampleState = &multisample_ci;
  pipeline_ci.pDepthStencilState = &depth_stencil_state_ci;
  pipeline_ci.pColorBlendState = &color_blend_state_ci;
  pipeline_ci.layout = pipeline_layout;
  pipeline_ci.renderPass = render_pass;
  pipeline_ci.subpass = 0;
  pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_ci.basePipelineIndex = -1;

  VkPipeline pipeline;
  M_vk_check(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &pipeline));
  return pipeline;
}

VkImageView Vk_window_t::create_image_view_(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo image_view_ci = {};
  image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_ci.image = image;
  image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_ci.format = format;
  image_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  image_view_ci.subresourceRange.aspectMask = aspect_flags;
  image_view_ci.subresourceRange.baseMipLevel = 0;
  image_view_ci.subresourceRange.levelCount = 1;
  image_view_ci.subresourceRange.baseArrayLayer = 0;
  image_view_ci.subresourceRange.layerCount = 1;
  VkImageView image_view;
  M_vk_check(vkCreateImageView(m_device, &image_view_ci, NULL, &image_view));
  return image_view;
}

int main() {
  core_init(M_txt("vulkan_sample.log"));
  Vk_window_t w(M_txt("vulkan_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}
