# ImGuiRenderers
A collection of self-contained, object-oriented, lightweight renderers for ImGui.
Currently there is only one renderer: Vulkan.

## Compilation
The project can be added to your existing solution by adding the ImGuiRenderers.vcxproj.

You will need to place the ImGuiRenderers.props properties file one directory level below the root directory. The file must contain include directory paths to the _vulkan.h_ and _imgui.h_. An example of the file is included in the root directory.

Upon compilation, the project will generate a static library named ImGuiRenderers one level down in the _lib_ directory, which you'll need to link against.

## Usage
The renderer is used through creating an object of the renderer. The user will need to provide an options structure, that contains info needed by the renderer.
The keymap is set up by the library, but the handling of input is left up to the user. The keymap can be overridden.

```c++
#include "ImGuiRenderers.h"
#include "imgui.h"

int loop()
{
	// Create a window for rendering
    // ...

    std::unique_ptr<ImGuiRenderer> renderer;
	renderer.reset(ImGuiVulkanRenderer);
	
	// The clear values for clearing the surface
	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.1f;
	clear_value.color.float32[1] = 0.075f;
	clear_value.color.float32[2] = 0.025f;
	
	ImGuiVulkanOptions vulkan_options;
	vulkan_options.clear_value = clear_value;
	vulkan_options.device_number = 0;              // Number of the device to be used for rendering
	vulkan_options.validation_layers = false;      // Whether to enable Vulkan validation layers or not
	vulkan_options.use_precompiled_shaders = true; // Whether to use included precompiled shaders or not. Using precompiled shaders is highly recommended.
	vulkan_options.vertex_shader = "...";          // Vertex shader path. Default path is ../shaders/imgui.vert.spv
	vulkan_options.fragment_shader = "...";        // Fragment shader path. Default path is ../shaders/imgui.frag.spv
    
    if (!renderer.initialize(window_handle, window_instance, &vulkan_options))
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
* OpenGL renderer (#5)
* Custom rendering (#4)
* Linux support (#2)
* Improved Vulkan renderer perfomance (#1)