#pragma once
#include <iostream>
#include <Windows.h>

class RCTime {
private:
	static size_t currentTime;
	static size_t lastTime;

public:	
	static void startUpdate();
	static void endUpdate();

	static float deltaTime() noexcept;
};