#include "Time.h"

float Time::currentTime = 0.0f;
float Time::lastTime    = 0.0f;

void Time::startUpdate() {
	Time::currentTime = GetTickCount();
}
void Time::endUpdate() {
	Time::lastTime = Time::currentTime;
}

float Time::deltaTime() noexcept { 
	return (Time::currentTime - Time::lastTime) / 1000.0f;
}