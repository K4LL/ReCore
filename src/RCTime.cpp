#include "RCTime.h"

float RCTime::currentTime = 0.0f;
float RCTime::lastTime    = 0.0f;

void RCTime::startUpdate() {
	RCTime::currentTime = GetTickCount64();
}
void RCTime::endUpdate() {
	RCTime::lastTime = RCTime::currentTime;
}

float RCTime::deltaTime() noexcept {
	return (RCTime::currentTime - RCTime::lastTime) / 1000.0f;
}