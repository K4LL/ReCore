#pragma once
#include <iostream>
#include <Windows.h>

class Time {
private:
	static float currentTime;
	static float lastTime;

public:	
	static void startUpdate();
	static void endUpdate();

	static float deltaTime() noexcept;
};