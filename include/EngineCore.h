#pragma once
#include "ReObjects.h"
#include "Renderer.h"
#include "Window.h"
#include "GuiManager.h"
#include "Time.h"
#include "InputManager.h"
#include "DebugConsole.h"

struct WindowDescription {
	const wchar_t* windowName;
	int            width;
	int            heigth;
};
struct RendererDescription {
	
};
struct ObjectsManagerDescription {
	int initialSize;
	int threadsAmmount;
};

class EngineCore {
private:
	std::unique_ptr<Window>         window;
	std::unique_ptr<Renderer>       renderer;
	std::unique_ptr<ObjectsManager> objectsManager;
	std::unique_ptr<GuiManager>     guiManager;
	std::unique_ptr<InputManager>   inputManager;
	std::unique_ptr<ThreadPool>     scheduler;

	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point endTime;
	
	float internalTargetFPS;

	std::function<void()> _updateFunction;

public:
	~EngineCore() {
		DebugConsole::del();
	}

	void build(const  WindowDescription		    windowDescription,
		       const  size_t                    rendererThreadsAmount,
		       const  ObjectsManagerDescription objectsManagerDescription, 
		       const  size_t					threadsAmount)
	{
		window		   = std::make_unique<Window>(windowDescription.windowName, 0, 0, windowDescription.width, windowDescription.heigth);
		renderer       = std::make_unique<Renderer>();
		objectsManager = std::make_unique<ObjectsManager>();
		guiManager     = std::make_unique<GuiManager>();
		inputManager   = std::make_unique<InputManager>();
		scheduler      = std::make_unique<ThreadPool>();

		scheduler->build(threadsAmount);
		renderer->build(this->objectsManager.get(), this->window.get(), this->guiManager.get(), rendererThreadsAmount);
		objectsManager->build(objectsManagerDescription.initialSize, objectsManagerDescription.threadsAmmount);		
		guiManager->build(this->window.get(), this->renderer->getDirectX11Handler());
		inputManager->build(this->window.get());
	}
	void buildDebugTools() {
		DebugConsole::init();

		RCStreamBuffer coutBuffer(std::cout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		RCStreamBuffer cerrBuffer(std::cerr, FOREGROUND_RED | FOREGROUND_INTENSITY);
	}

	void setMainLoop(std::function<void()> updateFunction, const int targetFPS) {
		this->internalTargetFPS = 1.0f / targetFPS;
		
		this->_updateFunction = [this, updateFunction]() {
			while (!this->inputManager->shouldQuit()) {
				this->inputManager->poolEvents();

				float frameDuration = std::chrono::duration<float>(endTime - startTime).count();
				if (frameDuration < this->internalTargetFPS) {
					std::this_thread::sleep_for(std::chrono::duration<float>(this->internalTargetFPS - frameDuration));
				}

				startTime = std::chrono::high_resolution_clock::now();
				RCTime::startUpdate();

				updateFunction();

				RCTime::endUpdate();
				endTime = std::chrono::high_resolution_clock::now();
			}
		};
	}
	void init() {
		this->_updateFunction();
	}

	Window* getWindow() noexcept { return this->window.get(); }
	Renderer* getRenderer() noexcept { return this->renderer.get(); }
	ObjectsManager* getObjectsManager() noexcept { return this->objectsManager.get(); }
	GuiManager* getGuiManager() noexcept { return this->guiManager.get(); }
	InputManager* getInputManager() noexcept { return this->inputManager.get(); }
	ThreadPool* getScheduler() noexcept { return this->scheduler.get(); }
};