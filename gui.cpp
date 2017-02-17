#include "gui.h"
#include "debug.h"

#include "engine.h"
#include <stdio.h>

constexpr const mbflag_t LMB = 1;
constexpr const mbflag_t MMB = 2;
constexpr const mbflag_t RMB = 4;

InputManager::sKeyStateBuffer InputManager::KeyStateBuffer;
uint16_t InputManager::lastMouseX;
uint16_t InputManager::lastMouseY;

const WindowParameters& VulkanWindow::getParams() const noexcept
{
	return m_params;
}

mbflag_t InputManager::getMouseState() noexcept
{
	return KeyStateBuffer.mouse;
}

bool InputManager::getKeyState(keycode_t k) noexcept
{
	return KeyStateBuffer.keyboard[k];
}

///////////////////////////////////////////////////////////////////////////////////////////
////////////////////					W I N  3 2					///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#ifdef VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WindowEventHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INPUT: {
		UINT raw_size;
		auto raw_data = std::make_unique<RAWINPUT>();

		//should return 0, otherwise there's an error
		if (0 != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &raw_size, sizeof(RAWINPUTHEADER))) return -1;
		//returns number of bytes copied to raw_data, if not equal to raw_size then there's an error
		if (raw_size != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_data.get(), &raw_size, sizeof(RAWINPUTHEADER))) return -1;

		//Send input to engine's input manager (which is effectively current scene's input manager)
		VulkanEngine::get().getInputManager().ManageInput(std::move(raw_data));

		return 0; }
	case WM_SIZE: {
		static bool first_run = true;
		if (first_run)
		{
			first_run = false;
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		else
		{
			VulkanEngine::get().onResize();
			return 0;
		}
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
};

void InputManager::ManageInput(std::unique_ptr<input_t, std::function<void(input_t*)>>&& raw)
{	
	//Manage keyboard input
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{		
		if (raw->data.keyboard.Flags == RI_KEY_E0 || raw->data.keyboard.Flags == RI_KEY_MAKE)
		{
			InputManager::KeyStateBuffer.keyboard[raw->data.keyboard.VKey] = true;
			KeyPressed(raw->data.keyboard.VKey);
		}
		else if (raw->data.keyboard.Flags == RI_KEY_BREAK)
		{
			InputManager::KeyStateBuffer.keyboard[raw->data.keyboard.VKey] = false;
			KeyReleased(raw->data.keyboard.VKey);
		}
	}
	//Manage mouse input
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		//update keystate buffers
		switch (raw->data.mouse.usButtonFlags)
		{
		case RI_MOUSE_LEFT_BUTTON_DOWN:
			InputManager::KeyStateBuffer.mouse |= LMB;
			MousePressed(LMB);
			break;
		case RI_MOUSE_LEFT_BUTTON_UP:
			InputManager::KeyStateBuffer.mouse &= ~LMB;
			MouseReleased(LMB);
			break;
		case RI_MOUSE_RIGHT_BUTTON_DOWN:
			InputManager::KeyStateBuffer.mouse |= RMB;
			MousePressed(RMB);
			break;
		case RI_MOUSE_RIGHT_BUTTON_UP:
			InputManager::KeyStateBuffer.mouse &= ~RMB;
			MouseReleased(RMB);
			break;
		default:
			break;
		}

		if (raw->data.mouse.lLastX != 0 || raw->data.mouse.lLastY != 0)
		{
			if (InputManager::KeyStateBuffer.mouse != 0)
				MouseDragged(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
			else
				MouseMoved(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
		}
	}
}

bool VulkanWindow::manageEvents(InputManager& im)
{
	//poll for a message and return if there are none
	MSG msg;
	if(!PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		return false;
	
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	
	return true;
}

std::pair<int16_t, int16_t> InputManager::getCursorPosWin(VulkanWindow& win)
{
	POINT p;
	HWND hwnd = win.getParams().hwnd;

	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	return std::pair<int16_t, int16_t>(p.x, p.y);
}

VulkanWindow::VulkanWindow()
{
	RAWINPUTDEVICE devices[2];

	//Mouse
	devices[0].usUsagePage = 0x01;
	devices[0].usUsage = 0x02;
	devices[0].dwFlags = 0;// RIDEV_NOLEGACY;
	devices[0].hwndTarget = 0;

	//Keyboard
	devices[1].usUsagePage = 0x01;
	devices[1].usUsage = 0x06;
	devices[1].dwFlags = RIDEV_NOLEGACY;
	devices[1].hwndTarget = 0;

	RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));

	UINT width = 1920;
	UINT height = 1080;

	m_params.hinstance = GetModuleHandle(NULL);
	LPCTSTR window_name = "MainWindow";
	LPCTSTR class_name = "MainWindow";

	WNDCLASSEX wnd_class{};
	wnd_class.cbSize = sizeof(WNDCLASSEX);
	wnd_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wnd_class.lpszClassName = window_name;
	wnd_class.hInstance = m_params.hinstance;
	wnd_class.style = CS_HREDRAW | CS_VREDRAW;
	wnd_class.lpfnWndProc = WindowEventHandler;
	wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClassEx(&wnd_class);

	RECT window_rect;
	window_rect.bottom = height;
	window_rect.left = 0;
	window_rect.right = width;
	window_rect.top = 0;
	//AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

	UINT w = window_rect.right - window_rect.left;
	UINT h = window_rect.bottom - window_rect.top;

	window_rect.left = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
	window_rect.right = window_rect.left + w;
	window_rect.top = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
	window_rect.bottom = window_rect.top + h;
	
	m_params.hwnd = CreateWindow(class_name, window_name, WS_POPUP | WS_CLIPCHILDREN, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, NULL, NULL, m_params.hinstance, NULL);
}

VulkanWindow::~VulkanWindow()
{
	UnregisterClass("MainWindow", m_params.hinstance);
}

void VulkanWindow::show() const
{
	ShowWindow(m_params.hwnd, SW_SHOW);
}

uint32_t VulkanWindow::getX() const noexcept
{
	RECT rect;
	GetClientRect(m_params.hwnd, &rect);

	return rect.left;
}

uint32_t VulkanWindow::getY() const noexcept
{
	RECT rect;
	GetClientRect(m_params.hwnd, &rect);

	return rect.top;
}

uint32_t VulkanWindow::getWidth() const noexcept
{
	RECT rect;
	GetClientRect(m_params.hwnd, &rect);

	return rect.right - rect.left;
}

uint32_t VulkanWindow::getHeight() const noexcept
{
	RECT rect;
	GetClientRect(m_params.hwnd, &rect);

	return rect.bottom - rect.top;
}

///////////////////////////////////////////////////////////////////////////////////////////
////////////////////					L I N U X					///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#elif defined(VK_USE_PLATFORM_XCB_KHR)

void InputManager::ManageInput(std::unique_ptr<input_t, std::function<void(input_t*)>>&& input)
{
	switch(input->response_type)
	{
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t* ev = (xcb_button_press_event_t*)input.get();
			
			switch(ev->detail)
			{
				case 1: //lmb
				InputManager::KeyStateBuffer.mouse |= LMB;
				MousePressed(LMB);
				break;
				case 2: //scroll press
				InputManager::KeyStateBuffer.mouse |= MMB;
				MousePressed(MMB);
				break;
				case 3: //rmb
				InputManager::KeyStateBuffer.mouse |= RMB;
				MousePressed(RMB);
				break;
				case 4: //scroll up
				MouseScrolledUp();
				break;
				case 5: //scroll down
				MouseScrolledDown();
				break;
			}
			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t* ev = (xcb_button_press_event_t*)input.get();
			
			switch(ev->detail)
			{
				case 1: //lmb
				InputManager::KeyStateBuffer.mouse &= ~LMB;
				MouseReleased(LMB);
				break;
				case 2: //scroll press
				InputManager::KeyStateBuffer.mouse &= ~MMB;
				MouseReleased(MMB);
				break;
				case 3: //rmb
				InputManager::KeyStateBuffer.mouse &= ~RMB;
				MouseReleased(RMB);
				break;
				default:
				break;
			}
			break;
		}
		case XCB_MOTION_NOTIFY:
		{
			xcb_motion_notify_event_t* ev = (xcb_motion_notify_event_t*)input.get();
			
			int16_t dx = ev->event_x - InputManager::lastMouseX;
			int16_t dy = ev->event_y - InputManager::lastMouseY;
			InputManager::lastMouseX = ev->event_x;
			InputManager::lastMouseY = ev->event_y;
			
			//if state == 0, then no buttons pressed, call moved
			if (ev->state == 0)
				MouseMoved(dx, dy);
			else //if state != 0, then buttons pressed, call dragged
				MouseDragged(dx, dy);
			
			break;
		}
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* ev = (xcb_key_press_event_t*)input.get();
			
			InputManager::KeyStateBuffer.keyboard[ev->detail] = true;
			KeyPressed(ev->detail);
			
			break;
		}
		case XCB_KEY_RELEASE:
		{
			xcb_key_release_event_t* ev = (xcb_key_release_event_t*)input.get();
			
			InputManager::KeyStateBuffer.keyboard[ev->detail] = false;
			KeyReleased(ev->detail);
			
			break;
		}
	}
}

bool VulkanWindow::manageEvents(InputManager& im)
{
	//poll for event
	xcb_generic_event_t* e;
	e = xcb_poll_for_event(m_params.connection);
	
	//return if no events
	if (e == nullptr)
		return false;
		
	switch(e->response_type)
	{
		case XCB_BUTTON_PRESS: 
		case XCB_BUTTON_RELEASE:
		case XCB_MOTION_NOTIFY:
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
			static auto event_del = [](input_t* p) { free(p); };
			im.ManageInput(std::unique_ptr < input_t, decltype(event_del)> (e, event_del));
			break;
		//case XCB_RESIZE_REQUEST:
			//VulkanEngine::get()->onResize();
			//break;
		case XCB_CONFIGURE_NOTIFY:
		xcb_configure_notify_event_t* ev = (xcb_configure_notify_event_t*)e;
			m_x = ev->x;
			m_y = ev->y;
			m_width = ev->width;
			m_height = ev->height;
			VulkanEngine::get().onResize();

			//free the event after handling
			free(e);

			break;
	}
	return true;
}

std::pair<int16_t, int16_t> InputManager::getCursorPosWin(VulkanWindow& win)
{
	auto cookie = xcb_query_pointer(win.getParams().connection, win.getParams().window);
	auto reply = xcb_query_pointer_reply(win.getParams().connection, cookie, nullptr);
	
	return std::pair<int16_t, int16_t>(reply->win_x, reply->win_y);
	
}

VulkanWindow::VulkanWindow()
{
	/* Open the connection to the X server */
	xcb_connection_t *connection = xcb_connect (NULL, NULL);

	/* Get the first screen */
	const xcb_setup_t      *setup  = xcb_get_setup (connection);
	xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
	xcb_screen_t           *screen = iter.data;
	
	/* Create the window */
	uint32_t window = xcb_generate_id(connection);
	uint32_t event_mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION;
	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0,0,1920,1080, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, &event_mask);
	
	m_params.connection = connection;
	m_params.window = window;
}

VulkanWindow::~VulkanWindow()
{
	xcb_disconnect(m_params.connection);
}

void VulkanWindow::show() const
{
	xcb_map_window(m_params.connection, m_params.window);
	xcb_flush(m_params.connection);
}

uint32_t VulkanWindow::getX() const noexcept
{
	return m_x;
}

uint32_t VulkanWindow::getY() const noexcept
{
	return m_y;
}

uint32_t VulkanWindow::getWidth() const noexcept
{
	return m_width;
}

uint32_t VulkanWindow::getHeight() const noexcept
{
	return m_height;
}

#endif //VK_USE_PLATFORM_WIN32_KHR