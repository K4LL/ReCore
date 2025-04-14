#include "RCTime.h"

size_t RCTime::currentTime = 0.0f;
size_t RCTime::lastTime    = 0.0f;

void RCTime::startUpdate() {
	RCTime::currentTime = GetTickCount64();
}
void RCTime::endUpdate() {
	RCTime::lastTime = RCTime::currentTime;
}

float RCTime::deltaTime() noexcept {
	return (RCTime::currentTime - RCTime::lastTime) / 1000.0f;
}