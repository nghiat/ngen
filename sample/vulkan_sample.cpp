//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/dynamic_lib.h"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/os.h"
#include "core/utils.h"
#include "core/window/window.h"

#define VK_NO_PROTOTYPES
#include "third_party/vulkan/Vulkan-Headers/include/vulkan/vulkan.h"

#define M_vk_check(condition) { \
  VkResult vk_result = condition; \
  M_check(vk_result == VK_SUCCESS); \
}

PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkDestroyDevice vkDestroyDevice;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueueWaitIdle vkQueueWaitIdle;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkMapMemory vkMapMemory;
PFN_vkUnmapMemory vkUnmapMemory;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
PFN_vkBindBufferMemory vkBindBufferMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkQueueBindSparse vkQueueBindSparse;
PFN_vkCreateFence vkCreateFence;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkResetFences vkResetFences;
PFN_vkGetFenceStatus vkGetFenceStatus;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkCreateEvent vkCreateEvent;
PFN_vkDestroyEvent vkDestroyEvent;
PFN_vkGetEventStatus vkGetEventStatus;
PFN_vkSetEvent vkSetEvent;
PFN_vkResetEvent vkResetEvent;
PFN_vkCreateQueryPool vkCreateQueryPool;
PFN_vkDestroyQueryPool vkDestroyQueryPool;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
PFN_vkCreateBuffer vkCreateBuffer;
PFN_vkDestroyBuffer vkDestroyBuffer;
PFN_vkCreateBufferView vkCreateBufferView;
PFN_vkDestroyBufferView vkDestroyBufferView;
PFN_vkCreateImage vkCreateImage;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkCreatePipelineCache vkCreatePipelineCache;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
PFN_vkMergePipelineCaches vkMergePipelineCaches;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateComputePipelines vkCreateComputePipelines;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkCreateSampler vkCreateSampler;
PFN_vkDestroySampler vkDestroySampler;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
PFN_vkResetDescriptorPool vkResetDescriptorPool;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkResetCommandPool vkResetCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
PFN_vkCmdDispatch vkCmdDispatch;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
PFN_vkCmdCopyImage vkCmdCopyImage;
PFN_vkCmdBlitImage vkCmdBlitImage;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
PFN_vkCmdFillBuffer vkCmdFillBuffer;
PFN_vkCmdClearColorImage vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
PFN_vkCmdClearAttachments vkCmdClearAttachments;
PFN_vkCmdResolveImage vkCmdResolveImage;
PFN_vkCmdSetEvent vkCmdSetEvent;
PFN_vkCmdResetEvent vkCmdResetEvent;
PFN_vkCmdWaitEvents vkCmdWaitEvents;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
PFN_vkCmdBeginQuery vkCmdBeginQuery;
PFN_vkCmdEndQuery vkCmdEndQuery;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
PFN_vkCmdPushConstants vkCmdPushConstants;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdNextSubpass vkCmdNextSubpass;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands;

#if M_os_is_win()
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif

PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

static VkBool32 debug_cb(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, U64 src_obj, Sz location, S32 msg_code, const char* layer_prefix, const char* msg, void* user_data) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    M_logf("layer: %s\ncode: %d\nmessage: msg", layer_prefix, msg_code, msg);
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    M_logw("layer: %s\ncode: %d\nmessage: msg", layer_prefix, msg_code, msg);
	}
	return VK_FALSE;
}

static void load_vulkan_() {
  Dynamic_lib_t vulkan_lib;
#if M_os_is_win()
  vulkan_lib.open("vulkan-1.dll");
#elif M_os_is_linux()
  vulkan_lib.open("libvulkan.so");
#endif

  vkCreateInstance = (PFN_vkCreateInstance)vulkan_lib.get_proc("vkCreateInstance");
  M_check(vkCreateInstance);
  vkDestroyInstance = (PFN_vkDestroyInstance)vulkan_lib.get_proc("vkDestroyInstance");
  M_check(vkDestroyInstance);
  vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vulkan_lib.get_proc("vkEnumeratePhysicalDevices");
  M_check(vkEnumeratePhysicalDevices);
  vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)vulkan_lib.get_proc("vkGetPhysicalDeviceFeatures");
  M_check(vkGetPhysicalDeviceFeatures);
  vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceFormatProperties");
  M_check(vkGetPhysicalDeviceFormatProperties);
  vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceImageFormatProperties");
  M_check(vkGetPhysicalDeviceImageFormatProperties);
  vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceProperties");
  M_check(vkGetPhysicalDeviceProperties);
  vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceQueueFamilyProperties");
  M_check(vkGetPhysicalDeviceQueueFamilyProperties);
  vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceMemoryProperties");
  M_check(vkGetPhysicalDeviceMemoryProperties);
  vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vulkan_lib.get_proc("vkGetInstanceProcAddr");
  M_check(vkGetInstanceProcAddr);
  vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vulkan_lib.get_proc("vkGetDeviceProcAddr");
  M_check(vkGetDeviceProcAddr);
  vkCreateDevice = (PFN_vkCreateDevice)vulkan_lib.get_proc("vkCreateDevice");
  M_check(vkCreateDevice);
  vkDestroyDevice = (PFN_vkDestroyDevice)vulkan_lib.get_proc("vkDestroyDevice");
  M_check(vkDestroyDevice);
  vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vulkan_lib.get_proc("vkEnumerateInstanceExtensionProperties");
  M_check(vkEnumerateInstanceExtensionProperties);
  vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vulkan_lib.get_proc("vkEnumerateDeviceExtensionProperties");
  M_check(vkEnumerateDeviceExtensionProperties);
  vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vulkan_lib.get_proc("vkEnumerateInstanceLayerProperties");
  M_check(vkEnumerateInstanceLayerProperties);
  vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)vulkan_lib.get_proc("vkEnumerateDeviceLayerProperties");
  M_check(vkEnumerateDeviceLayerProperties);
  vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vulkan_lib.get_proc("vkGetDeviceQueue");
  M_check(vkGetDeviceQueue);
  vkQueueSubmit = (PFN_vkQueueSubmit)vulkan_lib.get_proc("vkQueueSubmit");
  M_check(vkQueueSubmit);
  vkQueueWaitIdle = (PFN_vkQueueWaitIdle)vulkan_lib.get_proc("vkQueueWaitIdle");
  M_check(vkQueueWaitIdle);
  vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)vulkan_lib.get_proc("vkDeviceWaitIdle");
  M_check(vkDeviceWaitIdle);
  vkAllocateMemory = (PFN_vkAllocateMemory)vulkan_lib.get_proc("vkAllocateMemory");
  M_check(vkAllocateMemory);
  vkFreeMemory = (PFN_vkFreeMemory)vulkan_lib.get_proc("vkFreeMemory");
  M_check(vkFreeMemory);
  vkMapMemory = (PFN_vkMapMemory)vulkan_lib.get_proc("vkMapMemory");
  M_check(vkMapMemory);
  vkUnmapMemory = (PFN_vkUnmapMemory)vulkan_lib.get_proc("vkUnmapMemory");
  M_check(vkUnmapMemory);
  vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vulkan_lib.get_proc("vkFlushMappedMemoryRanges");
  M_check(vkFlushMappedMemoryRanges);
  vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)vulkan_lib.get_proc("vkInvalidateMappedMemoryRanges");
  M_check(vkInvalidateMappedMemoryRanges);
  vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)vulkan_lib.get_proc("vkGetDeviceMemoryCommitment");
  M_check(vkGetDeviceMemoryCommitment);
  vkBindBufferMemory = (PFN_vkBindBufferMemory)vulkan_lib.get_proc("vkBindBufferMemory");
  M_check(vkBindBufferMemory);
  vkBindImageMemory = (PFN_vkBindImageMemory)vulkan_lib.get_proc("vkBindImageMemory");
  M_check(vkBindImageMemory);
  vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vulkan_lib.get_proc("vkGetBufferMemoryRequirements");
  M_check(vkGetBufferMemoryRequirements);
  vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vulkan_lib.get_proc("vkGetImageMemoryRequirements");
  M_check(vkGetImageMemoryRequirements);
  vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)vulkan_lib.get_proc("vkGetImageSparseMemoryRequirements");
  M_check(vkGetImageSparseMemoryRequirements);
  vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)vulkan_lib.get_proc("vkGetPhysicalDeviceSparseImageFormatProperties");
  M_check(vkGetPhysicalDeviceSparseImageFormatProperties);
  vkQueueBindSparse = (PFN_vkQueueBindSparse)vulkan_lib.get_proc("vkQueueBindSparse");
  M_check(vkQueueBindSparse);
  vkCreateFence = (PFN_vkCreateFence)vulkan_lib.get_proc("vkCreateFence");
  M_check(vkCreateFence);
  vkDestroyFence = (PFN_vkDestroyFence)vulkan_lib.get_proc("vkDestroyFence");
  M_check(vkDestroyFence);
  vkResetFences = (PFN_vkResetFences)vulkan_lib.get_proc("vkResetFences");
  M_check(vkResetFences);
  vkGetFenceStatus = (PFN_vkGetFenceStatus)vulkan_lib.get_proc("vkGetFenceStatus");
  M_check(vkGetFenceStatus);
  vkWaitForFences = (PFN_vkWaitForFences)vulkan_lib.get_proc("vkWaitForFences");
  M_check(vkWaitForFences);
  vkCreateSemaphore = (PFN_vkCreateSemaphore)vulkan_lib.get_proc("vkCreateSemaphore");
  M_check(vkCreateSemaphore);
  vkDestroySemaphore = (PFN_vkDestroySemaphore)vulkan_lib.get_proc("vkDestroySemaphore");
  M_check(vkDestroySemaphore);
  vkCreateEvent = (PFN_vkCreateEvent)vulkan_lib.get_proc("vkCreateEvent");
  M_check(vkCreateEvent);
  vkDestroyEvent = (PFN_vkDestroyEvent)vulkan_lib.get_proc("vkDestroyEvent");
  M_check(vkDestroyEvent);
  vkGetEventStatus = (PFN_vkGetEventStatus)vulkan_lib.get_proc("vkGetEventStatus");
  M_check(vkGetEventStatus);
  vkSetEvent = (PFN_vkSetEvent)vulkan_lib.get_proc("vkSetEvent");
  M_check(vkSetEvent);
  vkResetEvent = (PFN_vkResetEvent)vulkan_lib.get_proc("vkResetEvent");
  M_check(vkResetEvent);
  vkCreateQueryPool = (PFN_vkCreateQueryPool)vulkan_lib.get_proc("vkCreateQueryPool");
  M_check(vkCreateQueryPool);
  vkDestroyQueryPool = (PFN_vkDestroyQueryPool)vulkan_lib.get_proc("vkDestroyQueryPool");
  M_check(vkDestroyQueryPool);
  vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)vulkan_lib.get_proc("vkGetQueryPoolResults");
  M_check(vkGetQueryPoolResults);
  vkCreateBuffer = (PFN_vkCreateBuffer)vulkan_lib.get_proc("vkCreateBuffer");
  M_check(vkCreateBuffer);
  vkDestroyBuffer = (PFN_vkDestroyBuffer)vulkan_lib.get_proc("vkDestroyBuffer");
  M_check(vkDestroyBuffer);
  vkCreateBufferView = (PFN_vkCreateBufferView)vulkan_lib.get_proc("vkCreateBufferView");
  M_check(vkCreateBufferView);
  vkDestroyBufferView = (PFN_vkDestroyBufferView)vulkan_lib.get_proc("vkDestroyBufferView");
  M_check(vkDestroyBufferView);
  vkCreateImage = (PFN_vkCreateImage)vulkan_lib.get_proc("vkCreateImage");
  M_check(vkCreateImage);
  vkDestroyImage = (PFN_vkDestroyImage)vulkan_lib.get_proc("vkDestroyImage");
  M_check(vkDestroyImage);
  vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)vulkan_lib.get_proc("vkGetImageSubresourceLayout");
  M_check(vkGetImageSubresourceLayout);
  vkCreateImageView = (PFN_vkCreateImageView)vulkan_lib.get_proc("vkCreateImageView");
  M_check(vkCreateImageView);
  vkDestroyImageView = (PFN_vkDestroyImageView)vulkan_lib.get_proc("vkDestroyImageView");
  M_check(vkDestroyImageView);
  vkCreateShaderModule = (PFN_vkCreateShaderModule)vulkan_lib.get_proc("vkCreateShaderModule");
  M_check(vkCreateShaderModule);
  vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vulkan_lib.get_proc("vkDestroyShaderModule");
  M_check(vkDestroyShaderModule);
  vkCreatePipelineCache = (PFN_vkCreatePipelineCache)vulkan_lib.get_proc("vkCreatePipelineCache");
  M_check(vkCreatePipelineCache);
  vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)vulkan_lib.get_proc("vkDestroyPipelineCache");
  M_check(vkDestroyPipelineCache);
  vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)vulkan_lib.get_proc("vkGetPipelineCacheData");
  M_check(vkGetPipelineCacheData);
  vkMergePipelineCaches = (PFN_vkMergePipelineCaches)vulkan_lib.get_proc("vkMergePipelineCaches");
  M_check(vkMergePipelineCaches);
  vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)vulkan_lib.get_proc("vkCreateGraphicsPipelines");
  M_check(vkCreateGraphicsPipelines);
  vkCreateComputePipelines = (PFN_vkCreateComputePipelines)vulkan_lib.get_proc("vkCreateComputePipelines");
  M_check(vkCreateComputePipelines);
  vkDestroyPipeline = (PFN_vkDestroyPipeline)vulkan_lib.get_proc("vkDestroyPipeline");
  M_check(vkDestroyPipeline);
  vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vulkan_lib.get_proc("vkCreatePipelineLayout");
  M_check(vkCreatePipelineLayout);
  vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vulkan_lib.get_proc("vkDestroyPipelineLayout");
  M_check(vkDestroyPipelineLayout);
  vkCreateSampler = (PFN_vkCreateSampler)vulkan_lib.get_proc("vkCreateSampler");
  M_check(vkCreateSampler);
  vkDestroySampler = (PFN_vkDestroySampler)vulkan_lib.get_proc("vkDestroySampler");
  M_check(vkDestroySampler);
  vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vulkan_lib.get_proc("vkCreateDescriptorSetLayout");
  M_check(vkCreateDescriptorSetLayout);
  vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vulkan_lib.get_proc("vkDestroyDescriptorSetLayout");
  M_check(vkDestroyDescriptorSetLayout);
  vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vulkan_lib.get_proc("vkCreateDescriptorPool");
  M_check(vkCreateDescriptorPool);
  vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vulkan_lib.get_proc("vkDestroyDescriptorPool");
  M_check(vkDestroyDescriptorPool);
  vkResetDescriptorPool = (PFN_vkResetDescriptorPool)vulkan_lib.get_proc("vkResetDescriptorPool");
  M_check(vkResetDescriptorPool);
  vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vulkan_lib.get_proc("vkAllocateDescriptorSets");
  M_check(vkAllocateDescriptorSets);
  vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)vulkan_lib.get_proc("vkFreeDescriptorSets");
  M_check(vkFreeDescriptorSets);
  vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vulkan_lib.get_proc("vkUpdateDescriptorSets");
  M_check(vkUpdateDescriptorSets);
  vkCreateFramebuffer = (PFN_vkCreateFramebuffer)vulkan_lib.get_proc("vkCreateFramebuffer");
  M_check(vkCreateFramebuffer);
  vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)vulkan_lib.get_proc("vkDestroyFramebuffer");
  M_check(vkDestroyFramebuffer);
  vkCreateRenderPass = (PFN_vkCreateRenderPass)vulkan_lib.get_proc("vkCreateRenderPass");
  M_check(vkCreateRenderPass);
  vkDestroyRenderPass = (PFN_vkDestroyRenderPass)vulkan_lib.get_proc("vkDestroyRenderPass");
  M_check(vkDestroyRenderPass);
  vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)vulkan_lib.get_proc("vkGetRenderAreaGranularity");
  M_check(vkGetRenderAreaGranularity);
  vkCreateCommandPool = (PFN_vkCreateCommandPool)vulkan_lib.get_proc("vkCreateCommandPool");
  M_check(vkCreateCommandPool);
  vkDestroyCommandPool = (PFN_vkDestroyCommandPool)vulkan_lib.get_proc("vkDestroyCommandPool");
  M_check(vkDestroyCommandPool);
  vkResetCommandPool = (PFN_vkResetCommandPool)vulkan_lib.get_proc("vkResetCommandPool");
  M_check(vkResetCommandPool);
  vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vulkan_lib.get_proc("vkAllocateCommandBuffers");
  M_check(vkAllocateCommandBuffers);
  vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)vulkan_lib.get_proc("vkFreeCommandBuffers");
  M_check(vkFreeCommandBuffers);
  vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vulkan_lib.get_proc("vkBeginCommandBuffer");
  M_check(vkBeginCommandBuffer);
  vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vulkan_lib.get_proc("vkEndCommandBuffer");
  M_check(vkEndCommandBuffer);
  vkResetCommandBuffer = (PFN_vkResetCommandBuffer)vulkan_lib.get_proc("vkResetCommandBuffer");
  M_check(vkResetCommandBuffer);
  vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vulkan_lib.get_proc("vkCmdBindPipeline");
  M_check(vkCmdBindPipeline);
  vkCmdSetViewport = (PFN_vkCmdSetViewport)vulkan_lib.get_proc("vkCmdSetViewport");
  M_check(vkCmdSetViewport);
  vkCmdSetScissor = (PFN_vkCmdSetScissor)vulkan_lib.get_proc("vkCmdSetScissor");
  M_check(vkCmdSetScissor);
  vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)vulkan_lib.get_proc("vkCmdSetLineWidth");
  M_check(vkCmdSetLineWidth);
  vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)vulkan_lib.get_proc("vkCmdSetDepthBias");
  M_check(vkCmdSetDepthBias);
  vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)vulkan_lib.get_proc("vkCmdSetBlendConstants");
  M_check(vkCmdSetBlendConstants);
  vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)vulkan_lib.get_proc("vkCmdSetDepthBounds");
  M_check(vkCmdSetDepthBounds);
  vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)vulkan_lib.get_proc("vkCmdSetStencilCompareMask");
  M_check(vkCmdSetStencilCompareMask);
  vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)vulkan_lib.get_proc("vkCmdSetStencilWriteMask");
  M_check(vkCmdSetStencilWriteMask);
  vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)vulkan_lib.get_proc("vkCmdSetStencilReference");
  M_check(vkCmdSetStencilReference);
  vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vulkan_lib.get_proc("vkCmdBindDescriptorSets");
  M_check(vkCmdBindDescriptorSets);
  vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vulkan_lib.get_proc("vkCmdBindIndexBuffer");
  M_check(vkCmdBindIndexBuffer);
  vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vulkan_lib.get_proc("vkCmdBindVertexBuffers");
  M_check(vkCmdBindVertexBuffers);
  vkCmdDraw = (PFN_vkCmdDraw)vulkan_lib.get_proc("vkCmdDraw");
  M_check(vkCmdDraw);
  vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vulkan_lib.get_proc("vkCmdDrawIndexed");
  M_check(vkCmdDrawIndexed);
  vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)vulkan_lib.get_proc("vkCmdDrawIndirect");
  M_check(vkCmdDrawIndirect);
  vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)vulkan_lib.get_proc("vkCmdDrawIndexedIndirect");
  M_check(vkCmdDrawIndexedIndirect);
  vkCmdDispatch = (PFN_vkCmdDispatch)vulkan_lib.get_proc("vkCmdDispatch");
  M_check(vkCmdDispatch);
  vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)vulkan_lib.get_proc("vkCmdDispatchIndirect");
  M_check(vkCmdDispatchIndirect);
  vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vulkan_lib.get_proc("vkCmdCopyBuffer");
  M_check(vkCmdCopyBuffer);
  vkCmdCopyImage = (PFN_vkCmdCopyImage)vulkan_lib.get_proc("vkCmdCopyImage");
  M_check(vkCmdCopyImage);
  vkCmdBlitImage = (PFN_vkCmdBlitImage)vulkan_lib.get_proc("vkCmdBlitImage");
  M_check(vkCmdBlitImage);
  vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vulkan_lib.get_proc("vkCmdCopyBufferToImage");
  M_check(vkCmdCopyBufferToImage);
  vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)vulkan_lib.get_proc("vkCmdCopyImageToBuffer");
  M_check(vkCmdCopyImageToBuffer);
  vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)vulkan_lib.get_proc("vkCmdUpdateBuffer");
  M_check(vkCmdUpdateBuffer);
  vkCmdFillBuffer = (PFN_vkCmdFillBuffer)vulkan_lib.get_proc("vkCmdFillBuffer");
  M_check(vkCmdFillBuffer);
  vkCmdClearColorImage = (PFN_vkCmdClearColorImage)vulkan_lib.get_proc("vkCmdClearColorImage");
  M_check(vkCmdClearColorImage);
  vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)vulkan_lib.get_proc("vkCmdClearDepthStencilImage");
  M_check(vkCmdClearDepthStencilImage);
  vkCmdClearAttachments = (PFN_vkCmdClearAttachments)vulkan_lib.get_proc("vkCmdClearAttachments");
  M_check(vkCmdClearAttachments);
  vkCmdResolveImage = (PFN_vkCmdResolveImage)vulkan_lib.get_proc("vkCmdResolveImage");
  M_check(vkCmdResolveImage);
  vkCmdSetEvent = (PFN_vkCmdSetEvent)vulkan_lib.get_proc("vkCmdSetEvent");
  M_check(vkCmdSetEvent);
  vkCmdResetEvent = (PFN_vkCmdResetEvent)vulkan_lib.get_proc("vkCmdResetEvent");
  M_check(vkCmdResetEvent);
  vkCmdWaitEvents = (PFN_vkCmdWaitEvents)vulkan_lib.get_proc("vkCmdWaitEvents");
  M_check(vkCmdWaitEvents);
  vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vulkan_lib.get_proc("vkCmdPipelineBarrier");
  M_check(vkCmdPipelineBarrier);
  vkCmdBeginQuery = (PFN_vkCmdBeginQuery)vulkan_lib.get_proc("vkCmdBeginQuery");
  M_check(vkCmdBeginQuery);
  vkCmdEndQuery = (PFN_vkCmdEndQuery)vulkan_lib.get_proc("vkCmdEndQuery");
  M_check(vkCmdEndQuery);
  vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)vulkan_lib.get_proc("vkCmdResetQueryPool");
  M_check(vkCmdResetQueryPool);
  vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)vulkan_lib.get_proc("vkCmdWriteTimestamp");
  M_check(vkCmdWriteTimestamp);
  vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)vulkan_lib.get_proc("vkCmdCopyQueryPoolResults");
  M_check(vkCmdCopyQueryPoolResults);
  vkCmdPushConstants = (PFN_vkCmdPushConstants)vulkan_lib.get_proc("vkCmdPushConstants");
  M_check(vkCmdPushConstants);
  vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vulkan_lib.get_proc("vkCmdBeginRenderPass");
  M_check(vkCmdBeginRenderPass);
  vkCmdNextSubpass = (PFN_vkCmdNextSubpass)vulkan_lib.get_proc("vkCmdNextSubpass");
  M_check(vkCmdNextSubpass);
  vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vulkan_lib.get_proc("vkCmdEndRenderPass");
  M_check(vkCmdEndRenderPass);
  vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)vulkan_lib.get_proc("vkCmdExecuteCommands");
  M_check(vkCmdExecuteCommands);

#if M_os_is_win()
  vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vulkan_lib.get_proc("vkCreateWin32SurfaceKHR");;
  M_check(vkCreateWin32SurfaceKHR);
#endif

  vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vulkan_lib.get_proc("vkDestroySurfaceKHR");
  vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vulkan_lib.get_proc("vkGetPhysicalDeviceSurfaceSupportKHR");
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vulkan_lib.get_proc("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
  vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vulkan_lib.get_proc("vkGetPhysicalDeviceSurfaceFormatsKHR");
  vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vulkan_lib.get_proc("vkGetPhysicalDeviceSurfacePresentModesKHR");
}

int main() {
  core_init(M_txt("vulkan_sample.log"));
  Window_t w(M_txt("vulkan_sample"), 1024, 768);
  w.init();
  load_vulkan_();
  Linear_allocator_t<> vulkan_allocator("vulkan_allocator");
  vulkan_allocator.init();
  M_scope_exit(vulkan_allocator.destroy());
  VkInstance instance;
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
    M_vk_check(vkCreateInstance(&instance_ci, nullptr, &instance));
  }

  VkSurfaceKHR surface;
  {
#if M_os_is_win()
    VkWin32SurfaceCreateInfoKHR win32_surface_ci = {};
    win32_surface_ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_ci.hwnd = w.m_platform_data.hwnd;
    win32_surface_ci.hinstance = GetModuleHandle(nullptr);
    M_vk_check(vkCreateWin32SurfaceKHR(instance, &win32_surface_ci, nullptr, &surface));
#elif M_os_is_linux()
    VkXCBSurfaceCreateInfoKHR xcb_surface_ci = {};
    xcb_surface_ci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_surface_ci.connect = w.m_platform_data.hwnd;
    xcb_surface_ci.window = GetModuleHandle(nullptr);
    M_vk_check(vkCreateWin32SurfaceKHR(instance, &win32_surface_ci, nullptr, &surface));
#else
#error "?"
#endif
  }

  VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
  {
    U32 gpu_count = 0;
    M_vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, NULL));
    Dynamic_array_t<VkPhysicalDevice> physical_devices;
    physical_devices.init(&vulkan_allocator);
    M_scope_exit(physical_devices.destroy());
    physical_devices.resize(gpu_count);
    M_vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, physical_devices.m_p));

    VkPhysicalDeviceProperties device_props;
    chosen_device = physical_devices[0];
    for (int i = 0; i < gpu_count; i++) {
      vkGetPhysicalDeviceProperties(physical_devices[i], &device_props);
      if (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        chosen_device = physical_devices[i];
        break;
      }
    }
  }

  {
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(chosen_device, &device_props);
    M_logi("Chosen GPU: %s", device_props.deviceName);
  }

  int graphics_q_idx = -1;
  int present_q_idx = -1;
  {
    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(chosen_device, &queue_family_count, NULL);
    Dynamic_array_t<VkQueueFamilyProperties> queue_properties_array;
    queue_properties_array.init(&vulkan_allocator);
    M_scope_exit(queue_properties_array.destroy());
    queue_properties_array.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(chosen_device, &queue_family_count, queue_properties_array.m_p);
    for (int i = 0; i < queue_family_count; ++i) {
      VkBool32 can_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(chosen_device, i, surface, &can_present);

      if (graphics_q_idx == -1 && queue_properties_array[i].queueCount > 0 && queue_properties_array[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        graphics_q_idx = i;
      }
      if (present_q_idx == -1 && can_present) {
        present_q_idx = i;
      }
      if (graphics_q_idx >= 0 && present_q_idx >= 0) {
        break;
      }
    }
  }

  VkDevice device;
  VkQueue graphics_q;
  VkQueue present_q;
  {
    float q_priority = 1.0f;

    VkDeviceQueueCreateInfo q_ci[2] = {};

    q_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci[0].queueFamilyIndex = graphics_q_idx;
    q_ci[0].queueCount = 1;
    q_ci[0].pQueuePriorities = &q_priority;

    q_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci[0].queueFamilyIndex = present_q_idx;
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

    if (graphics_q_idx == present_q_idx) {
      device_ci.queueCreateInfoCount = 1;
    } else {
      device_ci.queueCreateInfoCount = 2;
    }
    device_ci.enabledExtensionCount = static_array_size(device_exts);
    device_ci.ppEnabledExtensionNames = device_exts;
    device_ci.pEnabledFeatures = NULL;
    device_ci.enabledLayerCount = static_array_size(layers);
    device_ci.ppEnabledLayerNames = layers;
    M_vk_check(vkCreateDevice(chosen_device, &device_ci, nullptr, &device));
    vkGetDeviceQueue(device, graphics_q_idx, 0, &graphics_q);
    vkGetDeviceQueue(device, present_q_idx, 0, &present_q);
  }

	VkDebugReportCallbackEXT callback;
  {
    VkDebugReportCallbackCreateInfoEXT dbg_report_ci = {};
    dbg_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    dbg_report_ci.pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_cb;
    dbg_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

    M_vk_check(vkCreateDebugReportCallbackEXT(instance, &dbg_report_ci, nullptr, &callback));
  }

  VkCommandPool cmd_pool;
  {
    VkCommandPoolCreateInfo pool_ci = {};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.queueFamilyIndex = graphics_q_idx;

    M_vk_check(vkCreateCommandPool(device, &pool_ci, nullptr, &cmd_pool));
  }

  {
    // // Find surface capabilities
    // VkSurfaceCapabilitiesKHR surface_capabilities;
    // M_vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(chosen_device, surface, &surface_capabilities));
    // // Find supported surface formats
    // U32 format_count;
    // M_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_device, surface, &format_count, nullptr));

    // Dynamic_array_t<VkSurfaceFormatKHR> surface_formats;
    // surface_formats.init(&vulkan_allocator);
    // M_scope_exit(surface_formats.destroy());
    // surface_formats.resize(format_count);
    // M_vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_device, surface, &format_count, surface_formats.m_p));

    // // Find supported present modes
    // U32 present_mode_count;
    // M_vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_device, surface, &present_mode_count, nullptr));

    // Dynamic_array_t<VkPresentModeKHR> present_modes;
    // present_modes.init(&vulkan_allocator);
    // M_scope_exit(present_modes.destroy());
    // present_modes.resize(present_mode_count);
    // M_vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_device, surface, &present_mode_count, present_modes.m_p));

    // // Determine number of images for swap chain
    // U32 image_count = surface_capabilities.minImageCount + 1;
    // if (surface_capabilities.maxImageCount != 0 && image_count > surface_capabilities.maxImageCount) {
    //   image_count = surface_capabilities.maxImageCount;
    // }

    // // Select a surface format
    // VkSurfaceFormatKHR surface_format = VK_FORMAT_R8G8B8A8_UNORM;

    // // Select swap chain size
    // VkExtent2D swapchain_extent = { 1024, 768 };

    // // Determine transformation to use (preferring no transform)
    // VkSurfaceTransformFlagBitsKHR surface_transform;
    // if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    //   surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    // } else {
    //   surface_transform = surface_capabilities.currentTransform;
    // }

    // // Choose presentation mode (preferring MAILBOX ~= triple buffering)
    // VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    // // Finally, create the swap chain
    // VkSwapchainCreateInfoKHR swapchain = {};
    // createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // createInfo.surface = surface;
    // createInfo.minImageCount = imageCount;
    // createInfo.imageFormat = surfaceFormat.format;
    // createInfo.imageColorSpace = surfaceFormat.colorSpace;
    // createInfo.imageExtent = swapChainExtent;
    // createInfo.imageArrayLayers = 1;
    // createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // createInfo.queueFamilyIndexCount = 0;
    // createInfo.pQueueFamilyIndices = nullptr;
    // createInfo.preTransform = surfaceTransform;
    // createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // createInfo.presentMode = presentMode;
    // createInfo.clipped = VK_TRUE;
    // createInfo.oldSwapchain = oldSwapChain;

    // if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    //   std::cerr << "failed to create swap chain" << std::endl;
    //   exit(1);
    // } else {
    //   std::cout << "created swap chain" << std::endl;
    // }

    // if (oldSwapChain != VK_NULL_HANDLE) {
    //   vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    // }
    // oldSwapChain = swapChain;

    // swapChainFormat = surfaceFormat.format;

    // // Store the images used by the swap chain
    // // Note: these are the images that swap chain image indices refer to
    // // Note: actual number of images may differ from requested number, since it's a lower bound
    // uint32_t actualImageCount = 0;
    // if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, nullptr) != VK_SUCCESS || actualImageCount == 0) {
    //   std::cerr << "failed to acquire number of swap chain images" << std::endl;
    //   exit(1);
    // }

    // swapChainImages.resize(actualImageCount);

    // if (vkGetSwapchainImagesKHR(device, swapChain, &actualImageCount, swapChainImages.data()) != VK_SUCCESS) {
    //   std::cerr << "failed to acquire swap chain images" << std::endl;
    //   exit(1);
    // }

    // std::cout << "acquired swap chain images" << std::endl;
  }
  w.os_loop();
  return 0;
}
