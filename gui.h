#ifndef GUI_H
#define GUI_H

#include "Platform.h"
#include <memory>
#include <vector>
#include <functional>

class VulkanWindow;

class InputManager
{
private:

	struct sKeyStateBuffer
	{
		std::vector<bool> keyboard = std::vector<bool>(256);
		mbflag_t mouse;
	};

	static uint16_t lastMouseX;
	static uint16_t lastMouseY;
	static sKeyStateBuffer KeyStateBuffer;
	
public:
	void ManageInput(std::unique_ptr<input_t, std::function<void(input_t*)>>&&);

	virtual void MousePressed(mbflag_t) {}
	virtual void MouseReleased(mbflag_t) {}
	virtual void MouseScrolledUp() {}
	virtual void MouseScrolledDown() {}
	virtual void MouseMoved(int16_t dx, int16_t dy) {}
	virtual void MouseDragged(int16_t dx, int16_t dy) {}
	//virtual void MouseHovered() {}

	virtual void KeyPressed(keycode_t) {}
	virtual void KeyReleased(keycode_t) {}
	
	static mbflag_t getMouseState() noexcept;
	static bool getKeyState(keycode_t) noexcept;
	static std::pair<int16_t, int16_t> getCursorPosWin(VulkanWindow&);
};

class VulkanWindow
{
public:
	VulkanWindow();
	~VulkanWindow();

	void show() const;
	//returns if there were any events
	bool manageEvents(InputManager&);
	
	uint32_t getX() const noexcept;
	uint32_t getY() const noexcept;
	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	const WindowParameters& getParams() const noexcept;
private:
	uint32_t m_x;
	uint32_t m_y;
	uint32_t m_width;
	uint32_t m_height;

	WindowParameters m_params;
};

#endif //GUI_H