# ImGuiVulkanRenderer
A self-contained, object-oriented, lightweight Vulkan renderer for ImGui.

## Compilation
The project can be added to your existing solution by adding the ImGuiVulkanRenderer.vcxproj.

You will need to place the ImGuiVulkanRenderer.props properties file one directory level below the root directory. The file must contain include directory paths to the _vulkan.h_ and the _imgui.h_. An example is included in the root directory.

Upon compilation, the project will generate a static library named ImGuiVulkanRenderer in the root directory, which you'll need to link against.

## Usage
The renderer is used through creating an object of the renderer. The user will need to provide an options structure, that contains some basic info.  
Handling of input is left up to the user.

```c++
#include "ImGuiVulkanRenderer.h"
#include "imgui.h"

int main()
{
	// Create a window for rendering
    // ...

	// The clear values for clearing the surface
	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.1f;
	clear_value.color.float32[1] = 0.075f;
	clear_value.color.float32[2] = 0.025f;

	// Options structure
	ImGuiVulkanOptions options;
	options.clear_value = clear_value;
	options.device_number = 0; // Number of the device to be used for rendering
	options.validation_layers = false; // Whether to enable Vulkan validation layers or not
    options.vertex_shader = "..."; // Vertex shader path. Default path is ../shaders/imgui.vert.spv
    options.fragment_shader = "..."; // Fragment shader path. Default path is ../shaders/imgui.frag.spv
    
    ImGuiVulkanRenderer renderer;
    
    if (!renderer.initialize(window_handle, window_instance, options))
    {
    	// Error handling
    }
    
    // ...
    
    while (running)
    {
    	renderer.new_frame();
        
        // Render using ImGui here
        // ...
        
        ImGui::Render();
    }
}
```

The default logger can be replaced by defining _REPLACE_LOGGER_ before including the header. A function named log will have to be made with the following definition along with the log level enumerator:

```c++
// Log level enumerator
enum LogLevel : unsigned char
{
	ERROR,
	INFO,
	WARNING,
	DEBUG,
	NONE,
};

// Log function definition
void log(LogLevel level, const std::string format, ...);
```

## Todo
* Linux support (#2)
* Improved perfomance (#1)