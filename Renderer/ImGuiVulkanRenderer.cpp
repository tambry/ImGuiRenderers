#include "ImGuiVulkanRenderer.h"

ImGuiVulkanRenderer::~ImGuiVulkanRenderer()
{
	// Must wait to make sure that the objects can be safely destroyed
	VkResult result;

	if (device)
	{
		if ((result = vkDeviceWaitIdle(device)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to wait for the device to become idle. (%d)", result);
			return;
		}

		// We need to check if these objects exist, or else we'll crash
		if (font_sampler)
		{
			vkDestroySampler(device, font_sampler, nullptr);
		}

		if (font_memory)
		{
			vkFreeMemory(device, font_memory, nullptr);
		}

		if (font_image_view)
		{
			vkDestroyImageView(device, font_image_view, nullptr);
		}

		if (font_image)
		{
			vkDestroyImage(device, font_image, nullptr);
		}

		if (pipeline)
		{
			vkDestroyPipeline(device, pipeline, nullptr);
		}

		if (pipeline_cache)
		{
			vkDestroyPipelineCache(device, pipeline_cache, nullptr);
		}

		if (pipeline_layout)
		{
			vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		}

		if (descriptor_set_layout)
		{
			vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
		}

		if (descriptor_set)
		{
			vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
		}

		if (descriptor_pool)
		{
			vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
		}

		if (render_pass)
		{
			vkDestroyRenderPass(device, render_pass, nullptr);
		}

		if (vertex_shader)
		{
			vkDestroyShaderModule(device, vertex_shader, nullptr);
		}

		if (fragment_shader)
		{
			vkDestroyShaderModule(device, fragment_shader, nullptr);
		}

		for (u8 i = 0; i < 2; i++)
		{
			if (swapchain_image_views[i])
			{
				vkDestroyImageView(device, swapchain_image_views[i], nullptr);
			}
		}

		if (command_pool)
		{
			vkDestroyCommandPool(device, command_pool, nullptr);
		}

		if (swapchain)
		{
			vkDestroySwapchainKHR(device, swapchain, nullptr);
		}

		vkDestroyDevice(device, nullptr);
	}

	if (surface)
	{
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

	if (destroy_debug_report)
	{
		destroy_debug_report(instance, debug_callback_function, nullptr);
	}

	if (instance)
	{
		vkDestroyInstance(instance, nullptr);
	}
}

bool ImGuiVulkanRenderer::create_swapchain_image_views()
{
	VkResult result;

	// Begin a command buffer
	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.pNext = nullptr;
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to begin a command buffer. (%d)", result);
		return false;
	}

	// Create image views for the swapchain images
	for (u8 i = 0; i < 2; ++i)
	{
		VkImageMemoryBarrier swapchain_barrier = {};
		swapchain_barrier.pNext = nullptr;
		swapchain_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapchain_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		swapchain_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapchain_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchain_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapchain_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		swapchain_barrier.image = swapchain_images[i];

		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapchain_barrier);

		VkImageViewCreateInfo swap_chain_image_view = {};
		swap_chain_image_view.pNext = nullptr;
		swap_chain_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		swap_chain_image_view.format = surface_format.format;
		swap_chain_image_view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		swap_chain_image_view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		swap_chain_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		swap_chain_image_view.image = swapchain_images[i];

		if ((result = vkCreateImageView(device, &swap_chain_image_view, nullptr, &swapchain_image_views[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to create swapchain image view. (%d)", result);
			return false;
		}
	}

	if ((result = vkEndCommandBuffer(command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to end the command buffer. (%d)", result);
		return false;
	}

	return true;
}

// Get the first device that supports graphics output and surfaces
u32 ImGuiVulkanRenderer::get_graphics_family(VkPhysicalDevice adapter, VkSurfaceKHR window_surface)
{
	// Get the amount of family queues
	u32 family_queue_count;
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &family_queue_count, nullptr);

	// Get list of family queues
	std::vector<VkQueueFamilyProperties> queues(family_queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &family_queue_count, queues.data());

	for (u32 i = 0; i < queues.size(); i++)
	{
		if (!(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			continue;
		}

		VkBool32 supported;

		if (vkGetPhysicalDeviceSurfaceSupportKHR(adapter, i, window_surface, &supported) != VK_SUCCESS)
		{
			log(ERROR, "Failed to query surface support.");
			return 0xBAD;
		}

		if (supported)
		{
			return i;
		}
	}

	return 0xBAD;
}

VkBool32 ImGuiVulkanRenderer::get_memory_type(u32 typeBits, VkFlags properties, u32 *typeIndex)
{
	for (u8 i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				*typeIndex = i;
				return true;
			}
		}

		typeBits >>= 1;
	}

	return false;
}

VkShaderModule ImGuiVulkanRenderer::load_shader(std::string file_name)
{
	std::ifstream stream(file_name, std::ios::binary);

	if (!stream)
	{
		log(ERROR, "Failed to open shader file.");
		return nullptr;
	}

	stream.seekg(0, std::ios::end);
	u64 size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	char* shader_code = (char*)malloc(size);

	if (shader_code == nullptr)
	{
		log(ERROR, "Failed to allocate memory for shader.");
	}

	stream.read(shader_code, size);

	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize = size;
	shader_module_info.pCode = (u32*)shader_code;

	VkShaderModule shader_module;
	VkResult result;

	if ((result = vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a shader module. (%d)", result);
		return nullptr;
	}

	free(shader_code);

	return shader_module;
}

VkShaderModule ImGuiVulkanRenderer::load_shader(const u8* shader, u64 size)
{
	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize = size;
	shader_module_info.pCode = (u32*)shader;

	VkShaderModule shader_module;
	VkResult result;

	if ((result = vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a shader module. (%d)", result);
		return nullptr;
	}

	return shader_module;
}

VkBool32 ImGuiVulkanRenderer::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, u64 src, u64 location, s32 msg_code, char* prefix, char* msg, void* user_data)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		log(ERROR, "[%s]: %s", prefix, msg);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT || flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		log(WARNING, "[%s]: %s", prefix, msg);
	}
	else
	{
		log(WARNING, "[%s]: %s", prefix, msg);
	}

	return false;
}

void ImGuiVulkanRenderer::new_frame()
{
	ImGuiIO& io = ImGui::GetIO();

	float width = 0;
	float height = 0;

	// Get the window width/height depending on the platform and calculate the delta time
#ifdef _WIN32
	HWND handle = static_cast<HWND>(window_handle);
	RECT coordinates;

	if (GetClientRect(handle, &coordinates) == 0)
	{
		log(ERROR, "Failed to obtain the size of the window. (%d)", GetLastError());
		return;
	}

	width = static_cast<float>(coordinates.right - coordinates.left);
	height = static_cast<float>(coordinates.bottom - coordinates.top);

	// Delta time calculation
	s64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	io.DeltaTime = (float)(current_time - time) / ticks_per_second;
	time = current_time;

	// Set the keyboard modifiers
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

	// Whether the cursor is being drawn in software
	SetCursor(io.MouseDrawCursor ? nullptr : LoadCursor(nullptr, IDC_ARROW));
#else
	// TODO: Unix implementation
#endif

	// The swapchain needs to be recreated, when the window is resized or else bad things happen
	if (io.DisplaySize.x != width || io.DisplaySize.y != height)
	{
		VkResult result;

		// Get the surface capabilities
		VkSurfaceCapabilitiesKHR surface_capabilities;

		if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to obtain the device surface capabilities. (%d)", result);
			return;
		}

		// Recreate the swapchain
		VkSwapchainKHR old_swapchain = swapchain;

		VkSwapchainCreateInfoKHR swapchain_info = {};
		swapchain_info.pNext = nullptr;
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = surface;
		swapchain_info.oldSwapchain = swapchain;
		swapchain_info.imageArrayLayers = 1;
		swapchain_info.imageExtent = surface_capabilities.currentExtent;
		swapchain_info.imageFormat = surface_format.format;
		swapchain_info.presentMode = present_mode;
		swapchain_info.minImageCount = 2;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

		if ((result = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to recreate the swapchain. (%d)", result);
			return;
		}

		// We still have to destroy the old swapchain to free all the associated memory
		vkDestroySwapchainKHR(device, old_swapchain, nullptr);

		// Get the new swapchain images
		u32 swapchain_image_count;

		if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
			return;
		}

		if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
			return;
		}

		// Destroy the previous swapchain image views
		for (u8 i = 0; i < 2; i++)
		{
			vkDestroyImageView(device, swapchain_image_views[i], nullptr);
		}

		// Recreate the swapchain image views
		if (!create_swapchain_image_views())
		{
			log(ERROR, "Failed to create swapchain image views.");
			return;
		}

		io.DisplaySize.x = width;
		io.DisplaySize.y = height;
	}

	// Calculate the delta time (platform-specific)
#ifdef _WIN32

#else
	// TODO: Unix version
#endif

	ImGui::NewFrame();
}

void ImGuiVulkanRenderer::imgui_render(ImDrawData* draw_data)
{
	ImGuiVulkanRenderer& renderer = *(ImGuiVulkanRenderer*)ImGui::GetIO().UserData;
	ImGuiIO& io = ImGui::GetIO();

	u32 width = static_cast<u32>(io.DisplaySize.x);
	u32 height = static_cast<u32>(io.DisplaySize.y);
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	VkResult result;
	u32 current_buffer;

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.pNext = nullptr;
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;

	if ((result = vkCreateSemaphore(renderer.device, &semaphore_info, nullptr, &semaphore)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a semaphore. (%d)", result);
		return;
	}

	if ((result = vkAcquireNextImageKHR(renderer.device, renderer.swapchain, UINT64_MAX, semaphore, nullptr, &current_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the index of the next image in the chain. (%d)", result);
		return;
	}

	VkCommandBufferBeginInfo command_buffer_begin = {};
	command_buffer_begin.pNext = nullptr;
	command_buffer_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((result = vkBeginCommandBuffer(renderer.command_buffer, &command_buffer_begin)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to begin the command buffer. (%d)", result);
		return;
	}

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.pNext = nullptr;
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.width = width;
	framebuffer_info.height = height;
	framebuffer_info.renderPass = renderer.render_pass;
	framebuffer_info.attachmentCount = 1;
	framebuffer_info.layers = 1;
	framebuffer_info.pAttachments = &renderer.swapchain_image_views[current_buffer];

	VkFramebuffer framebuffer;

	if ((result = vkCreateFramebuffer(renderer.device, &framebuffer_info, nullptr, &framebuffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a framebuffer. (%d)", result);
		return;
	}

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.framebuffer = framebuffer;
	render_pass_begin_info.renderPass = renderer.render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &renderer.clear_value;

	vkCmdBeginRenderPass(renderer.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.width = io.DisplaySize.x;
	viewport.height = io.DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(renderer.command_buffer, 0, 1, &viewport);

	// Projection matrix
	const float ortho_projection[4][4] =
	{
		{ 2.0f / io.DisplaySize.x,  0.0f, 0.0f, 0.0f },
		{ 0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f, 0.0f },
		{ -1.0f, 1.0f,  0.0f, 1.0f },
	};

	vkCmdBindDescriptorSets(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline_layout, 0, 1, &renderer.descriptor_set, 0, nullptr);
	vkCmdPushConstants(renderer.command_buffer, renderer.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 16, &ortho_projection);
	vkCmdBindPipeline(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);

	std::vector<VkDeviceMemory> buffer_memory(draw_data->CmdListsCount);
	std::vector<VkBuffer> render_buffer(draw_data->CmdListsCount);

	for (s32 i = 0; i < draw_data->CmdListsCount; i++)
	{
		ImDrawList* draw_list = draw_data->CmdLists[i];

		VkBufferCreateInfo render_buffer_info = {};
		render_buffer_info.pNext = nullptr;
		render_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		render_buffer_info.size = draw_list->IdxBuffer.size() * sizeof(ImDrawIdx) + draw_list->VtxBuffer.size() * sizeof(ImDrawVert);
		render_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		if ((result = vkCreateBuffer(renderer.device, &render_buffer_info, nullptr, &render_buffer[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to create a buffer for rendering. (%d)", result);
			return;
		}

		VkMemoryRequirements buffer_memory_requirements;
		vkGetBufferMemoryRequirements(renderer.device, render_buffer[i], &buffer_memory_requirements);

		VkMemoryAllocateInfo memory_allocation_info = {};
		memory_allocation_info.pNext = nullptr;
		memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_allocation_info.allocationSize = buffer_memory_requirements.size;

		renderer.get_memory_type(buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocation_info.memoryTypeIndex);

		if ((result = vkAllocateMemory(renderer.device, &memory_allocation_info, nullptr, &buffer_memory[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to allocate memory for rendering. (%d)", result);
			return;
		}

		void* data;

		if ((result = vkMapMemory(renderer.device, buffer_memory[i], 0, memory_allocation_info.allocationSize, 0, &data)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to map memory for rendering buffer. (%d)", result);
			return;
		}

		memcpy(data, &draw_list->VtxBuffer.front(), draw_list->VtxBuffer.size() * sizeof(ImDrawVert));
		memcpy((u8*)data + draw_list->VtxBuffer.size() * sizeof(ImDrawVert), &draw_list->IdxBuffer.front(), draw_list->IdxBuffer.size() * sizeof(ImDrawIdx));

		vkUnmapMemory(renderer.device, buffer_memory[i]);

		if ((result = vkBindBufferMemory(renderer.device, render_buffer[i], buffer_memory[i], 0)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to bind memory for rendering buffer. (%d)", result);
			return;
		}

		u64 offset = 0;
		vkCmdBindVertexBuffers(renderer.command_buffer, 0, 1, &render_buffer[i], &offset);
		vkCmdBindIndexBuffer(renderer.command_buffer, render_buffer[i], draw_list->VtxBuffer.size() * sizeof(ImDrawVert), VK_INDEX_TYPE_UINT16);

		u32 index_offset = 0;

		for (s32 j = 0; j < draw_list->CmdBuffer.size(); j++)
		{
			ImDrawCmd* draw_cmd = &draw_list->CmdBuffer[j];

			if (draw_cmd->UserCallback)
			{
				draw_cmd->UserCallback(draw_list, draw_cmd);
			}
			else
			{
				VkRect2D scissor;
				scissor.offset.x = static_cast<s32>(draw_cmd->ClipRect.x);
				scissor.offset.y = static_cast<s32>(draw_cmd->ClipRect.y);
				scissor.extent.width = static_cast<s32>(draw_cmd->ClipRect.z - draw_cmd->ClipRect.x);
				scissor.extent.height = static_cast<s32>(draw_cmd->ClipRect.w - draw_cmd->ClipRect.y);

				vkCmdSetScissor(renderer.command_buffer, 0, 1, &scissor);
				vkCmdDrawIndexed(renderer.command_buffer, draw_cmd->ElemCount, 1, index_offset, 0, 0);
			}

			index_offset += draw_cmd->ElemCount;
		}
	}

	vkCmdEndRenderPass(renderer.command_buffer);

	// Post present image memory barrier
	VkImageMemoryBarrier post_present_barrier = {};
	post_present_barrier.pNext = nullptr;
	post_present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	post_present_barrier.srcAccessMask = 0;
	post_present_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	post_present_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	post_present_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	post_present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	post_present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	post_present_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	post_present_barrier.image = renderer.swapchain_images[current_buffer];

	vkCmdPipelineBarrier(renderer.command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &post_present_barrier);

	if ((result = vkEndCommandBuffer(renderer.command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to end the command buffer. (%d)", result);
		return;
	}

	VkSubmitInfo submit_info = {};
	submit_info.pNext = nullptr;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &semaphore;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &renderer.command_buffer;

	if ((result = vkQueueSubmit(renderer.queue, 1, &submit_info, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to submit to the queue. (%d)", result);
		return;
	}

	VkPresentInfoKHR present_info = {};
	present_info.pNext = nullptr;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &renderer.swapchain;
	present_info.pImageIndices = &current_buffer;

	if ((result = vkQueuePresentKHR(renderer.queue, &present_info)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to present swapchain image. (%d)", result);
		return;
	}

	// We need to make sure everything has finished before destroying destroying objects and freeing memory
	if ((result = vkDeviceWaitIdle(renderer.device)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to wait for the device to become idle. (%d)", result);
		return;
	}

	vkDestroyFramebuffer(renderer.device, framebuffer, nullptr);
	vkDestroySemaphore(renderer.device, semaphore, nullptr);

	for (s32 i = 0; i < draw_data->CmdListsCount; i++)
	{
		vkDestroyBuffer(renderer.device, render_buffer[i], nullptr);
		vkFreeMemory(renderer.device, buffer_memory[i], nullptr);
	}
}

bool ImGuiVulkanRenderer::prepare_vulkan(u8 device_num, bool validation_layers)
{
	// The names of validation layers
	const char* validation_layer_names[] =
	{
		"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_GOOGLE_unique_objects",
		"VK_LAYER_LUNARG_api_dump",
	};

	std::vector<const char*> instance_extensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	std::vector<const char*> layer_names;

	// Add all the validation layers (change to 3, to enable API call dumping)
	if (validation_layers)
	{
		for (u8 i = 0; i < 2; i++)
		{
			layer_names.push_back(validation_layer_names[i]);
		}
	}

	// Set application info
	VkApplicationInfo application_info = {};
	application_info.pNext = nullptr;
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "NGEmu";
	application_info.pEngineName = "NGEmu";
	application_info.apiVersion = VK_MAKE_VERSION(1, 0, 4);

	// Set Vulkan instance info and create the instance
	VkInstanceCreateInfo instance_info = {};
	instance_info.pNext = nullptr;
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &application_info;
	instance_info.enabledLayerCount = (u32)layer_names.size();
	instance_info.ppEnabledLayerNames = layer_names.data();
	instance_info.enabledExtensionCount = (u32)instance_extensions.size();
	instance_info.ppEnabledExtensionNames = instance_extensions.data();

	VkResult result;

	if ((result = vkCreateInstance(&instance_info, nullptr, &instance)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a Vulkan instance. (%d)", result);
		return false;
	}

	// Set up the debug callback
	create_debug_report = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	destroy_debug_report = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

	if (!create_debug_report || !destroy_debug_report)
	{
		log(ERROR, "Failed to obtain debug reporting function addresses. (%d)", result);
		return false;
	}

	VkDebugReportFlagsEXT flags = {};
	flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
	flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
	flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

	VkDebugReportCallbackCreateInfoEXT debug_callback_info = {};
	debug_callback_info.pNext = nullptr;
	debug_callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_callback_info.pfnCallback = (PFN_vkDebugReportCallbackEXT)debug_callback;
	debug_callback_info.flags = flags;

	create_debug_report(instance, &debug_callback_info, nullptr, &debug_callback_function);

	// Find an appropriate device
	u32 device_count;

	if ((result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of devices. (%d)", result);
		return false;
	}

	std::vector<VkPhysicalDevice> adapters(device_count);

	if ((result = vkEnumeratePhysicalDevices(instance, &device_count, adapters.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain devices. (%d)", result);
		return false;
	}

	physical_device = adapters[device_num];

	// Create a window surface depending on the platform
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR window_surface_info = {};
	window_surface_info.pNext = nullptr;
	window_surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	window_surface_info.hwnd = static_cast<HWND>(window_handle);
	window_surface_info.hinstance = static_cast<HINSTANCE>(window_instance);

	if ((result = vkCreateWin32SurfaceKHR(instance, &window_surface_info, nullptr, &surface)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create window surface. (%d)", result);
		return false;
	}
#else
	// TODO: Unix version
#endif

	// Get the first graphic family, that supports Vulkan
	u32 queue_family = get_graphics_family(physical_device, surface);

	if (queue_family == 0xBAD)
	{
		log(ERROR, "Failed to obtain a graphics family, that supports Vulkan. (%d)", result);
		return false;
	}

	// Create a Vulkan device
	float queue_priority = 1;

	VkDeviceQueueCreateInfo device_queue_info = {};
	device_queue_info.pNext = nullptr;
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = queue_family;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = &queue_priority;

	std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_info = {};
	device_info.pNext = nullptr;
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.enabledLayerCount = (u32)layer_names.size();
	device_info.ppEnabledLayerNames = layer_names.data();
	device_info.enabledExtensionCount = (u32)device_extensions.size();
	device_info.ppEnabledExtensionNames = device_extensions.data();

	if ((result = vkCreateDevice(physical_device, &device_info, nullptr, &device)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a Vulkan device handle. (%d)", result);
		return false;
	}

	// Get the queue
	vkGetDeviceQueue(device, queue_family, 0, &queue);

	// Get the memory properties
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	// Get surface capabilities, formats and presentation modes
	u32 format_count;
	u32 present_mode_count;

	if ((vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of device surface formats. (%d)", result);
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of device surface present modes. (%d)", result);
		return false;
	}

	std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
	std::vector<VkPresentModeKHR> present_modes(present_mode_count);

	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the surface formats. (%d)", result);
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the surface present modes. (%d)", result);
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_capabilities;

	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the device surface capabilities. (%d)", result);
		return false;
	}

	surface_format = surface_formats[0];
	present_mode = present_modes[0];

	// Create a swapchain
	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.pNext = nullptr;
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageExtent = surface_capabilities.currentExtent;
	swapchain_info.imageFormat = surface_format.format;
	swapchain_info.presentMode = present_mode;
	swapchain_info.minImageCount = 2;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	if ((result = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a swapchain. (%d)", result);
		return false;
	}

	u32 swapchain_image_count;

	if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
		return false;
	}

	if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
		return false;
	}

	// Set the ImGui size values
	ImGuiIO io = ImGui::GetIO();
	io.DisplaySize.x = static_cast<float>(surface_capabilities.currentExtent.width);
	io.DisplaySize.y = static_cast<float>(surface_capabilities.currentExtent.height);

	// Create a command pool
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.pNext = nullptr;
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex = get_graphics_family(physical_device, surface);;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if ((result = vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a command pool. (%d)", result);
		return false;
	}

	// Allocate a command buffer
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.pNext = nullptr;
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = 1;

	if ((result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate a command buffer. (%d)", result);
		return false;
	}

	if (!create_swapchain_image_views())
	{
		log(ERROR, "Failed to create swapchain image views.");
		return false;
	}

	// Create a render pass
	VkAttachmentDescription attachement_description = {};
	attachement_description.format = VK_FORMAT_R8G8B8A8_UNORM;
	attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
	attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachement_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachement_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachment_reference = {};
	attachment_reference.attachment = 0;
	attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description = {};
	subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &attachment_reference;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.pNext = nullptr;
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass_description;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &attachement_description;

	if ((result = vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a render pass. (%d)", result);
		return false;
	}

	// Create a descriptor set layout
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
	descriptor_set_layout_binding.binding = 0;
	descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
	descriptor_set_layout_info.pNext = nullptr;
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &descriptor_set_layout_binding;

	if ((result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a descriptor set layout. (%d)", result);
		return false;
	}

	// Create the pipeline layout
	VkPushConstantRange push_constant_range;
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.size = sizeof(float) * 16;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.pNext = nullptr;
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	if ((result = vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a pipeline layout. (%d)", result);
		return false;
	}

	// Prepare vertex input bindings
	vertex_input_binding.binding = 0;
	vertex_input_binding.stride = sizeof(ImDrawVert);
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Prepare vertex input attributes
	vertex_input_attribute[0].location = 0;
	vertex_input_attribute[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute[0].offset = offsetof(ImDrawVert, pos);

	vertex_input_attribute[1].location = 1;
	vertex_input_attribute[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute[1].offset = offsetof(ImDrawVert, uv);

	vertex_input_attribute[2].location = 2;
	vertex_input_attribute[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertex_input_attribute[2].offset = offsetof(ImDrawVert, col);

	// Create a rendering pipeline
	// Vertex input info
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.pNext = nullptr;
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexAttributeDescriptionCount = 3;
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding;

	// Vertex input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.pNext = nullptr;
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport info
	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.pNext = nullptr;
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.scissorCount = 1;

	// Rasterization info
	VkPipelineRasterizationStateCreateInfo rasterization_info = {};
	rasterization_info.pNext = nullptr;
	rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_info.cullMode = VK_CULL_MODE_NONE;
	rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_info.depthClampEnable = VK_FALSE;
	rasterization_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_info.depthBiasEnable = VK_FALSE;

	// Multisampling info
	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	multisample_info.pNext = nullptr;
	multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth stencil info
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	depth_stencil_info.pNext = nullptr;
	depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil_info.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil_info.stencilTestEnable = VK_FALSE;
	depth_stencil_info.front = depth_stencil_info.back;

	// Color blending info
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorWriteMask = 0xF;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.pNext = nullptr;
	color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;

	// Dynamic states
	std::vector<VkDynamicState> dynamic_states =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_info;
	dynamic_info.pNext = nullptr;
	dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_info.dynamicStateCount = (u32)dynamic_states.size();
	dynamic_info.pDynamicStates = dynamic_states.data();

	// Shaders
	if (precompiled_shaders)
	{
		vertex_shader = load_shader(imgui_vertex.data(), imgui_vertex.size());
		fragment_shader = load_shader(imgui_fragment.data(), imgui_fragment.size());
	}
	else
	{
		vertex_shader = load_shader(vertex_shader_path);
		fragment_shader = load_shader(fragment_shader_path);
	}

	VkPipelineShaderStageCreateInfo shader_info[2] = {};
	shader_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_info[0].module = vertex_shader;
	shader_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_info[0].pName = "main";

	shader_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_info[1].module = fragment_shader;
	shader_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_info[1].pName = "main";

	// Create the pipeline cache
	VkPipelineCacheCreateInfo pipeline_cache_info = {};
	pipeline_cache_info.pNext = nullptr;
	pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	if ((result = vkCreatePipelineCache(device, &pipeline_cache_info, nullptr, &pipeline_cache)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a pipeline cache. (%d)", result);
		return false;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.pNext = nullptr;
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_info;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState = &viewport_info;
	pipeline_info.pRasterizationState = &rasterization_info;
	pipeline_info.pMultisampleState = &multisample_info;
	pipeline_info.pDepthStencilState = &depth_stencil_info;
	pipeline_info.pColorBlendState = &color_blend_info;
	pipeline_info.pDynamicState = &dynamic_info;
	pipeline_info.renderPass = render_pass;
	pipeline_info.layout = pipeline_layout;

	if ((result = vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &pipeline)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a graphics pipeline. (%d)", result);
		return false;
	}

	// Setup a descriptor pool
	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_info = {};
	descriptor_pool_info.pNext = nullptr;
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_info.poolSizeCount = 1;
	descriptor_pool_info.pPoolSizes = &descriptor_pool_size;
	descriptor_pool_info.maxSets = 1;

	if ((result = vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr, &descriptor_pool)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a descriptor pool. (%d)", result);
		return false;
	}

	// Setup up the descriptor set
	VkDescriptorSetAllocateInfo descriptor_set_info = {};
	descriptor_set_info.pNext = nullptr;
	descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_info.descriptorPool = descriptor_pool;
	descriptor_set_info.descriptorSetCount = 1;
	descriptor_set_info.pSetLayouts = &descriptor_set_layout;

	if ((result = vkAllocateDescriptorSets(device, &descriptor_set_info, &descriptor_set)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate a descriptor set. (%d)", result);
		return false;
	}

	u8* pixels;
	s32 width, height;

	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Prepare font texture
	VkImageCreateInfo image_info = {};
	image_info.pNext = nullptr;
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.extent = { (u32)width, (u32)height, 1 };
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if ((result = vkCreateImage(device, &image_info, nullptr, &font_image)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a font image. (%d)", result);
		return false;
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, font_image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocation_info = {};
	memory_allocation_info.pNext = nullptr;
	memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocation_info.allocationSize = memory_requirements.size;

	if (!get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocation_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get the memory type. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocation_info, nullptr, &font_memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate memory for font texture. (%d)", result);
		return false;
	}

	if ((result = vkBindImageMemory(device, font_image, font_memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory for font texture. (%d)", result);
		return false;
	}

	VkImageViewCreateInfo image_view_info;
	image_view_info.pNext = nullptr;
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = font_image;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	image_view_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	if ((result = vkCreateImageView(device, &image_view_info, nullptr, &font_image_view)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create image view for font texture. (%d)", result);
		return false;
	}

	VkSamplerCreateInfo sampler_info;
	sampler_info.pNext = nullptr;
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_info.unnormalizedCoordinates = VK_FALSE;

	if ((result = vkCreateSampler(device, &sampler_info, nullptr, &font_sampler)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a sampler for font texture. (%d)", result);
		return false;
	}

	// Update descriptors
	VkDescriptorImageInfo descriptor_image_info = {};
	descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	descriptor_image_info.sampler = font_sampler;
	descriptor_image_info.imageView = font_image_view;

	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.pNext = nullptr;
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set.pImageInfo = &descriptor_image_info;

	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

	// Upload the image to the GPU
	VkImageSubresource image_subresource = {};
	image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkSubresourceLayout subresource_layout = {};
	vkGetImageSubresourceLayout(device, font_image, &image_subresource, &subresource_layout);

	void* data;

	if ((result = vkMapMemory(device, font_memory, 0, subresource_layout.size, 0, &data)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for font texture upload. (%d)", result);
		return false;
	}

	memcpy(data, pixels, subresource_layout.size);

	vkUnmapMemory(device, font_memory);

	return true;
}

bool ImGuiVulkanRenderer::initialize(void* handle, void* instance, ImGuiVulkanOptions options)
{
	// Set some basic ImGui info
	ImGuiIO& io = ImGui::GetIO();
	io.ImeWindowHandle = handle;
	io.RenderDrawListsFn = imgui_render;
	io.UserData = this;

	// Set some internal values
	window_handle = handle;
	window_instance = instance;
	clear_value = options.clear_value;
	precompiled_shaders = options.use_precompiled_shaders;

	if (!options.vertex_shader.empty())
	{
		vertex_shader_path = options.vertex_shader;
	}

	if (!options.fragment_shader.empty())
	{
		fragment_shader_path = options.fragment_shader;
	}

	if (!prepare_vulkan(options.device_number, options.validation_layers))
	{
		log(ERROR, "Failed to initialize Vulkan renderer.");
		return false;
	}

#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&ticks_per_second);
	QueryPerformanceCounter((LARGE_INTEGER*)&time);
#endif

	return true;
}