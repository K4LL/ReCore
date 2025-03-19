#pragma once
#include "framework.h"

class Window;

class InputManager {
private:
	Window* window;

	MSG msg;

	bool squit;

public:
	void build(Window* window);

	void update();

	bool getKey(const char key) {
		if (GetAsyncKeyState(key) & 0x8000) return true;
		else return false;
	}

	BOOL poolEvents();

	BOOL shouldQuit() noexcept { return this->squit; }
};