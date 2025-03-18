#include "framework.h"

#include "DebugConsole.h"

#include "DirectX11Handler.h"
#include "Window.h"

#include "Renderer.h"

#include "EngineCore.h"

#include "Math.h"
#include "Meta.h"

#include <chrono>

const char* GroundVertexShaderSource = R"(
cbuffer FrameData : register(b0) {
    matrix model;
    matrix view;
    matrix projection;
    // Optionally, if you need to support non-uniform scaling:
    // matrix modelInverseTranspose;
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
    
    // Transform position to world space then to clip space.
    float4 worldPos = mul(float4(input.position, 1.0f), model);
    output.position = mul(worldPos, view);
    output.position = mul(output.position, projection);
    
    // Transform the normal to world space.
    // For models with only rotation (and uniform scaling):
    output.normal = normalize(mul(input.normal, (float3x3)model));
    
    // If using non-uniform scaling, use the inverse transpose:
    // output.normal = normalize(mul(input.normal, (float3x3)modelInverseTranspose));
    
    output.texCoords = input.texCoords;
    return output;
}
)";

const char* GroundPixelShaderSource = R"(
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
    // Apply scale to texture coordinates
    float2 newTexcoords = input.texCoords * scale.xy;

    // Sample the texture
    float4 texColor = tex.Sample(samplerState, newTexcoords);

    float len = length(direction);
    if (len > 0) {
        // Calculate light contribution (simple diffuse shading)
        // Normalize the light direction
        float3 lightDir = normalize(direction);
    
        // Calculate diffuse shading based on light direction and surface normal
        float diffuse = max(dot(input.normal, -lightDir), 0.0f);

        // Apply the lighting model (ambient + diffuse)
        float4 finalColor = ambient + lightColor * diffuse * intensity;

        // Combine texture color and lighting
        finalColor = texColor * finalColor;
    
        return finalColor;
    }
    else {
		return texColor;
	}
}
)";
const char* DefaultPixelShaderSource = R"(
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
    return tex.Sample(samplerState, input.texCoords);
}
)";
const char* SolidColorPixelShaderSource = R"(
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

cbuffer Color : register(b2) {
    float4 color;
}

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_TARGET {
    return color;
}
)";

std::vector<Vertex> vertices = {
    // Positions                  // Normals                // Texture Coordinates (UV)
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},  // Front face
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

    {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Back face
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

    // Top face
    {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

    // Bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

    // Left face
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

    // Right face
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
};

std::vector<DWORD> indices = {
        0, 1, 2, 0, 2, 3, // Front
        4, 5, 6, 4, 6, 7, // Back
        8, 9, 10, 8, 10, 11, // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Left
        20, 21, 22, 20, 22, 23, // Right
};


void QuaternionToEulerAngles(const DirectX::XMVECTOR& q, float& roll, float& pitch, float& yaw) {
    float x = DirectX::XMVectorGetX(q);
    float y = DirectX::XMVectorGetY(q);
    float z = DirectX::XMVectorGetZ(q);
    float w = DirectX::XMVectorGetW(q);

    // Roll (X-axis rotation)
    float sinr_cosp = 2.0f * (w * x + y * z);
    float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
    roll = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (Y-axis rotation)
    float sinp = 2.0f * (w * y - z * x);
    if (fabsf(sinp) >= 1)
        pitch = copysignf(DirectX::XM_PI / 2, sinp);  // Use 90 degrees if out of range
    else
        pitch = asinf(sinp);

    // Yaw (Z-axis rotation)
    float siny_cosp = 2.0f * (w * z + x * y);
    float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
    yaw = atan2f(siny_cosp, cosy_cosp);
}
// Converts radians to degrees
float radiansToDegrees(float radians) {
    return radians * (180.0f / DirectX::XM_PI);
}

// Converts degrees to radians
float degreesToRadians(float degrees) {
    return degrees * (DirectX::XM_PI / 180.0f);
}

int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    const std::string metaIOPath = "C:\\Users\\gupue\\source\\repos\\ReCore\\Resources\\meta.txt";
    
    MetaIO metaIO;
    metaIO.read(metaIOPath);
    metaIO.printCode();
    //metaIO.print();
    //metaIO.clear(metaIOPath);

    EngineCore core;
	core.build({ L"ReCore Test", 1200, 720 }, 100, { 1, 100 }, 4);
    core.buildDebugTools();

    Renderer*     renderer     = core.getRenderer();
    InputManager* inputManager = core.getInputManager();

    Model groundModel = std::move(renderer->createModel(vertices, indices,
                                                  GroundVertexShaderSource,
                                                  SolidColorPixelShaderSource,
                                                  "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    groundModel.transform.position = DirectX::XMVectorSet(0.0f, -4.5f, 0.0f, 0.0f);
    groundModel.transform.rotation = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    groundModel.transform.scale    = DirectX::XMVectorSet(100.0f, 1.0f, 100.0f, 1.0f);

    renderer->toQueue(std::move(groundModel));
    
    DirectX::XMFLOAT4 color;
    color.x = 0.35f;
    color.y = 0.35f;
    color.z = 0.35f;
    color.w = 0.35f;
    renderer->createBuffer(core.getObjectsManager()->get<std::unique_ptr<Model>>().at(0)->get(), color);


    Model model0 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        GroundPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model0.transform.position = DirectX::XMVectorSet(0, -4.5, 0, 1.0f);
    model0.transform.rotation = DirectX::XMVectorSet(0, 0, 0, 1.0f);
    model0.transform.scale = DirectX::XMVectorSet(100, 1, 100, 1.0f);

    renderer->toQueue(std::move(model0));


    Model model1 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        GroundPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model1.transform.position = DirectX::XMVectorSet(0, 3.5, 0, 1.0f);
    model1.transform.rotation = DirectX::XMVectorSet(1.30037e-10, 0, 0, 1.0f);
    model1.transform.scale = DirectX::XMVectorSet(20, 7, 1, 1.0f);

    renderer->toQueue(std::move(model1));


    Model model2 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        GroundPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model2.transform.position = DirectX::XMVectorSet(21, 3.5, -21, 1.0f);
    model2.transform.rotation = DirectX::XMVectorSet(0, 0.707107, 0, 1.0f);
    model2.transform.scale = DirectX::XMVectorSet(20, 7, 1, 1.0f);

    renderer->toQueue(std::move(model2));


    Model model3 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        GroundPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model3.transform.position = DirectX::XMVectorSet(-21, 3.5, -21, 1.0f);
    model3.transform.rotation = DirectX::XMVectorSet(0, 0.707107, 0, 1.0f);
    model3.transform.scale = DirectX::XMVectorSet(20, 7, 1, 1.0f);

    renderer->toQueue(std::move(model3));


    Model model4 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        GroundPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model4.transform.position = DirectX::XMVectorSet(0, 3.5, -41.9999, 1.0f);
    model4.transform.rotation = DirectX::XMVectorSet(0, 0, 0, 1.0f);
    model4.transform.scale = DirectX::XMVectorSet(20, 7, 1, 1.0f);

    renderer->toQueue(std::move(model4));


    Model model5 = std::move(renderer->createModel(vertices, indices,
        GroundVertexShaderSource,
        SolidColorPixelShaderSource,
        "C:\\Users\\gupue\\source\\repos\\ReCore\\grass-block.png"));
    model5.transform.position = DirectX::XMVectorSet(0, 11.5, -20, 1.0f);
    model5.transform.rotation = DirectX::XMVectorSet(0, 0, 0, 1.0f);
    model5.transform.scale = DirectX::XMVectorSet(40, 1, 40, 1.0f);

    renderer->toQueue(std::move(model5));

    auto changeCameraPosition = [&](void* param) {
        Renderer* renderer = reinterpret_cast<Renderer*>(param);
        Scene*    scene    = &renderer->scene;
        Camera*   camera   = renderer->camera;

        auto queue = core.getObjectsManager()->get<std::unique_ptr<Model>>();

        static bool onMenu = true;
        static int  index  = 0;

        static std::vector<char> modelPath(256, '\0');
        static std::vector<char> texturePath(256, '\0');

        if (ImGui::Begin("Selection Menu")) {
            ImGui::InputInt("Object Index", &index, 1, 1);
            if (ImGui::Button(onMenu ? "Open Object Data" : "Close Object Data")) {
                onMenu = !onMenu;
            }

            ImGui::InputText("Model Path", modelPath.data(), 255);
            ImGui::InputText("Texture Path", texturePath.data(), 255);

            if (ImGui::Button("Create Object")) {
                renderer->toQueue(std::move(renderer->createModel(modelPath.data(),
                                            GroundVertexShaderSource, GroundPixelShaderSource, 
                                            texturePath.data())));
            }
        }
        ImGui::End();

        if (onMenu) return;

        Model* currentModel = nullptr;
        Transform* transform = nullptr;

        if (index < queue.size() && index > -1) currentModel = queue.at(index)->get();
        if (currentModel) transform = &currentModel->transform;
        if (index == -1) transform = &camera->transform;

        if (transform == nullptr) {
            ImGui::TextWrapped("There is nothing to see here... Load some object first!");
            return;
        }

        // Create a static map to store Euler angles for each object (using index as key).
        static std::unordered_map<int, DirectX::XMFLOAT3> objectEulerAngles;

        // If this object doesn't have stored Euler angles yet, initialize them.
        if (objectEulerAngles.find(index) == objectEulerAngles.end()) {
            float pitch, yaw, roll;
            QuaternionToEulerAngles(transform->rotation, pitch, yaw, roll);
            // Convert to degrees so the UI is easier to use.
            objectEulerAngles[index] = DirectX::XMFLOAT3(radiansToDegrees(pitch), radiansToDegrees(yaw), radiansToDegrees(roll));
        }

        // Retrieve the stored Euler angles.
        DirectX::XMFLOAT3& eulerAngles = objectEulerAngles[index];

        // Also get position for editing.
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 scale;

        DirectX::XMStoreFloat3(&position, transform->position);
        DirectX::XMStoreFloat3(&scale, transform->scale);

        // UI window for transform
        ImGui::SetNextWindowSize(ImVec2(350, 175), ImGuiCond_Appearing);
        if (ImGui::Begin("Transform", nullptr, ImGuiWindowFlags_None)) {
            // Update position if edited.
            if (ImGui::InputFloat("Position X", &position.x, 0.1f, 1.0f) ||
                ImGui::InputFloat("Position Y", &position.y, 0.1f, 1.0f) ||
                ImGui::InputFloat("Position Z", &position.z, 0.1f, 1.0f))
            {
                transform->position = DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f);
            }

            // Update rotation if edited.
            if (ImGui::InputFloat("Rotation X", &eulerAngles.x, 0.1f, 1.0f) ||
                ImGui::InputFloat("Rotation Y", &eulerAngles.y, 0.1f, 1.0f) ||
                ImGui::InputFloat("Rotation Z", &eulerAngles.z, 0.1f, 1.0f))
            {
                // Convert the Euler angles (in degrees) to a quaternion.
                DirectX::XMVECTOR nrot = DirectX::XMQuaternionRotationRollPitchYaw(
                    degreesToRadians(eulerAngles.x),
                    degreesToRadians(eulerAngles.y),
                    degreesToRadians(eulerAngles.z)
                );
                transform->rotation = DirectX::XMQuaternionNormalize(nrot);
                DirectX::XMFLOAT4 r;
                DirectX::XMStoreFloat4(&r, transform->rotation);
                RC_DBG_LOG(r.x << " " << r.y << " " << r.z << " " << r.w << '\n');
            }

            // Update scale if edited.
            if (ImGui::InputFloat("Scale X", &scale.x, 0.1f, 1.0f) ||
                ImGui::InputFloat("Scale Y", &scale.y, 0.1f, 1.0f) ||
                ImGui::InputFloat("Scale Z", &scale.z, 0.1f, 1.0f))
            {
                transform->scale = DirectX::XMVectorSet(scale.x, scale.y, scale.z, 1.0f);
            }

            if (ImGui::Button("Build Meta")) {
                Model* toMetaModel = nullptr;
                toMetaModel = queue.at(index)->get();

                ModelMeta modelMeta   = {};
                modelMeta.modelPath   = toMetaModel->mesh.path;
                modelMeta.texturePath = toMetaModel->texture.path;

                DirectX::XMStoreFloat3(&modelMeta.position, toMetaModel->transform.position);
                DirectX::XMStoreFloat3(&modelMeta.rotation, toMetaModel->transform.rotation);
                DirectX::XMStoreFloat3(&modelMeta.scale, toMetaModel->transform.scale);

                metaIO.write(metaIOPath, modelMeta);
            }
        }
        ImGui::End();
    };

    renderer->camera->transform.scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    auto playerControl = [&]() {
        Camera* camera = renderer->camera;

        float lookSpeed = 0.5f;

        if (!camera->isMouseRotating) {
            GetCursorPos(&camera->lastMousePos);
            camera->isMouseRotating = true;
        }
        else {
            // When processing mouse input:
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);

            float deltaX = ((currentMousePos.x - camera->lastMousePos.x) * lookSpeed) * Time::deltaTime();
            camera->lastMousePos = currentMousePos;

            camera->eulerAngles.yaw += deltaX;

            constexpr float maxPitch = DirectX::XMConvertToRadians(89.9f);
            camera->eulerAngles.pitch = std::clamp(camera->eulerAngles.pitch, -maxPitch, maxPitch);

            camera->transform.rotation = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, camera->eulerAngles.yaw, 0.0f);
            camera->transform.rotation = DirectX::XMQuaternionNormalize(camera->transform.rotation);
        }

        const float speed = 5.0f;
        DirectX::XMFLOAT4 co;
        DirectX::XMStoreFloat4(&co, camera->transform.rotation);

        DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), camera->transform.rotation
        );
        DirectX::XMVECTOR right = DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), camera->transform.rotation
        );

        DirectX::XMVECTOR rotatedForward = DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), camera->transform.rotation
        );

        DirectX::XMVECTOR lastPosition = camera->transform.position;

        if (inputManager->getKey('W')) {
            camera->transform.position = DirectX::XMVectorAdd(
                camera->transform.position,
                DirectX::XMVectorScale(forward, speed * Time::deltaTime())
            );
        }
        if (inputManager->getKey('S')) {
            camera->transform.position = DirectX::XMVectorSubtract(
                camera->transform.position,
                DirectX::XMVectorScale(forward, speed * Time::deltaTime())
            );
        }
        if (inputManager->getKey('A')) {
            camera->transform.position = DirectX::XMVectorAdd(
                camera->transform.position,
                DirectX::XMVectorScale(right, speed * Time::deltaTime())
            );
        }
        if (inputManager->getKey('D')) {
            camera->transform.position = DirectX::XMVectorSubtract(
                camera->transform.position,
                DirectX::XMVectorScale(right, speed * Time::deltaTime())
            );
        }
    };

    core.setMainLoop([&]() {
        renderer->clearScreen();

        auto guiManager = core.getGuiManager();
        guiManager->createNewFrame(changeCameraPosition, &renderer);

        // playerControl();
        renderer->camera->update();

        renderer->render();

        renderer->present();
    }, -1);
    core.init();

    return 0;
}