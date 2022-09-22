//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/gpu/gpu.h"
#include "core/gpu/vulkan/vulkan_loader.h"
#include "core/linear_allocator.h"
#include "core/window/window.h"

struct Vk_sub_buffer_t_;

struct Vk_buffer_t_ {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void* cpu_p = NULL;
  Sip size = 0;
  Sip offset_for_sub_buffer = 0;
};

class Vulkan_t : public Gpu_t {
public:
  Vulkan_t() : Gpu_t(), m_vk_allocator("vk_allocator"), m_swapchain_images(&m_vk_allocator), m_swapchain_image_views(&m_vk_allocator), m_graphics_cmd_buffers(&m_vk_allocator), m_fences(&m_vk_allocator) {}
  bool init(Window_t* w);
  void destroy() override;
  Texture_t* create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) override;
  Texture_t* create_texture_cube(Allocator_t* allocator, const Texture_create_info_t& ci) override;
  Resources_set_t* create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci) override;
  Pipeline_layout_t* create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci) override;
  Vertex_buffer_t* create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) override;
  Index_buffer_t* create_index_buffer(Allocator_t* allocator, const Index_buffer_create_info_t& ci) override;
  Render_target_t* create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) override;
  Render_pass_t* create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) override;
  Resource_t create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) override;
  Resource_t create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) override;
  Resource_t create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) override;
  void bind_resource_to_set(const Resource_t& resource, const Resources_set_t* set, int binding) override;
  Shader_t* compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci) override;
  Pipeline_state_object_t* create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci) override;
  void get_back_buffer() override;
  void cmd_begin() override;
  void cmd_begin_render_pass(Render_pass_t* render_pass) override;
  void cmd_end_render_pass(Render_pass_t* render_pass) override;
  void cmd_set_pipeline_state(Pipeline_state_object_t* pso) override;
  void cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding) override;
  void cmd_set_index_buffer(Index_buffer_t* ib) override;
  void cmd_set_resource(const Resource_t& resource, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) override;
  void cmd_draw(int vertex_count, int first_vertex) override;
  void cmd_draw_index(int index_count, int instance_count, int first_index, int vertex_offset, int first_instance) override;
  void cmd_end() override;

  Window_t* m_window = NULL;

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
  VkQueue m_transfer_q;
  VkCommandPool m_graphics_cmd_pool;
  VkCommandPool m_transfer_cmd_pool;
  VkSwapchainKHR m_swapchain;
  VkFormat m_swapchain_format;
  VkFormat m_depth_format;
  Dynamic_array_t<VkImage> m_swapchain_images;
  Dynamic_array_t<VkImageView> m_swapchain_image_views;
  VkDescriptorPool m_descriptors_pool;
  Vk_buffer_t_ m_uniform_buffer;
  Vk_buffer_t_ m_vertex_buffer;
  Vk_buffer_t_ m_upload_buffer;
  Dynamic_array_t<VkCommandBuffer> m_graphics_cmd_buffers;
  VkCommandBuffer m_transfer_cmd_buffer;
  Dynamic_array_t<VkFence> m_fences;
  U32 m_next_swapchain_image_idx;
  VkSemaphore m_image_available_semaphore;
  VkSemaphore m_rendering_finished_semaphore;
private:
  int get_mem_type_idx_(U32 mem_type_bits, VkFlags mem_flags);
  VkImageView create_image_view_(VkImage image, VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags);
  bool create_buffer_(Vk_buffer_t_* buffer, Sz size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlagBits mem_prop_flags);
  void allocate_sub_buffer_(Vk_sub_buffer_t_* sub_buffer, Vk_buffer_t_* buffer, Sip size, int alignment);
  VkCommandBuffer get_active_cmd_buffer_() const;
  void create_image_(VkImage* image, VkDeviceMemory* memory, U32 width, U32 height, VkFormat format, VkImageUsageFlags usage, VkImageCreateFlags flags);
};
