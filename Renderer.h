#pragma once
#include "framework.h"

#include <bitset>

#include "DirectX11Handler.h"

#include "Math.h"

#include "Camera.h"
#include "GlobalLight.h"

#include "GuiManager.h"
#include "ReObjects.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class EngineCore;

enum class BufferUsage {
	
};

struct SceneDescription {
	bool globalLightsEnabled = true;
};
struct Scene {
	Buffer      globalLightBuffer;
	GlobalLight globalLightData;
};

class Renderer {
public:
	friend class EngineCore;

private:
	Window* window;

	DirectX11Handler* handler;

	GuiManager* guiManager;

	ObjectsManager* objectsManager;

	Assimp::Importer importer;

	ThreadPool scheduler;

	DirectX11Handler* getDirectX11Handler() { return this->handler; }

public:
	Scene			 scene;
	SceneDescription sceneDescription;

	Camera* camera;

	~Renderer();

	void build(ObjectsManager* objectsManager, Window* window, GuiManager* guiManager, const size_t threadsAmount);

	Mesh createMesh(std::vector<Vertex>& vertices, std::vector<DWORD>& indices);
	Mesh createMesh(const char* path);
	
	Shader createShaderFromSource(const char* vertexShaderSource, const char* pixelShaderSource);

	Texture createTexture(const char* path);

	Model createModel(std::vector<Vertex>& vertices, 
					  std::vector<DWORD>&  indices,
					  const char*          vertexShaderSource, 
					  const char*		   pixelShaderSource,
					  const char*          texturePath);
	Model createModel(const char* path,
					  const char* vertexShaderSource, 
					  const char* pixelShaderSource,
					  const char* texturePath);

	template <typename Ty>
	void createBuffer(Model* bufferParent, Ty& bufferData) {
		Buffer buffer = this->handler->createConstantBuffer<Ty>(&bufferData);
		bufferParent->buffers.push_back(buffer.buffer);
	}

	void createGlobalLight();

	void clearScreen();

	void render();
	void present();

	void toQueue(Model&& model);
};