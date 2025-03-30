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
	const char* vertexShaderSource,
	const char* pixelShaderSource,
	const char* texturePath)
{
	Shader  shader = this->handler->createShadersFromSource(vertexShaderSource, pixelShaderSource);
	Mesh    mesh   = this->handler->createVertexArrayBuffer(vertices, indices);
	Texture tex    = this->createTexture(texturePath);

	Model model   = {};
	model.mesh    = std::move(mesh);
	model.shader  = std::move(shader);
	model.texture = std::move(tex);

	return model;
}
Model Renderer::createModel(const char* path,
						    const char* vertexShaderSource,
							const char* pixelShaderSource,
							const char* texturePath)
{
	Shader  shader = this->handler->createShadersFromSource(vertexShaderSource, pixelShaderSource);
	Mesh    mesh   = this->createMesh(path);
	Texture tex    = this->createTexture(texturePath);


	Model model   = {};
	model.mesh    = std::move(mesh);
	model.shader  = std::move(shader);
	model.texture = std::move(tex);

	return model;
}

Model Renderer::getTemplate(const ModelTemplate t, void* params) {
	switch (t)
	{
	case ModelTemplate::Billboard: { 
		const char* BillboardVertexShaderSource = R"(
		// Some code i got from chatgpt
		cbuffer FrameData : register(b0) {
			matrix model;
			matrix view;
			matrix projection;
		};
		cbuffer CameraRotation : register(b1) {
			float4 qCameraRotation;
		};

		struct VSInput {
			float3 position : POSITION;
			float3 normal   : NORMAL;
			float2 texCoords: TEXCOORD;
		};

		struct PSInput {
			float4 position : SV_POSITION;
			float3 normal   : NORMAL;
			float2 texCoords: TEXCOORD;
		};

		PSInput VSMain(VSInput input) {
			PSInput output;

			// The billboard center is assumed to be stored in model[3].xyz.
			float3 billboardCenter = model[3].xyz;

			// Convert the camera rotation quaternion into right and up vectors.
			// Note: Ensure the quaternion is normalized.
			float x = qCameraRotation.x;
			float y = qCameraRotation.y;
			float z = qCameraRotation.z;
			float w = qCameraRotation.w;
    
			// Compute the right vector from the quaternion.
			float3 camRight = float3(
				1.0f - 2.0f * (y * y + z * z),
				2.0f * (x * y + w * z),
				2.0f * (x * z - w * y)
			);

			// Compute the up vector from the quaternion.
			float3 camUp = float3(
				2.0f * (x * y - w * z),
				1.0f - 2.0f * (x * x + z * z),
				2.0f * (y * z + w * x)
			);

			// Calculate the local offset using the scale (assumed to be set to the billboard half-size)
			float3 localOffset = input.position;

			// Construct the billboard world position by offsetting the center along camera axes.
			float3 billboardPos = billboardCenter + camRight * localOffset.x + camUp * localOffset.y;

			// Transform the billboard position to clip space.
			float4 worldPos = float4(billboardPos, 1.0f);
			output.position = mul(mul(worldPos, view), projection);

			output.normal    = input.normal;
			output.texCoords = input.texCoords;
			return output;
		}
		)";
		const char* BillboardPixelShaderSource = R"(
			Texture2D tex : register(t0);
			SamplerState samplerState : register(s0);

			cbuffer LightBuffer : register(b0) {
			float3 direction;

			float4 ambient;
			float4 lightColor;

			float intensity;
		}

		cbuffer Scale : register(b1) {
			float3 scale;
		}

		struct PSInput {
			float4 position : SV_POSITION;
			float3 normal : NORMAL;
			float2 texCoords : TEXCOORD;
		};

		float4 PSMain(PSInput input) : SV_TARGET {
			float4 texColor = tex.Sample(samplerState, input.texCoords);
			return texColor;
		}
		)";

		BillboardDescription* desc = reinterpret_cast<BillboardDescription*>(params);
		Model model                = this->createModel(desc->modelPath, BillboardVertexShaderSource, BillboardPixelShaderSource, desc->texturePath);

		auto& p = this->camera->transform.rotation;
		this->createBuffer<DirectX::XMVECTOR>(&model, PipelineStage::VertexStage, p, "cameraRotation");

		return model;
	}

	default: {
		Model model = {};
		return model;
	}
	}
}

void Renderer::createGlobalLight() {
	GlobalLight globalLight = {};
	globalLight.ambient     = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	globalLight.lightColor  = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	globalLight.direction   = DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f);
	globalLight.intensity   = 1.0f;

	this->scene.globalLightData   = globalLight;
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

		struct AlignedScale {
			DirectX::XMFLOAT3 scale;
			float             padding = 0.0f;
		} ascale;
		DirectX::XMStoreFloat3(&ascale.scale, model.get()->transform.scale);

		Buffer scaleBuffer = Renderer::handler->createConstantBuffer<AlignedScale>(&ascale);

		std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> VSBuffers = { cbuffer.buffer };
		std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> PSBuffers = { this->scene.globalLightBuffer.buffer, scaleBuffer.buffer };

		for (uint32_t i = 0; i < model.get()->buffers.size(); i++) {
			if (model.get()->buffers[i].stage == PipelineStage::VertexStage) VSBuffers.push_back(model.get()->buffers[i].buffer);
			else PSBuffers.push_back(model.get()->buffers[i].buffer);
		}
		this->handler->VSBindBuffers(VSBuffers);
		this->handler->PSBindBuffers(PSBuffers);

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