//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/gpu/vulkan/vulkan_loader.h"

#include "core/dynamic_lib.h"
#include "core/log.h"

#define M_vk_check_return_false(condition) { \
  M_check_return_val(condition, false); \
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
#elif M_os_is_linux()
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
#endif

PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR;
PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR;
PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR;

Dynamic_lib_t g_vulkan_lib_;

bool vulkan_loader_init() {
#if M_os_is_win()
  g_vulkan_lib_.open("vulkan-1.dll");
#elif M_os_is_linux()
  g_vulkan_lib_.open("libvulkan.so");
#endif

  vkCreateInstance = (PFN_vkCreateInstance)g_vulkan_lib_.get_proc("vkCreateInstance");
  M_vk_check_return_false(vkCreateInstance);
  vkDestroyInstance = (PFN_vkDestroyInstance)g_vulkan_lib_.get_proc("vkDestroyInstance");
  M_vk_check_return_false(vkDestroyInstance);
  vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)g_vulkan_lib_.get_proc("vkEnumeratePhysicalDevices");
  M_vk_check_return_false(vkEnumeratePhysicalDevices);
  vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceFeatures");
  M_vk_check_return_false(vkGetPhysicalDeviceFeatures);
  vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceFormatProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceFormatProperties);
  vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceImageFormatProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceImageFormatProperties);
  vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceProperties);
  vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceQueueFamilyProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceQueueFamilyProperties);
  vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceMemoryProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceMemoryProperties);
  vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)g_vulkan_lib_.get_proc("vkGetInstanceProcAddr");
  M_vk_check_return_false(vkGetInstanceProcAddr);
  vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)g_vulkan_lib_.get_proc("vkGetDeviceProcAddr");
  M_vk_check_return_false(vkGetDeviceProcAddr);
  vkCreateDevice = (PFN_vkCreateDevice)g_vulkan_lib_.get_proc("vkCreateDevice");
  M_vk_check_return_false(vkCreateDevice);
  vkDestroyDevice = (PFN_vkDestroyDevice)g_vulkan_lib_.get_proc("vkDestroyDevice");
  M_vk_check_return_false(vkDestroyDevice);
  vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)g_vulkan_lib_.get_proc("vkEnumerateInstanceExtensionProperties");
  M_vk_check_return_false(vkEnumerateInstanceExtensionProperties);
  vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)g_vulkan_lib_.get_proc("vkEnumerateDeviceExtensionProperties");
  M_vk_check_return_false(vkEnumerateDeviceExtensionProperties);
  vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)g_vulkan_lib_.get_proc("vkEnumerateInstanceLayerProperties");
  M_vk_check_return_false(vkEnumerateInstanceLayerProperties);
  vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)g_vulkan_lib_.get_proc("vkEnumerateDeviceLayerProperties");
  M_vk_check_return_false(vkEnumerateDeviceLayerProperties);
  vkGetDeviceQueue = (PFN_vkGetDeviceQueue)g_vulkan_lib_.get_proc("vkGetDeviceQueue");
  M_vk_check_return_false(vkGetDeviceQueue);
  vkQueueSubmit = (PFN_vkQueueSubmit)g_vulkan_lib_.get_proc("vkQueueSubmit");
  M_vk_check_return_false(vkQueueSubmit);
  vkQueueWaitIdle = (PFN_vkQueueWaitIdle)g_vulkan_lib_.get_proc("vkQueueWaitIdle");
  M_vk_check_return_false(vkQueueWaitIdle);
  vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)g_vulkan_lib_.get_proc("vkDeviceWaitIdle");
  M_vk_check_return_false(vkDeviceWaitIdle);
  vkAllocateMemory = (PFN_vkAllocateMemory)g_vulkan_lib_.get_proc("vkAllocateMemory");
  M_vk_check_return_false(vkAllocateMemory);
  vkFreeMemory = (PFN_vkFreeMemory)g_vulkan_lib_.get_proc("vkFreeMemory");
  M_vk_check_return_false(vkFreeMemory);
  vkMapMemory = (PFN_vkMapMemory)g_vulkan_lib_.get_proc("vkMapMemory");
  M_vk_check_return_false(vkMapMemory);
  vkUnmapMemory = (PFN_vkUnmapMemory)g_vulkan_lib_.get_proc("vkUnmapMemory");
  M_vk_check_return_false(vkUnmapMemory);
  vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)g_vulkan_lib_.get_proc("vkFlushMappedMemoryRanges");
  M_vk_check_return_false(vkFlushMappedMemoryRanges);
  vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)g_vulkan_lib_.get_proc("vkInvalidateMappedMemoryRanges");
  M_vk_check_return_false(vkInvalidateMappedMemoryRanges);
  vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)g_vulkan_lib_.get_proc("vkGetDeviceMemoryCommitment");
  M_vk_check_return_false(vkGetDeviceMemoryCommitment);
  vkBindBufferMemory = (PFN_vkBindBufferMemory)g_vulkan_lib_.get_proc("vkBindBufferMemory");
  M_vk_check_return_false(vkBindBufferMemory);
  vkBindImageMemory = (PFN_vkBindImageMemory)g_vulkan_lib_.get_proc("vkBindImageMemory");
  M_vk_check_return_false(vkBindImageMemory);
  vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)g_vulkan_lib_.get_proc("vkGetBufferMemoryRequirements");
  M_vk_check_return_false(vkGetBufferMemoryRequirements);
  vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)g_vulkan_lib_.get_proc("vkGetImageMemoryRequirements");
  M_vk_check_return_false(vkGetImageMemoryRequirements);
  vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)g_vulkan_lib_.get_proc("vkGetImageSparseMemoryRequirements");
  M_vk_check_return_false(vkGetImageSparseMemoryRequirements);
  vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceSparseImageFormatProperties");
  M_vk_check_return_false(vkGetPhysicalDeviceSparseImageFormatProperties);
  vkQueueBindSparse = (PFN_vkQueueBindSparse)g_vulkan_lib_.get_proc("vkQueueBindSparse");
  M_vk_check_return_false(vkQueueBindSparse);
  vkCreateFence = (PFN_vkCreateFence)g_vulkan_lib_.get_proc("vkCreateFence");
  M_vk_check_return_false(vkCreateFence);
  vkDestroyFence = (PFN_vkDestroyFence)g_vulkan_lib_.get_proc("vkDestroyFence");
  M_vk_check_return_false(vkDestroyFence);
  vkResetFences = (PFN_vkResetFences)g_vulkan_lib_.get_proc("vkResetFences");
  M_vk_check_return_false(vkResetFences);
  vkGetFenceStatus = (PFN_vkGetFenceStatus)g_vulkan_lib_.get_proc("vkGetFenceStatus");
  M_vk_check_return_false(vkGetFenceStatus);
  vkWaitForFences = (PFN_vkWaitForFences)g_vulkan_lib_.get_proc("vkWaitForFences");
  M_vk_check_return_false(vkWaitForFences);
  vkCreateSemaphore = (PFN_vkCreateSemaphore)g_vulkan_lib_.get_proc("vkCreateSemaphore");
  M_vk_check_return_false(vkCreateSemaphore);
  vkDestroySemaphore = (PFN_vkDestroySemaphore)g_vulkan_lib_.get_proc("vkDestroySemaphore");
  M_vk_check_return_false(vkDestroySemaphore);
  vkCreateEvent = (PFN_vkCreateEvent)g_vulkan_lib_.get_proc("vkCreateEvent");
  M_vk_check_return_false(vkCreateEvent);
  vkDestroyEvent = (PFN_vkDestroyEvent)g_vulkan_lib_.get_proc("vkDestroyEvent");
  M_vk_check_return_false(vkDestroyEvent);
  vkGetEventStatus = (PFN_vkGetEventStatus)g_vulkan_lib_.get_proc("vkGetEventStatus");
  M_vk_check_return_false(vkGetEventStatus);
  vkSetEvent = (PFN_vkSetEvent)g_vulkan_lib_.get_proc("vkSetEvent");
  M_vk_check_return_false(vkSetEvent);
  vkResetEvent = (PFN_vkResetEvent)g_vulkan_lib_.get_proc("vkResetEvent");
  M_vk_check_return_false(vkResetEvent);
  vkCreateQueryPool = (PFN_vkCreateQueryPool)g_vulkan_lib_.get_proc("vkCreateQueryPool");
  M_vk_check_return_false(vkCreateQueryPool);
  vkDestroyQueryPool = (PFN_vkDestroyQueryPool)g_vulkan_lib_.get_proc("vkDestroyQueryPool");
  M_vk_check_return_false(vkDestroyQueryPool);
  vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)g_vulkan_lib_.get_proc("vkGetQueryPoolResults");
  M_vk_check_return_false(vkGetQueryPoolResults);
  vkCreateBuffer = (PFN_vkCreateBuffer)g_vulkan_lib_.get_proc("vkCreateBuffer");
  M_vk_check_return_false(vkCreateBuffer);
  vkDestroyBuffer = (PFN_vkDestroyBuffer)g_vulkan_lib_.get_proc("vkDestroyBuffer");
  M_vk_check_return_false(vkDestroyBuffer);
  vkCreateBufferView = (PFN_vkCreateBufferView)g_vulkan_lib_.get_proc("vkCreateBufferView");
  M_vk_check_return_false(vkCreateBufferView);
  vkDestroyBufferView = (PFN_vkDestroyBufferView)g_vulkan_lib_.get_proc("vkDestroyBufferView");
  M_vk_check_return_false(vkDestroyBufferView);
  vkCreateImage = (PFN_vkCreateImage)g_vulkan_lib_.get_proc("vkCreateImage");
  M_vk_check_return_false(vkCreateImage);
  vkDestroyImage = (PFN_vkDestroyImage)g_vulkan_lib_.get_proc("vkDestroyImage");
  M_vk_check_return_false(vkDestroyImage);
  vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)g_vulkan_lib_.get_proc("vkGetImageSubresourceLayout");
  M_vk_check_return_false(vkGetImageSubresourceLayout);
  vkCreateImageView = (PFN_vkCreateImageView)g_vulkan_lib_.get_proc("vkCreateImageView");
  M_vk_check_return_false(vkCreateImageView);
  vkDestroyImageView = (PFN_vkDestroyImageView)g_vulkan_lib_.get_proc("vkDestroyImageView");
  M_vk_check_return_false(vkDestroyImageView);
  vkCreateShaderModule = (PFN_vkCreateShaderModule)g_vulkan_lib_.get_proc("vkCreateShaderModule");
  M_vk_check_return_false(vkCreateShaderModule);
  vkDestroyShaderModule = (PFN_vkDestroyShaderModule)g_vulkan_lib_.get_proc("vkDestroyShaderModule");
  M_vk_check_return_false(vkDestroyShaderModule);
  vkCreatePipelineCache = (PFN_vkCreatePipelineCache)g_vulkan_lib_.get_proc("vkCreatePipelineCache");
  M_vk_check_return_false(vkCreatePipelineCache);
  vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)g_vulkan_lib_.get_proc("vkDestroyPipelineCache");
  M_vk_check_return_false(vkDestroyPipelineCache);
  vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)g_vulkan_lib_.get_proc("vkGetPipelineCacheData");
  M_vk_check_return_false(vkGetPipelineCacheData);
  vkMergePipelineCaches = (PFN_vkMergePipelineCaches)g_vulkan_lib_.get_proc("vkMergePipelineCaches");
  M_vk_check_return_false(vkMergePipelineCaches);
  vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)g_vulkan_lib_.get_proc("vkCreateGraphicsPipelines");
  M_vk_check_return_false(vkCreateGraphicsPipelines);
  vkCreateComputePipelines = (PFN_vkCreateComputePipelines)g_vulkan_lib_.get_proc("vkCreateComputePipelines");
  M_vk_check_return_false(vkCreateComputePipelines);
  vkDestroyPipeline = (PFN_vkDestroyPipeline)g_vulkan_lib_.get_proc("vkDestroyPipeline");
  M_vk_check_return_false(vkDestroyPipeline);
  vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)g_vulkan_lib_.get_proc("vkCreatePipelineLayout");
  M_vk_check_return_false(vkCreatePipelineLayout);
  vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)g_vulkan_lib_.get_proc("vkDestroyPipelineLayout");
  M_vk_check_return_false(vkDestroyPipelineLayout);
  vkCreateSampler = (PFN_vkCreateSampler)g_vulkan_lib_.get_proc("vkCreateSampler");
  M_vk_check_return_false(vkCreateSampler);
  vkDestroySampler = (PFN_vkDestroySampler)g_vulkan_lib_.get_proc("vkDestroySampler");
  M_vk_check_return_false(vkDestroySampler);
  vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)g_vulkan_lib_.get_proc("vkCreateDescriptorSetLayout");
  M_vk_check_return_false(vkCreateDescriptorSetLayout);
  vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)g_vulkan_lib_.get_proc("vkDestroyDescriptorSetLayout");
  M_vk_check_return_false(vkDestroyDescriptorSetLayout);
  vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)g_vulkan_lib_.get_proc("vkCreateDescriptorPool");
  M_vk_check_return_false(vkCreateDescriptorPool);
  vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)g_vulkan_lib_.get_proc("vkDestroyDescriptorPool");
  M_vk_check_return_false(vkDestroyDescriptorPool);
  vkResetDescriptorPool = (PFN_vkResetDescriptorPool)g_vulkan_lib_.get_proc("vkResetDescriptorPool");
  M_vk_check_return_false(vkResetDescriptorPool);
  vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)g_vulkan_lib_.get_proc("vkAllocateDescriptorSets");
  M_vk_check_return_false(vkAllocateDescriptorSets);
  vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)g_vulkan_lib_.get_proc("vkFreeDescriptorSets");
  M_vk_check_return_false(vkFreeDescriptorSets);
  vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)g_vulkan_lib_.get_proc("vkUpdateDescriptorSets");
  M_vk_check_return_false(vkUpdateDescriptorSets);
  vkCreateFramebuffer = (PFN_vkCreateFramebuffer)g_vulkan_lib_.get_proc("vkCreateFramebuffer");
  M_vk_check_return_false(vkCreateFramebuffer);
  vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)g_vulkan_lib_.get_proc("vkDestroyFramebuffer");
  M_vk_check_return_false(vkDestroyFramebuffer);
  vkCreateRenderPass = (PFN_vkCreateRenderPass)g_vulkan_lib_.get_proc("vkCreateRenderPass");
  M_vk_check_return_false(vkCreateRenderPass);
  vkDestroyRenderPass = (PFN_vkDestroyRenderPass)g_vulkan_lib_.get_proc("vkDestroyRenderPass");
  M_vk_check_return_false(vkDestroyRenderPass);
  vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)g_vulkan_lib_.get_proc("vkGetRenderAreaGranularity");
  M_vk_check_return_false(vkGetRenderAreaGranularity);
  vkCreateCommandPool = (PFN_vkCreateCommandPool)g_vulkan_lib_.get_proc("vkCreateCommandPool");
  M_vk_check_return_false(vkCreateCommandPool);
  vkDestroyCommandPool = (PFN_vkDestroyCommandPool)g_vulkan_lib_.get_proc("vkDestroyCommandPool");
  M_vk_check_return_false(vkDestroyCommandPool);
  vkResetCommandPool = (PFN_vkResetCommandPool)g_vulkan_lib_.get_proc("vkResetCommandPool");
  M_vk_check_return_false(vkResetCommandPool);
  vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)g_vulkan_lib_.get_proc("vkAllocateCommandBuffers");
  M_vk_check_return_false(vkAllocateCommandBuffers);
  vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)g_vulkan_lib_.get_proc("vkFreeCommandBuffers");
  M_vk_check_return_false(vkFreeCommandBuffers);
  vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)g_vulkan_lib_.get_proc("vkBeginCommandBuffer");
  M_vk_check_return_false(vkBeginCommandBuffer);
  vkEndCommandBuffer = (PFN_vkEndCommandBuffer)g_vulkan_lib_.get_proc("vkEndCommandBuffer");
  M_vk_check_return_false(vkEndCommandBuffer);
  vkResetCommandBuffer = (PFN_vkResetCommandBuffer)g_vulkan_lib_.get_proc("vkResetCommandBuffer");
  M_vk_check_return_false(vkResetCommandBuffer);
  vkCmdBindPipeline = (PFN_vkCmdBindPipeline)g_vulkan_lib_.get_proc("vkCmdBindPipeline");
  M_vk_check_return_false(vkCmdBindPipeline);
  vkCmdSetViewport = (PFN_vkCmdSetViewport)g_vulkan_lib_.get_proc("vkCmdSetViewport");
  M_vk_check_return_false(vkCmdSetViewport);
  vkCmdSetScissor = (PFN_vkCmdSetScissor)g_vulkan_lib_.get_proc("vkCmdSetScissor");
  M_vk_check_return_false(vkCmdSetScissor);
  vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)g_vulkan_lib_.get_proc("vkCmdSetLineWidth");
  M_vk_check_return_false(vkCmdSetLineWidth);
  vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)g_vulkan_lib_.get_proc("vkCmdSetDepthBias");
  M_vk_check_return_false(vkCmdSetDepthBias);
  vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)g_vulkan_lib_.get_proc("vkCmdSetBlendConstants");
  M_vk_check_return_false(vkCmdSetBlendConstants);
  vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)g_vulkan_lib_.get_proc("vkCmdSetDepthBounds");
  M_vk_check_return_false(vkCmdSetDepthBounds);
  vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)g_vulkan_lib_.get_proc("vkCmdSetStencilCompareMask");
  M_vk_check_return_false(vkCmdSetStencilCompareMask);
  vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)g_vulkan_lib_.get_proc("vkCmdSetStencilWriteMask");
  M_vk_check_return_false(vkCmdSetStencilWriteMask);
  vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)g_vulkan_lib_.get_proc("vkCmdSetStencilReference");
  M_vk_check_return_false(vkCmdSetStencilReference);
  vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)g_vulkan_lib_.get_proc("vkCmdBindDescriptorSets");
  M_vk_check_return_false(vkCmdBindDescriptorSets);
  vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)g_vulkan_lib_.get_proc("vkCmdBindIndexBuffer");
  M_vk_check_return_false(vkCmdBindIndexBuffer);
  vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)g_vulkan_lib_.get_proc("vkCmdBindVertexBuffers");
  M_vk_check_return_false(vkCmdBindVertexBuffers);
  vkCmdDraw = (PFN_vkCmdDraw)g_vulkan_lib_.get_proc("vkCmdDraw");
  M_vk_check_return_false(vkCmdDraw);
  vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)g_vulkan_lib_.get_proc("vkCmdDrawIndexed");
  M_vk_check_return_false(vkCmdDrawIndexed);
  vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)g_vulkan_lib_.get_proc("vkCmdDrawIndirect");
  M_vk_check_return_false(vkCmdDrawIndirect);
  vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)g_vulkan_lib_.get_proc("vkCmdDrawIndexedIndirect");
  M_vk_check_return_false(vkCmdDrawIndexedIndirect);
  vkCmdDispatch = (PFN_vkCmdDispatch)g_vulkan_lib_.get_proc("vkCmdDispatch");
  M_vk_check_return_false(vkCmdDispatch);
  vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)g_vulkan_lib_.get_proc("vkCmdDispatchIndirect");
  M_vk_check_return_false(vkCmdDispatchIndirect);
  vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)g_vulkan_lib_.get_proc("vkCmdCopyBuffer");
  M_vk_check_return_false(vkCmdCopyBuffer);
  vkCmdCopyImage = (PFN_vkCmdCopyImage)g_vulkan_lib_.get_proc("vkCmdCopyImage");
  M_vk_check_return_false(vkCmdCopyImage);
  vkCmdBlitImage = (PFN_vkCmdBlitImage)g_vulkan_lib_.get_proc("vkCmdBlitImage");
  M_vk_check_return_false(vkCmdBlitImage);
  vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)g_vulkan_lib_.get_proc("vkCmdCopyBufferToImage");
  M_vk_check_return_false(vkCmdCopyBufferToImage);
  vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)g_vulkan_lib_.get_proc("vkCmdCopyImageToBuffer");
  M_vk_check_return_false(vkCmdCopyImageToBuffer);
  vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)g_vulkan_lib_.get_proc("vkCmdUpdateBuffer");
  M_vk_check_return_false(vkCmdUpdateBuffer);
  vkCmdFillBuffer = (PFN_vkCmdFillBuffer)g_vulkan_lib_.get_proc("vkCmdFillBuffer");
  M_vk_check_return_false(vkCmdFillBuffer);
  vkCmdClearColorImage = (PFN_vkCmdClearColorImage)g_vulkan_lib_.get_proc("vkCmdClearColorImage");
  M_vk_check_return_false(vkCmdClearColorImage);
  vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)g_vulkan_lib_.get_proc("vkCmdClearDepthStencilImage");
  M_vk_check_return_false(vkCmdClearDepthStencilImage);
  vkCmdClearAttachments = (PFN_vkCmdClearAttachments)g_vulkan_lib_.get_proc("vkCmdClearAttachments");
  M_vk_check_return_false(vkCmdClearAttachments);
  vkCmdResolveImage = (PFN_vkCmdResolveImage)g_vulkan_lib_.get_proc("vkCmdResolveImage");
  M_vk_check_return_false(vkCmdResolveImage);
  vkCmdSetEvent = (PFN_vkCmdSetEvent)g_vulkan_lib_.get_proc("vkCmdSetEvent");
  M_vk_check_return_false(vkCmdSetEvent);
  vkCmdResetEvent = (PFN_vkCmdResetEvent)g_vulkan_lib_.get_proc("vkCmdResetEvent");
  M_vk_check_return_false(vkCmdResetEvent);
  vkCmdWaitEvents = (PFN_vkCmdWaitEvents)g_vulkan_lib_.get_proc("vkCmdWaitEvents");
  M_vk_check_return_false(vkCmdWaitEvents);
  vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)g_vulkan_lib_.get_proc("vkCmdPipelineBarrier");
  M_vk_check_return_false(vkCmdPipelineBarrier);
  vkCmdBeginQuery = (PFN_vkCmdBeginQuery)g_vulkan_lib_.get_proc("vkCmdBeginQuery");
  M_vk_check_return_false(vkCmdBeginQuery);
  vkCmdEndQuery = (PFN_vkCmdEndQuery)g_vulkan_lib_.get_proc("vkCmdEndQuery");
  M_vk_check_return_false(vkCmdEndQuery);
  vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)g_vulkan_lib_.get_proc("vkCmdResetQueryPool");
  M_vk_check_return_false(vkCmdResetQueryPool);
  vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)g_vulkan_lib_.get_proc("vkCmdWriteTimestamp");
  M_vk_check_return_false(vkCmdWriteTimestamp);
  vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)g_vulkan_lib_.get_proc("vkCmdCopyQueryPoolResults");
  M_vk_check_return_false(vkCmdCopyQueryPoolResults);
  vkCmdPushConstants = (PFN_vkCmdPushConstants)g_vulkan_lib_.get_proc("vkCmdPushConstants");
  M_vk_check_return_false(vkCmdPushConstants);
  vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)g_vulkan_lib_.get_proc("vkCmdBeginRenderPass");
  M_vk_check_return_false(vkCmdBeginRenderPass);
  vkCmdNextSubpass = (PFN_vkCmdNextSubpass)g_vulkan_lib_.get_proc("vkCmdNextSubpass");
  M_vk_check_return_false(vkCmdNextSubpass);
  vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)g_vulkan_lib_.get_proc("vkCmdEndRenderPass");
  M_vk_check_return_false(vkCmdEndRenderPass);
  vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)g_vulkan_lib_.get_proc("vkCmdExecuteCommands");
  M_vk_check_return_false(vkCmdExecuteCommands);

#if M_os_is_win()
  vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)g_vulkan_lib_.get_proc("vkCreateWin32SurfaceKHR");;
  M_vk_check_return_false(vkCreateWin32SurfaceKHR);
#elif M_os_is_linux()
  vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)g_vulkan_lib_.get_proc("vkCreateXcbSurfaceKHR");;
  M_vk_check_return_false(vkCreateXcbSurfaceKHR);
#endif

  vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)g_vulkan_lib_.get_proc("vkDestroySurfaceKHR");
  M_vk_check_return_false(vkDestroySurfaceKHR);
  vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceSurfaceSupportKHR");
  M_vk_check_return_false(vkGetPhysicalDeviceSurfaceSupportKHR);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
  M_vk_check_return_false(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceSurfaceFormatsKHR");
  M_vk_check_return_false(vkGetPhysicalDeviceSurfaceFormatsKHR);
  vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)g_vulkan_lib_.get_proc("vkGetPhysicalDeviceSurfacePresentModesKHR");
  M_vk_check_return_false(vkGetPhysicalDeviceSurfacePresentModesKHR);
  vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)g_vulkan_lib_.get_proc("vkCreateSwapchainKHR");
  M_vk_check_return_false(vkCreateSwapchainKHR);
  vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)g_vulkan_lib_.get_proc("vkDestroySwapchainKHR");
  M_vk_check_return_false(vkDestroySwapchainKHR);
  vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)g_vulkan_lib_.get_proc("vkGetSwapchainImagesKHR");
  M_vk_check_return_false(vkGetSwapchainImagesKHR);
  vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)g_vulkan_lib_.get_proc("vkAcquireNextImageKHR");
  M_vk_check_return_false(vkAcquireNextImageKHR);
  vkQueuePresentKHR = (PFN_vkQueuePresentKHR)g_vulkan_lib_.get_proc("vkQueuePresentKHR");
  M_vk_check_return_false(vkQueuePresentKHR);
  vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)g_vulkan_lib_.get_proc("vkGetDeviceGroupPresentCapabilitiesKHR");
  M_vk_check_return_false(vkGetDeviceGroupPresentCapabilitiesKHR);
  vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)g_vulkan_lib_.get_proc("vkGetDeviceGroupSurfacePresentModesKHR");
  M_vk_check_return_false(vkGetDeviceGroupSurfacePresentModesKHR);
  vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)g_vulkan_lib_.get_proc("vkGetPhysicalDevicePresentRectanglesKHR");
  M_vk_check_return_false(vkGetPhysicalDevicePresentRectanglesKHR);
  vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)g_vulkan_lib_.get_proc("vkAcquireNextImage2KHR");
  M_vk_check_return_false(vkAcquireNextImage2KHR);
  return true;
}
