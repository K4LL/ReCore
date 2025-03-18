#include "InputManager.h"

#include "Window.h"

void InputManager::build(Window* window) {
	this->window = window;
}

void InputManager::update() {
	MSG msg;
	while (PeekMessage(&msg, this->window->hWnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

BOOL InputManager::poolEvents() {
	BOOL returnValue;
	while (returnValue = PeekMessage(&this->msg, NULL, 0, 0, PM_REMOVE)) {
		if (this->msg.message == WM_QUIT) {
			this->squit = TRUE;
			return FALSE;
		}

		TranslateMessage(&this->msg);
		DispatchMessage(&this->msg);
	}
	return returnValue;
}