#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>

using keycode_t = uint8_t; //keyboard code type
using mbflag_t = uint8_t; //mouse buttons flag

#ifdef _WIN32

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

using input_t = RAWINPUT;

constexpr const keycode_t VKey_1 = 0x31;
constexpr const keycode_t VKey_2 = 0x32;
constexpr const keycode_t VKey_3 = 0x33;
constexpr const keycode_t VKey_4 = 0x34;
constexpr const keycode_t VKey_5 = 0x35;
constexpr const keycode_t VKey_6 = 0x36;
constexpr const keycode_t VKey_7 = 0x37;
constexpr const keycode_t VKey_8 = 0x38;
constexpr const keycode_t VKey_9 = 0x39;
constexpr const keycode_t VKey_0 = 0x30;
constexpr const keycode_t VKey_A = 0x41;
constexpr const keycode_t VKey_B = 0x42;
constexpr const keycode_t VKey_C = 0x43;
constexpr const keycode_t VKey_D = 0x44;
constexpr const keycode_t VKey_E = 0x45;
constexpr const keycode_t VKey_F = 0x46;
constexpr const keycode_t VKey_G = 0x47;
constexpr const keycode_t VKey_H = 0x48;
constexpr const keycode_t VKey_I = 0x49;
constexpr const keycode_t VKey_J = 0x4a;
constexpr const keycode_t VKey_K = 0x4b;
constexpr const keycode_t VKey_L = 0x4c;
constexpr const keycode_t VKey_M = 0x4d;
constexpr const keycode_t VKey_N = 0x4e;
constexpr const keycode_t VKey_O = 0x4f;
constexpr const keycode_t VKey_P = 0x50;
constexpr const keycode_t VKey_Q = 0x51;
constexpr const keycode_t VKey_R = 0x52;
constexpr const keycode_t VKey_S = 0x53;
constexpr const keycode_t VKey_T = 0x54;
constexpr const keycode_t VKey_U = 0x55;
constexpr const keycode_t VKey_V = 0x56;
constexpr const keycode_t VKey_W = 0x57;
constexpr const keycode_t VKey_X = 0x58;
constexpr const keycode_t VKey_Y = 0x59;
constexpr const keycode_t VKey_Z = 0x5a;
constexpr const keycode_t VKey_SPACE = VK_SPACE;
constexpr const keycode_t VKey_ESC = VK_ESCAPE;
constexpr const keycode_t VKey_ENTER = VK_RETURN;
constexpr const keycode_t VKey_LSHIFT = VK_LSHIFT;
constexpr const keycode_t VKey_RSHIFT = VK_RSHIFT;

struct WindowParameters{
		HWND hwnd;
		HINSTANCE hinstance;
};

#elif defined(__linux__)

#define VK_USE_PLATFORM_XCB_KHR
#define VK_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#include <xcb/xcb.h>

constexpr const keycode_t VKey_1 = 0x13;
constexpr const keycode_t VKey_2 = 0x14;
constexpr const keycode_t VKey_3 = 0x15;
constexpr const keycode_t VKey_4 = 0x16;
constexpr const keycode_t VKey_5 = 0x17;
constexpr const keycode_t VKey_6 = 0x18;
constexpr const keycode_t VKey_7 = 0x19;
constexpr const keycode_t VKey_8 = 0x1a;
constexpr const keycode_t VKey_9 = 0x1b;
constexpr const keycode_t VKey_0 = 0x1c;
constexpr const keycode_t VKey_A = 0x26;
constexpr const keycode_t VKey_B = 0x38;
constexpr const keycode_t VKey_C = 0x36;
constexpr const keycode_t VKey_D = 0x28;
constexpr const keycode_t VKey_E = 0x1a;
constexpr const keycode_t VKey_F = 0x29;
constexpr const keycode_t VKey_G = 0x30;
constexpr const keycode_t VKey_H = 0x31;
constexpr const keycode_t VKey_I = 0x1f;
constexpr const keycode_t VKey_J = 0x32;
constexpr const keycode_t VKey_K = 0x33;
constexpr const keycode_t VKey_L = 0x34;
constexpr const keycode_t VKey_M = 0x3a;
constexpr const keycode_t VKey_N = 0x39;
constexpr const keycode_t VKey_O = 0x20;
constexpr const keycode_t VKey_P = 0x21;
constexpr const keycode_t VKey_Q = 0x18;
constexpr const keycode_t VKey_R = 0x1b;
constexpr const keycode_t VKey_S = 0x27;
constexpr const keycode_t VKey_T = 0x1c;
constexpr const keycode_t VKey_U = 0x1e;
constexpr const keycode_t VKey_V = 0x37;
constexpr const keycode_t VKey_W = 0x19;
constexpr const keycode_t VKey_X = 0x35;
constexpr const keycode_t VKey_Y = 0x1d;
constexpr const keycode_t VKey_Z = 0x34;
constexpr const keycode_t VKey_SPACE = 0x41;
constexpr const keycode_t VKey_ESC = 0x09;
constexpr const keycode_t VKey_ENTER = 0x24;
constexpr const keycode_t VKey_LSHIFT = 0x32;
constexpr const keycode_t VKey_RSHIFT = 0x3e;

using input_t = xcb_generic_event_t;

struct WindowParameters{
	xcb_connection_t* connection;
	xcb_window_t window;
};

#else
#error Unsupported platform
#endif //_WIN32

#endif //PLATFORM_H