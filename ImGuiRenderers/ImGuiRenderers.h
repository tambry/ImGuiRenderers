#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "imgui.h"

#ifdef _WIN32
#include <windows.h>

#undef ERROR
#endif

// Size definitions
using s64 = int64_t;
using s32 = int32_t;
using s16 = int16_t;
using s8  = int8_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

// To allow replacement of the logger for a custom logger
#ifndef REPLACE_LOGGER
#include "Logger.h"
#endif

class ImGuiRenderer
{
public:
	virtual bool initialize(void* handle, void* instance, void* renderer_options);
	virtual void new_frame();

protected:
	// Internal values
	float width = 0;
	float height = 0;
	void* window_handle;
	void* window_instance;
	s64 ticks_per_second;
	s64 time;
};

#include "Renderers/VulkanRenderer.h"