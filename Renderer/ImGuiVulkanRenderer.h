#pragma once

#include <fstream>
#include <stdint.h>
#include <vector>

// Platform-specific includes and surface extension defines
#ifdef _WIN32
#include <windows.h>

#undef ERROR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Vulkan and imgui headers
#include "vulkan/vulkan.h"
#include "imgui.h"

// Size definitions
using s64 = int64_t;
using s32 = int32_t;
using s16 = int16_t;
using s8  = int8_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

// To allow replacement of the logger for a more functional one
#ifndef REPLACE_LOGGER
#include "Logger.h"
#endif

// Stores the options for the renderer, which are passed during initialization.
struct ImGuiVulkanOptions
{
	VkClearValue clear_value;
	bool validation_layers;
	u8 device_number;
	std::string vertex_shader;
	std::string fragment_shader;
};

class ImGuiVulkanRenderer
{
public:
	ImGuiVulkanRenderer::~ImGuiVulkanRenderer();
	bool initialize(void* handle, void* h_instance, ImGuiVulkanOptions options);
	void new_frame();

	// TODO: These shouldn't probably be public. Maybe use a struct with needed handles for rendering?
	// Vulkan
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
	VkPhysicalDeviceMemoryProperties memory_properties;

	// Rendering
	VkImage swapchain_images[2];
	VkImageView swapchain_image_views[2];
	VkPipeline pipeline;
	VkPipelineCache pipeline_cache;
	VkPipelineLayout pipeline_layout;
	VkDescriptorSet descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkVertexInputAttributeDescription vertex_input_attribute[3];
	VkVertexInputBindingDescription vertex_input_binding;
	VkRenderPass render_pass;
	VkClearValue clear_value;
	VkShaderModule vertex_shader;
	VkShaderModule fragment_shader;

	// For convenience
	VkBool32 get_memory_type(u32 typeBits, VkFlags properties, u32 *typeIndex);

private:
	// For debug output
	PFN_vkCreateDebugReportCallbackEXT create_debug_report;
	PFN_vkDestroyDebugReportCallbackEXT destroy_debug_report;
	VkDebugReportCallbackEXT debug_callback_function;
	static VkBool32 debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, u64 src, u64 location, s32 msg_code, char* prefix, char* msg, void* user_data);

	// Vulkan
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;

	// For font
	VkImage font_image;
	VkImageView font_image_view;
	VkSampler font_sampler;
	VkDeviceMemory font_memory;

	// For convenience
	u32 get_graphics_family(VkPhysicalDevice adapter, VkSurfaceKHR window_surface);
	VkShaderModule load_shader_GLSL(std::string file_name);
	bool create_swapchain_image_views();

	// Internal functions for Vulkan
	bool prepare_vulkan(u8 device_num, bool validation_layers);
	static void imgui_render(ImDrawData* draw_data);

	// Internal values
	void* window_handle;
	void* window_instance;
	std::string vertex_shader_path = "../shaders/imgui.vert.spv";
	std::string fragment_shader_path = "../shaders/imgui.frag.spv";
};