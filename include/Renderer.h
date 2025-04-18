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

enum class ModelTemplate {
	Billboard,
};
struct BillboardDescription {
	const char*			  modelPath;
	const char*			  texturePath;
	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;
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

	Mesh createMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	Mesh createMesh(const char* path);
	
	Shader createShaderFromSource(const char* vertexShaderSource, const char* pixelShaderSource);

	Texture createTexture(const char* path);

	Model createModel(const std::vector<Vertex>&    vertices, 
					  const std::vector<uint32_t>&  indices,
					  const char*                   vertexShaderSource, 
					  const char*					pixelShaderSource,
					  const char*					texturePath);
	Model createModel(const char* path,
					  const char* vertexShaderSource, 
					  const char* pixelShaderSource,
					  const char* texturePath);

	Model getTemplate(const ModelTemplate t, void* params);

	template <typename Ty>
	void createBuffer(Model* bufferParent, const PipelineStage stage, const Ty& bufferData, const std::string name = "") {
		Buffer buffer = {};
		this->handler->createConstantBuffer<Ty>(&buffer.buffer, bufferData);
		buffer.name   = std::move(name);
		buffer.stage  = stage;
		bufferParent->buffers.push_back(buffer);
	}
	template <typename Ty>
	void updateBuffer(Model* bufferParent, const Ty& bufferData, const size_t index) {
		this->handler->updateConstantBuffer<Ty>(bufferParent->buffers[index].buffer, bufferData);
	}

	void createGlobalLight();

	void clearScreen();

	void render();
	void present();

	void toQueue(Model&& model);
	void removeModel(const size_t index);
	void removeModel(const std::string& name);
	void removeModel(const Model* model);
};