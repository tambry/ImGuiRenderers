#pragma once

#include "ImGuiVulkanRenderer.h"

// A lightweight logger implementation from https://github.com/tambry/Common
#include <stdarg.h>

enum LogLevel : u8
{
	ERROR,
	INFO,
	WARNING,
	DEBUG,
	NONE,
};

void log(LogLevel level, const std::string format, ...);