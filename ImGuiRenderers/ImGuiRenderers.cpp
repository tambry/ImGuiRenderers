#include "ImGuiRenderers.h"

bool ImGuiRenderer::initialize(void* handle, void* instance, void* options)
{
	// Set some basic ImGui info
	ImGuiIO& io = ImGui::GetIO();
	io.ImeWindowHandle = handle;

	// Internal values
	window_handle = handle;
	window_instance = instance;

#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&ticks_per_second);
	QueryPerformanceCounter((LARGE_INTEGER*)&time);

	// Keymap
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
#endif

	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	return true;
}

void ImGuiRenderer::new_frame()
{
	ImGuiIO& io = ImGui::GetIO();

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
}