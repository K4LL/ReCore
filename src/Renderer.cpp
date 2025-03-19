#include "Renderer.h"

Renderer::~Renderer() {
	delete Renderer::handler;
}

void Renderer::build(ObjectsManager* objectsManager, Window* window, GuiManager* guiManager, const size_t threadsAmount) {
	this->window		 = window;
	this->objectsManager = objectsManager;
	this->guiManager	 = guiManager;

	DirectX11HandlerDescription handlerDescription = {};
	handlerDescription.backBufferCount = 2;
	handlerDescription.vSync           = FALSE;

	this->handler = new DirectX11Handler(this->window, handlerDescription);
	this->window->setWIP(this->handler);
	this->scheduler.build(threadsAmount);

	this->handler->prepare();

	this->camera = new Camera(this->window);
}

Mesh Renderer::createMesh(std::vector<Vertex>& vertices, std::vector<DWORD>& indices) {
	return this->handler->createVertexArrayBuffer(vertices, indices);
}
Mesh Renderer::createMesh(const char* path) {
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	RC_EI_ASSERT(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode,
		"ERROR::ASSIMP::" << importer.GetErrorString()
	);

	aiMesh* mesh = scene->mMeshes[0];

	std::vector<Vertex> vertices;
	std::vector<DWORD>  indices;

	std::thread verticesThread([&]() {
		vertices.reserve(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex = {};

			vertex.position = DirectX::XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			if (mesh->HasNormals()) {
				vertex.normal = DirectX::XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}
			if (mesh->mTextureCoords[0]) {
				vertex.uv = DirectX::XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}

			vertices.push_back(vertex);
		}
		});

	std::thread indicesThread([&]() {
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			auto& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}
		});

	verticesThread.join();
	indicesThread.join();

	Mesh retVal = this->handler->createVertexArrayBuffer(vertices, indices);
	retVal.path = path;

	return retVal;
}

Shader Renderer::createShaderFromSource(const char* vertexShaderSource, const char* pixelShaderSource) {
	return this->handler->createShadersFromSource(vertexShaderSource, pixelShaderSource);
}

Texture Renderer::createTexture(const char* path) {
	Texture texture;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>                      texture2D;
	std::unique_ptr<unsigned char[], decltype(&stbi_image_free)> imageData(nullptr, stbi_image_free);

	this->handler->createTexture2D(&texture2D, &imageData, path, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	this->handler->createShaderResourceView(&texture.texture, texture2D);
	texture.image = std::move(imageData);

	texture.path = path;

	return texture;
}

Model Renderer::createModel(std::vector<Vertex>& vertices,
							std::vector<DWORD>& indices,
							const char*			vertexShaderSource,
							const char*		    pixelShaderSource,
							const char*			texturePath)
{
	Shader  shader = this->handler->createShadersFromSource(vertexShaderSource, pixelShaderSource);
	Mesh    mesh = this->handler->createVertexArrayBuffer(vertices, indices);
	Texture tex = this->createTexture(texturePath);

	Model model = {};
	model.mesh = std::move(mesh);
	model.shader = std::move(shader);
	model.texture = std::move(tex);

	return model;
}
Model Renderer::createModel(const char* path,
						    const char* vertexShaderSource,
							const char* pixelShaderSource,
							const char* texturePath)
{
	Shader  shader = this->handler->createShadersFromSource(vertexShaderSource, pixelShaderSource);
	Mesh    mesh = this->createMesh(path);
	Texture tex = this->createTexture(texturePath);


	Model model = {};
	model.mesh = std::move(mesh);
	model.shader = std::move(shader);
	model.texture = std::move(tex);

	return model;
}

void Renderer::createGlobalLight() {
	GlobalLight globalLight = {};
	globalLight.ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	globalLight.lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	globalLight.direction = DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f);
	globalLight.intensity = 1.0f;

	this->scene.globalLightData = globalLight;
	this->scene.globalLightBuffer = this->handler->createConstantBuffer<GlobalLight>(&this->scene.globalLightData);
}

void Renderer::clearScreen() {
	this->handler->clearScreen();
}

void Renderer::render() {
	this->handler->prepare();

	this->objectsManager->get<std::unique_ptr<Model>>()
		.for_each([this](std::unique_ptr<Model>& model) {
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;

		DirectX::XMStoreFloat3(&position, model.get()->transform.position);
		DirectX::XMStoreFloat4(&rotation, model.get()->transform.rotation);
		DirectX::XMStoreFloat3(&scale, model.get()->transform.scale);

		auto& transform = model.get()->transform;

		DirectX::XMMATRIX scaleMatrix    = DirectX::XMMatrixScalingFromVector(transform.scale);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(transform.rotation);
		DirectX::XMMATRIX positionMatrix = DirectX::XMMatrixTranslationFromVector(transform.position);

		transform.model = scaleMatrix * rotationMatrix * positionMatrix;

		FrameData fm  = {};
		fm.model      = DirectX::XMMatrixTranspose(transform.model);
		fm.projection = DirectX::XMMatrixTranspose(this->camera->projectionMatrix);
		fm.view       = DirectX::XMMatrixTranspose(this->camera->viewMatrix);

		Buffer cbuffer = Renderer::handler->createConstantBuffer<FrameData>(&fm);

		std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> buffers = { cbuffer.buffer };
		this->handler->VSBindBuffers(buffers);

		struct AlignedScale {
			DirectX::XMFLOAT3 scale;
			float             padding = 0.0f;
		} ascale;
		DirectX::XMStoreFloat3(&ascale.scale, model.get()->transform.scale);

		Buffer scaleBuffer = Renderer::handler->createConstantBuffer<AlignedScale>(&ascale);

		std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> lightBuffer = {
			this->scene.globalLightBuffer.buffer,
			scaleBuffer.buffer,
		};
		for (int j = 0; j < model.get()->buffers.size(); j++) {
			lightBuffer.push_back(model.get()->buffers[j]);
		}
		this->handler->PSBindBuffers(lightBuffer);

		this->handler->bindShaderResource(model.get()->texture.texture);

		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
		this->handler->createSamplerState(&samplerState);
		this->handler->bindSamplerState(samplerState);

		this->handler->render(model.get()->shader, model.get()->mesh);
	});

	guiManager->render();
}
void Renderer::present() {
	this->handler->present();
}

void Renderer::toQueue(Model&& model) {
	std::unique_ptr<Model> ptr = std::make_unique<Model>(std::move(model));
	this->objectsManager->createEntity(std::move(ptr));
}